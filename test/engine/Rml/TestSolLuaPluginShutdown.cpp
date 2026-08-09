/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include "Rml/SolLua/plugin/SolLuaPlugin.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/RenderInterface.h>

#include <sol2/sol.hpp>

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

void AddPendingDelete(Rml::ElementPtr) {}

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
