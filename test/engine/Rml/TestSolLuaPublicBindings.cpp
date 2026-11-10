/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include "Rml/SolLua/bind/bind.h"
#include "Rml/Backends/RmlUi_SystemInterface.h"
#include "Rml/SolLua/plugin/SolLuaPlugin.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/RenderInterface.h>

#include <sol2/sol.hpp>

namespace
{
class NullRenderInterface final : public Rml::RenderInterface
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

struct BindingFixture
{
	BindingFixture(const char* contextName)
		: renderInterface()
		, lua()
	{
		Rml::SetRenderInterface(&renderInterface);
		REQUIRE(Rml::Initialise());

		lua.open_libraries(sol::lib::base);
		plugin = new Rml::SolLua::SolLuaPlugin(lua, "rmlPublicBindings");
		Rml::RegisterPlugin(plugin);

		namespaceTable = lua.create_named_table("RmlUi");
		Rml::SolLua::bind_global(namespaceTable, plugin);
		Rml::SolLua::bind_context(namespaceTable, plugin);
		Rml::SolLua::bind_document(namespaceTable);
		Rml::SolLua::bind_element(namespaceTable);
		Rml::SolLua::bind_element_derived(namespaceTable);
		Rml::SolLua::bind_element_form(namespaceTable);
		Rml::SolLua::bind_event(namespaceTable);

		context = Rml::CreateContext(contextName, {1024, 768});
		REQUIRE(context != nullptr);
		lua["context"] = context;
	}

	~BindingFixture()
	{
		if (context != nullptr)
			Rml::RemoveContext(context->GetName());
		Rml::UnregisterPlugin(plugin);
		Rml::Shutdown();
	}

	NullRenderInterface renderInterface;
	sol::state lua;
	sol::table namespaceTable;
	Rml::SolLua::SolLuaPlugin* plugin = nullptr;
	Rml::Context* context = nullptr;
};

void requireLuaSuccess(const sol::protected_function_result& result)
{
	REQUIRE(result.valid());
}
} // namespace

namespace RmlGui
{
void MarkContextForRemoval(Rml::Context*) {}
void SetDebugContext(Rml::Context*) {}
bool ClearDebugContext(Rml::Context*) { return true; }
Rml::Context* GetOrCreateContext(const std::string& name)
{
	if (auto* context = Rml::GetContext(name); context != nullptr)
		return context;
	return Rml::CreateContext(name, {1024, 768});
}
Rml::Context* GetContext(const std::string& name) { return Rml::GetContext(name); }
void BeginFrame() {}
void PresentFrame() {}
void SetMouseCursorAlias(std::string, std::string) {}
} // namespace RmlGui

void AddPendingDelete(Rml::ElementPtr) {}

std::vector<std::string> RmlSystemInterface::GetDocumentPathRequests(const Rml::String&)
{
	return {};
}

void RmlSystemInterface::ClearDocumentPathRequests(const Rml::String&)
{
}

TEST_CASE("RmlUi.version is exposed as a string")
{
	BindingFixture fixture("sol-lua-version");

	auto result = fixture.lua.safe_script(
		"assert(type(RmlUi.version) == 'string')\n"
		"assert(#RmlUi.version > 0)"
	);
	requireLuaSuccess(result);
}
