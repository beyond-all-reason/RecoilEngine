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

	SolLuaPlugin::SolLuaPlugin(sol::state_view lua_state)
		: m_lua_state{lua_state}
	{
		plugins.push_back(this);
	}

	SolLuaPlugin::SolLuaPlugin(sol::state_view lua_state, const Rml::String& lua_environment_identifier)
		: m_lua_state{lua_state}, m_lua_env_identifier{lua_environment_identifier}
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

		return plugins.empty() ? nullptr : plugins.back();
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
