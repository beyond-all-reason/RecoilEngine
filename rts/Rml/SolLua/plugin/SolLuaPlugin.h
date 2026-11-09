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

#pragma once

#include "Rml/Backends/RmlUi_SystemInterface.h"
#include "Rml/SolLua/TranslationTable.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Platform.h>
#include <RmlUi/Core/Plugin.h>
#include <RmlUi/Lua/Header.h>

#include <sol2/sol.hpp>

#include <memory>


namespace Rml::SolLua
{

	class SolLuaDocumentElementInstancer;
	class SolLuaEventListenerInstancer;

	class RMLUILUA_API SolLuaPlugin : public Plugin
	{
	public:
		SolLuaPlugin(sol::state_view lua_state);
		SolLuaPlugin(sol::state_view lua_state, const Rml::String& lua_environment_identifier);
		~SolLuaPlugin() override;

		void RemoveLuaItems();
		void AddContextTracking(Context* context);
		void AddDocumentTracking(ElementDocument* document);

		lua_State* GetLuaState() const { return m_lua_state.lua_state(); }
		sol::state_view GetStateView() const { return m_lua_state; }
		const Rml::String& GetEnvironmentIdentifier() const { return m_lua_env_identifier; }

		/// The plugin that belongs to the given Lua state, or nullptr.
		static SolLuaPlugin* ForState(lua_State* L);

		/// The plugin whose Lua state is currently calling into RmlUi (see ScopedActive), or nullptr.
		static SolLuaPlugin* Active();

		/// Picks the plugin for something being instanced by RmlUi (a document, an inline event
		/// listener): the active one, else the one that owns the hint element's document, else the
		/// most recently registered one.
		static SolLuaPlugin* Resolve(Rml::Element* hint);

		/// Marks a plugin as the one whose Lua state is calling into RmlUi for the lifetime of
		/// the object. RmlUi's factories are global, so they cannot tell which Lua state asked
		/// for a document or listener to be created unless the caller says so.
		class ScopedActive
		{
		public:
			explicit ScopedActive(SolLuaPlugin* plugin);
			explicit ScopedActive(lua_State* L);
			~ScopedActive();
			ScopedActive(const ScopedActive&) = delete;
			ScopedActive& operator=(const ScopedActive&) = delete;
		private:
			SolLuaPlugin* previous;
		};

		TranslationTable translationTable;
		RmlSystemInterface* systemInterface;
	private:
		int GetEventClasses() override;

		void OnInitialise() override;
		void OnShutdown() override;

		void OnContextDestroy(Context* context) override;
		void OnDocumentUnload(ElementDocument* document) override;

		sol::state_view m_lua_state;
		Rml::String m_lua_env_identifier;

		std::vector<Context*> luaContexts;
		std::vector<ElementDocument*> luaDocuments;
	};

} // end namespace Rml::SolLua
