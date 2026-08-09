/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <catch_amalgamated.hpp>

#include "Rml/SolLua/bind/bind.h"
#include "Rml/Backends/RmlUi_SystemInterface.h"
#include "Rml/SolLua/plugin/SolLuaDataModel.h"
#include "Rml/SolLua/plugin/SolLuaDocument.h"
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
Rml::Context* clearedDebugContext = nullptr;

void MarkContextForRemoval(Rml::Context*) {}
void SetDebugContext(Rml::Context*) {}
bool ClearDebugContext(Rml::Context* context)
{
	clearedDebugContext = context;
	return true;
}
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

namespace Rml::SolLua::document
{
void appendToStyleSheet(SolLuaDocument&, const Rml::String&);
}

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

TEST_CASE("Context:CreateDocument returns a document userdata")
{
	BindingFixture fixture("sol-lua-create-document");

	auto result = fixture.lua.safe_script(
		"local document = context:CreateDocument()\n"
		"assert(document ~= nil)\n"
		"document.title = 'created from Lua'\n"
		"assert(document.title == 'created from Lua')"
	);
	requireLuaSuccess(result);
}

TEST_CASE("Context:UnloadDocument accepts a Lua-created document")
{
	BindingFixture fixture("sol-lua-unload-document");

	auto result = fixture.lua.safe_script(
		"local document = context:CreateDocument()\n"
		"assert(document ~= nil)\n"
		"document.title = 'created from Lua'\n"
		"assert(document.title == 'created from Lua')\n"
		"context:UnloadDocument(document)"
	);
	requireLuaSuccess(result);
}

TEST_CASE("Context:UnloadAllDocuments detaches the debugger context")
{
	BindingFixture fixture("sol-lua-unload-all-documents");
	REQUIRE(fixture.context->CreateDocument() != nullptr);
	RmlGui::clearedDebugContext = nullptr;

	auto result = fixture.lua.safe_script("context:UnloadAllDocuments()");
	requireLuaSuccess(result);

	CHECK(fixture.context->GetNumDocuments() == 0);
	CHECK(RmlGui::clearedDebugContext == fixture.context);
}

TEST_CASE("Element:DispatchEvent accepts Lua table parameters")
{
	BindingFixture fixture("sol-lua-dispatch-event");
	auto* document = fixture.context->CreateDocument();
	REQUIRE(document != nullptr);
	auto* element = document->AppendChild(document->CreateElement("button"));
	REQUIRE(element != nullptr);
	fixture.lua["element"] = element;

	auto result = fixture.lua.safe_script(
		"local received = false\n"
		"element:AddEventListener('custom', function(event)\n"
		"  received = event.parameters.kind == 'synthetic' and event.parameters.count == 3 and event.parameters.flag == true\n"
		"end)\n"
		"assert(element:DispatchEvent('custom', {kind = 'synthetic', count = 3, flag = true}))\n"
		"assert(received)"
	);
	requireLuaSuccess(result);
}

TEST_CASE("Element:GetValue supports select controls")
{
	BindingFixture fixture("sol-lua-form-value");
	auto* document = fixture.context->CreateDocument();
	REQUIRE(document != nullptr);
	auto select = document->CreateElement("select");
	REQUIRE(select != nullptr);
	auto* selectElement = dynamic_cast<Rml::ElementFormControlSelect*>(select.get());
	REQUIRE(selectElement != nullptr);
	selectElement = dynamic_cast<Rml::ElementFormControlSelect*>(document->AppendChild(std::move(select)));
	REQUIRE(selectElement != nullptr);
	auto option = document->CreateElement("option");
	REQUIRE(option != nullptr);
	option->SetAttribute("value", "one");
	selectElement->Add(std::move(option));
	selectElement->SetSelection(0);
	fixture.lua["element"] = selectElement;

	auto result = fixture.lua.safe_script("assert(element:GetValue() == 'one')");
	requireLuaSuccess(result);
}

TEST_CASE("Select options expose the option value field")
{
	BindingFixture fixture("sol-lua-select-options");
	auto* document = fixture.context->CreateDocument();
	REQUIRE(document != nullptr);
	auto select = document->CreateElement("select");
	REQUIRE(select != nullptr);
	auto* selectElement = dynamic_cast<Rml::ElementFormControlSelect*>(select.get());
	REQUIRE(selectElement != nullptr);
	selectElement = dynamic_cast<Rml::ElementFormControlSelect*>(document->AppendChild(std::move(select)));
	REQUIRE(selectElement != nullptr);

	auto option = document->CreateElement("option");
	REQUIRE(option != nullptr);
	option->SetAttribute("value", "one");
	selectElement->Add(std::move(option));
	selectElement->SetSelection(0);
	fixture.lua["element"] = selectElement;

	auto result = fixture.lua.safe_script("assert(element.options[0].value == 'one')");
	requireLuaSuccess(result);
}

TEST_CASE("Document:AppendToStyleSheet handles a new and malformed document")
{
	BindingFixture fixture("sol-lua-append-stylesheet");
	auto* document = dynamic_cast<Rml::SolLua::SolLuaDocument*>(fixture.context->CreateDocument());
	REQUIRE(document != nullptr);
	REQUIRE(document->GetStyleSheetContainer() == nullptr);

	Rml::SolLua::document::appendToStyleSheet(*document, "body { color: rgb(255, 0, 0); }");
	REQUIRE(document->GetStyleSheetContainer() != nullptr);
	Rml::SolLua::document::appendToStyleSheet(*document, "body { color: ");
}

TEST_CASE("Lua data-model arrays expose their size child")
{
	sol::state lua;
	lua.open_libraries(sol::lib::base);

	Rml::SolLua::SolLuaDataModel model(lua);
	model.Table = lua.create_table();
	auto items = lua.create_table();
	items[1] = "One";
	items[2] = "Two";
	model.Table["items"] = items;
	model.ObjectDef = std::make_unique<Rml::SolLua::SolLuaObjectDef>(&model);

	Rml::SolLua::DataVariableReference root(model.Table, "items", "");
	auto size = model.ObjectDef->Child(&root, Rml::DataAddressEntry("size"));
	REQUIRE(size);

	Rml::Variant value;
	REQUIRE(size.Get(value));
	REQUIRE(value.GetType() == Rml::Variant::INT);
	REQUIRE(value.Get<int>() == 2);
}

TEST_CASE("RmlUi element collections use Lua tables")
{
	BindingFixture fixture("sol-lua-element-collections");

	auto result = fixture.lua.safe_script(R"(
		local document = context:CreateDocument()
		document.inner_rml = [[
			<div id="root">
				<span class="item"></span>
				<span class="item"></span>
			</div>
		]]
		document:UpdateDocument()

		local root = document:GetElementById('root')
		assert(root ~= nil)

		local byTag = root:GetElementsByTagName('span')
		assert(type(byTag) == 'table')
		assert(#byTag == 2)
		assert(byTag[1] ~= nil)
		assert(byTag[1].tag_name == 'span')

		local byClass = root:GetElementsByClassName('item')
		assert(type(byClass) == 'table')
		assert(#byClass == 2)
		assert(byClass[2] ~= nil)
		assert(byClass[2].class_name == 'item')

		local selected = root:QuerySelectorAll('.item')
		assert(type(selected) == 'table')
		assert(#selected == 2)
		assert(selected[1] ~= nil)
		assert(selected[1].tag_name == 'span')

	)");
	requireLuaSuccess(result);
}
