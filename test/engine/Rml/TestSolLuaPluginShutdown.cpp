/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include "Rml/SolLua/bind/bind.h"
#include "Rml/SolLua/plugin/SolLuaPlugin.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/RenderInterface.h>

#include <sol2/sol.hpp>

#include <vector>

namespace
{
class NullRenderInterface : public Rml::RenderInterface
{
public:
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override { return {}; }
	void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
	void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}

	Rml::TextureHandle LoadTexture(Rml::Vector2i& textureDimensions, const Rml::String&) override
	{
		textureDimensions = {};
		return {};
	}

	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override { return {}; }
	void ReleaseTexture(Rml::TextureHandle) override {}
	void EnableScissorRegion(bool) override {}
	void SetScissorRegion(Rml::Rectanglei) override {}
};
}

namespace RmlGui
{
void MarkContextForRemoval(Rml::Context*) {}
}

namespace
{
std::vector<Rml::ElementPtr> pendingDeletes;
}

void AddPendingDelete(Rml::ElementPtr element)
{
	pendingDeletes.emplace_back(std::move(element));
}

TEST_CASE("SolLuaPlugin removes all tracked documents before plugin shutdown")
{
	NullRenderInterface renderInterface;
	Rml::SetRenderInterface(&renderInterface);
	REQUIRE(Rml::Initialise());

	sol::state lua;
	auto* luaPlugin = new Rml::SolLua::SolLuaPlugin(lua, "rmlDocumentId");
	Rml::RegisterPlugin(luaPlugin);

	Rml::Context* context = Rml::CreateContext("sol-lua-shutdown", {1024, 768});
	REQUIRE(context != nullptr);

	Rml::ElementDocument* firstDocument = context->CreateDocument();
	Rml::ElementDocument* secondDocument = context->CreateDocument();
	REQUIRE(firstDocument != nullptr);
	REQUIRE(secondDocument != nullptr);

	luaPlugin->AddDocumentTracking(firstDocument);
	luaPlugin->AddDocumentTracking(secondDocument);

	luaPlugin->RemoveLuaItems();
	context->Update();

	REQUIRE(context->GetNumDocuments() == 0);

	REQUIRE(Rml::RemoveContext(context->GetName()));
	Rml::UnregisterPlugin(luaPlugin);
	Rml::Shutdown();
}

// Lua may keep an element reference across a DOM rebuild. The lifetime hooks
// must distinguish a live element, a detached element still owned by Lua, and
// the same pointer after its object has been destroyed.
TEST_CASE("SolLuaPlugin tracks element lifetime so stale Lua handles are detectable")
{
	NullRenderInterface renderInterface;
	Rml::SetRenderInterface(&renderInterface);
	REQUIRE(Rml::Initialise());

	sol::state lua;
	auto* luaPlugin = new Rml::SolLua::SolLuaPlugin(lua, "rmlDocumentId");
	Rml::RegisterPlugin(luaPlugin);

	Rml::Context* context = Rml::CreateContext("sol-lua-element-lifetime", {1024, 768});
	REQUIRE(context != nullptr);

	Rml::ElementDocument* document = context->CreateDocument();
	REQUIRE(document != nullptr);

	// A null pointer is never alive, and the lookup must not dereference it.
	REQUIRE_FALSE(Rml::SolLua::IsSolLuaElementAlive(nullptr));

	Rml::ElementPtr owned = document->CreateElement("div");
	REQUIRE(owned != nullptr);
	Rml::Element* element = owned.get();

	// Created through the factory, so the plugin's OnElementCreate hook saw it.
	REQUIRE(Rml::SolLua::IsSolLuaElementAlive(element));

	REQUIRE(document->AppendChild(std::move(owned)) == element);
	REQUIRE(Rml::SolLua::IsSolLuaElementAlive(element));

	// Detaching alone must not invalidate it: the caller still owns the element.
	Rml::ElementPtr detached = document->RemoveChild(element);
	REQUIRE(detached != nullptr);
	REQUIRE(Rml::SolLua::IsSolLuaElementAlive(element));

	// Destroying it must. `element` is now exactly the dangling pointer Lua may
	// still be holding; the lookup compares pointer values and never dereferences,
	// so it is safe to ask.
	detached.reset();
	REQUIRE_FALSE(Rml::SolLua::IsSolLuaElementAlive(element));

	REQUIRE(Rml::RemoveContext(context->GetName()));
	Rml::UnregisterPlugin(luaPlugin);
	Rml::Shutdown();
}
// This is the Lua-facing stale-handle reproducer. A widget can retain a child
// while replacing its parent's inner_rml, so the child userdata then points at
// an element which RmlUi has destroyed. Every operation below used to
// dereference that pointer directly; under ASAN the first getter aborts in
// Rml::Element::GetInnerRML. The bindings should treat all of these calls as
// harmless no-ops/empty reads instead.
TEST_CASE("SolLua element bindings ignore stale handles after a DOM rebuild")
{
	NullRenderInterface renderInterface;
	Rml::SetRenderInterface(&renderInterface);
	REQUIRE(Rml::Initialise());

	sol::state lua;
	lua.open_libraries(sol::lib::base);
	auto* luaPlugin = new Rml::SolLua::SolLuaPlugin(lua, "rmlDocumentId");
	Rml::RegisterPlugin(luaPlugin);
	auto namespaceTable = lua.create_named_table("RmlUi");
	Rml::SolLua::bind_element(namespaceTable);

	Rml::Context* context = Rml::CreateContext("sol-lua-stale-element", {1024, 768});
	REQUIRE(context != nullptr);
	Rml::ElementDocument* document = context->CreateDocument();
	REQUIRE(document != nullptr);

	Rml::Element* parent = document->AppendChild(document->CreateElement("div"));
	REQUIRE(parent != nullptr);
	Rml::Element* child = parent->AppendChild(document->CreateElement("span"));
	REQUIRE(child != nullptr);

	lua["parent"] = parent;
	lua["child"] = child;
	auto rebuild = lua.safe_script(R"(
		assert(child.tag_name == 'span')
		parent.inner_rml = '<span id="replacement"></span>'
	)");
	REQUIRE(rebuild.valid());
	REQUIRE(Rml::SolLua::IsSolLuaElementAlive(child));

	// The engine defers deletion until the safe point after Lua returns. Clear
	// the same pending-delete queue here before exercising the stale userdata.
	pendingDeletes.clear();
	REQUIRE_FALSE(Rml::SolLua::IsSolLuaElementAlive(child));

	auto result = lua.safe_script(R"(
		assert(child.inner_rml == '')
		child.inner_rml = '<i>ignored</i>'
		child:SetAttribute('stale', 'ignored')
		child:SetClass('stale', true)
		assert(child.inner_rml == '')
	)");
	REQUIRE(result.valid());
	REQUIRE_FALSE(Rml::SolLua::IsSolLuaElementAlive(child));

	REQUIRE(Rml::RemoveContext(context->GetName()));
	Rml::UnregisterPlugin(luaPlugin);
	Rml::Shutdown();
}
