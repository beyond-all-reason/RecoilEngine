/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This source file is derived from the code
 * at https://github.com/LoneBoco/RmlSolLua
 * which is under the following license:
 *
 * MIT License
 *
 * Copyright (c) 2022 John Norman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SolLuaPlugin.h"

#include "RmlUi/Core/Context.h"
#include "SolLuaInstancer.h"
#include "Rml/Backends/RmlUi_Backend.h"
#include <RmlUi/Core.h>

#include "SolLuaDocument.h"

#include <algorithm>


namespace Rml::SolLua
{
	// One plugin per Lua state that uses RmlUi (e.g. LuaMenu and LuaUI can be alive together).
	// Only ever touched from the main thread.
	static std::vector<SolLuaPlugin*> plugins;
	static SolLuaPlugin* activePlugin = nullptr;
	static int nextSerial = 1;

	SolLuaPlugin::SolLuaPlugin(sol::state_view lua_state)
		: m_lua_state{lua_state}, serial{nextSerial++}
	{
		plugins.push_back(this);
	}

	SolLuaPlugin::SolLuaPlugin(sol::state_view lua_state, const Rml::String& lua_environment_identifier)
		: m_lua_state{lua_state}, m_lua_env_identifier{lua_environment_identifier}, serial{nextSerial++}
	{
		plugins.push_back(this);
	}

	SolLuaPlugin::~SolLuaPlugin()
	{
		plugins.erase(std::remove(plugins.begin(), plugins.end(), this), plugins.end());
		if (activePlugin == this)
			activePlugin = nullptr;
	}

	SolLuaPlugin* SolLuaPlugin::ForState(lua_State* L)
	{
		for (auto* plugin : plugins) {
			if (plugin->GetLuaState() == L)
				return plugin;
		}
		return nullptr;
	}

	SolLuaPlugin* SolLuaPlugin::Active()
	{
		return activePlugin;
	}

	SolLuaPlugin* SolLuaPlugin::Resolve(Rml::Element* hint)
	{
		if (activePlugin != nullptr)
			return activePlugin;

		if (hint != nullptr) {
			if (auto* document = dynamic_cast<SolLuaDocument*>(hint->GetOwnerDocument())) {
				if (auto* plugin = ForState(document->GetLuaState()))
					return plugin;
			}
		}

		// with a single Lua state there is no ambiguity. With several, guessing would bind the
		// object to a state that may not be the one asking (and may be removed before it).
		return plugins.size() == 1 ? plugins.front() : nullptr;
	}

	SolLuaPlugin::ScopedActive::ScopedActive(SolLuaPlugin* plugin)
		: previous{activePlugin}
	{
		if (plugin != nullptr)
			activePlugin = plugin;
	}

	SolLuaPlugin::ScopedActive::ScopedActive(lua_State* L)
		: ScopedActive(ForState(L))
	{
	}

	SolLuaPlugin::ScopedActive::~ScopedActive()
	{
		activePlugin = previous;
	}

	int SolLuaPlugin::GetEventClasses()
	{
		return EVT_BASIC | EVT_DOCUMENT;
	}

	void SolLuaPlugin::AddContextTracking(Context* context) {
		luaContexts.emplace_back(context);
	}

	void SolLuaPlugin::OnContextDestroy(Context* context) {
		luaContexts.erase(std::remove(luaContexts.begin(), luaContexts.end(), context), luaContexts.end());
		std::erase_if(contextNames, [&](const auto& entry) { return entry.second == context->GetName(); });
	}

	Rml::String SolLuaPlugin::InternalContextName(const Rml::String& luaName, bool create)
	{
		if (const auto it = contextNames.find(luaName); it != contextNames.end()) {
			return it->second;
		}
		if (!create) {
			return {};
		}
		Rml::String internalName = luaName;
		if (Rml::GetContext(internalName) != nullptr) {
			// taken by another Lua state
			internalName = luaName + "#" + std::to_string(serial);
		}
		contextNames[luaName] = internalName;
		return internalName;
	}

	Rml::String SolLuaPlugin::LuaContextName(const Rml::String& internalName) const
	{
		for (const auto& [luaName, internal] : contextNames) {
			if (internal == internalName) {
				return luaName;
			}
		}
		return internalName;
	}

	bool SolLuaPlugin::OwnsContext(const Context* context) const
	{
		return std::find(luaContexts.begin(), luaContexts.end(), context) != luaContexts.end();
	}

	void SolLuaPlugin::AddDocumentTracking(ElementDocument* document) {
		luaDocuments.emplace_back(document);
	}

	void SolLuaPlugin::OnDocumentUnload(ElementDocument* document) {
		luaDocuments.erase(std::remove(luaDocuments.begin(), luaDocuments.end(), document), luaDocuments.end());
	}

	void SolLuaPlugin::RemoveLuaItems(){
		for(auto d: luaDocuments) {
			d->Close();
		}
		for(auto c: luaContexts) {
			RmlGui::MarkContextForRemoval(c);
		}
	}

	void SolLuaPlugin::OnInitialise()
	{
		// RmlUi's factories are global, so the instancers are shared by every plugin and pick the
		// plugin (Lua state) per call. They must outlive any single plugin, which can be removed
		// while another one (e.g. LuaMenu's) keeps running.
		static SolLuaDocumentElementInstancer documentInstancer;
		static SolLuaEventListenerInstancer listenerInstancer;
		Factory::RegisterElementInstancer("body", &documentInstancer);
		Factory::RegisterEventListenerInstancer(&listenerInstancer);
	}

	void SolLuaPlugin::OnShutdown()
	{
		// m_lua_state.collect_garbage();
		delete this;
	}
} // end namespace Rml::SolLua
