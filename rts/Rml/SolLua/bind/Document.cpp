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

#include "bind.h"

#include "../plugin/SolLuaDocument.h"


namespace Rml::SolLua
{

	namespace document
	{
		auto show(SolLuaDocument& self)
		{
			self.Show();
		}

		auto showModal(SolLuaDocument& self, Rml::ModalFlag modal)
		{
			self.Show(modal);
		}

		auto showModalFocus(SolLuaDocument& self, Rml::ModalFlag modal, Rml::FocusFlag focus)
		{
			self.Show(modal, focus);
		}

		auto reloadStyleSheet(SolLuaDocument& self)
		{
			self.ReloadStyleSheet();
		}

		auto reloadStyleSheetAndLoad(SolLuaDocument& self, bool load)
		{
			reloadStyleSheet(self);
			if (load)
			{
				// Dispatch the load event so we can re-bind any scripts that got wiped out.
				self.DispatchEvent(EventId::Load, Dictionary());
			}
		}

		auto loadInlineScript3(SolLuaDocument& self, const Rml::String& content, const Rml::String& source_path, int source_line)
		{
			self.LoadInlineScript(content, source_path, source_line);
		}

		auto loadInlineScript2(SolLuaDocument& self, const Rml::String& content, const Rml::String& source_path)
		{
			loadInlineScript3(self, content, source_path, 0);
		}

		auto loadInlineScript1(SolLuaDocument& self, const Rml::String& content)
		{
			loadInlineScript3(self, content, self.GetSourceURL(), 0);
		}

		auto appendToStyleSheet(SolLuaDocument& self, const Rml::String& content)
		{
			auto styleSheet = Rml::Factory::InstanceStyleSheetString(content);
			auto combined = styleSheet->CombineStyleSheetContainer(*self.GetStyleSheetContainer());
			self.SetStyleSheetContainer(std::move(combined));
		}

		auto getWidget(SolLuaDocument& self)
		{
			return self.GetLuaEnvironment()["widget"];
		}
	}

	void bind_document(sol::table& namespace_table)
	{
		/***
		 * @enum RmlUi.RmlModalFlag 
		 */
		namespace_table.new_enum<Rml::ModalFlag>("RmlModalFlag",
			{
				/*** @field RmlUi.RmlModalFlag.None integer */
				{ "None", Rml::ModalFlag::None },
				/*** @field RmlUi.RmlModalFlag.Modal integer */
				{ "Modal", Rml::ModalFlag::Modal },
				/*** @field RmlUi.RmlModalFlag.Keep integer */
				{ "Keep", Rml::ModalFlag::Keep }
			}
		);

		/***
		 * @enum RmlUi.RmlFocusFlag 
		 */
		namespace_table.new_enum<Rml::FocusFlag>("RmlFocusFlag",
			{
				/*** @field RmlUi.RmlFocusFlag.None integer */
				{ "None", Rml::FocusFlag::None },
				/*** @field RmlUi.RmlFocusFlag.Document integer */
				{ "Document", Rml::FocusFlag::Document },
				/*** @field RmlUi.RmlFocusFlag.Keep integer */
				{ "Keep", Rml::FocusFlag::Keep },
				/*** @field RmlUi.RmlFocusFlag.Auto integer */
				{ "Auto", Rml::FocusFlag::Auto }
			}
		);

		/*** Document derives from Element.
		 *
		 * Document has no constructor; it must be instantiated through a Context object instead, either by loading an external RML file or creating an empty document. It has the following functions and properties:
		 *
		 * @class RmlUi.Document : RmlUi.Element
		 */
		
		namespace_table.new_usertype<SolLuaDocument>("Document", sol::no_constructor,
			// M
			/***
			 * Pulls the document in front of other documents within its context with a similar z-index.
			 * @function RmlUi.Document:PullToFront
			 */
			"PullToFront", &SolLuaDocument::PullToFront,
			/***
			 * Pushes the document behind other documents within its context with a similar z-index.
			 * @function RmlUi.Document:PushToBack
			 */
			"PushToBack", &SolLuaDocument::PushToBack,
			/***
			 * Shows the document.
			 * @function RmlUi.Document:Show
			 * @param modal RmlUi.RmlModalFlag? Defaults to Focus
			 * @param focus RmlUi.RmlFocusFlag?
			 */
			"Show", sol::overload(&document::show, &document::showModal, &document::showModalFocus),
			/***
			 * Hides the document.
			 * @function RmlUi.Document:Hide
			 */
			"Hide", &SolLuaDocument::Hide,
			/***
			 * Hides and closes the document, destroying its contents.
			 * @function RmlUi.Document:Close
			 */
			"Close", &SolLuaDocument::Close,
			/***
			 * Instances an element with a tag of tag_name.
			 * @function RmlUi.Document:CreateElement
			 * @param tag_name string
			 * @return RmlUi.ElementPtr
			 */
			"CreateElement", &SolLuaDocument::CreateElement,
			/***
			 * Instances a text element containing the string text.
			 * @function RmlUi.Document:CreateTextNode
			 * @param text string
			 * @return RmlUi.ElementPtr
			 */
			"CreateTextNode", &SolLuaDocument::CreateTextNode,
			//--
			/***
			 * Reload the active style sheet.
			 * @function RmlUi.Document:ReloadStyleSheet
			 * @param load boolean?
			 */
			"ReloadStyleSheet", sol::overload(&document::reloadStyleSheet, &document::reloadStyleSheetAndLoad),
			/***
			 * Load scripts as if it were in the script tag.
			 * @function RmlUi.Document:LoadInlineScript
			 * @param content string
			 * @param source string?
			 * @param source_line integer?
			 */
			"LoadInlineScript", sol::overload(&document::loadInlineScript1, &document::loadInlineScript2, &document::loadInlineScript3),
			/***
			 * Load an external script.
			 * @function RmlUi.Document:LoadExternalScript
			 * @param source_path string
			 */
			"LoadExternalScript", &SolLuaDocument::LoadExternalScript,
			/***
			 * Update the Document.
			 * @function RmlUi.Document:UpdateDocument
			 */
			"UpdateDocument", &SolLuaDocument::UpdateDocument,
			/***
			 * Append text to style sheet.
			 * @function RmlUi.Document:AppendToStyleSheet
			 * @param content string
			 */
			"AppendToStyleSheet", &document::appendToStyleSheet,

			// G+S
			/*** @field RmlUi.Document.title string */
			"title", sol::property(&SolLuaDocument::GetTitle, &SolLuaDocument::SetTitle),

			// G
			/*** @field RmlUi.Document.context RmlUi.Context */
			"context", sol::readonly_property(&SolLuaDocument::GetContext),
			//--
			/*** @field RmlUi.Document.url string */
			"url", sol::readonly_property(&SolLuaDocument::GetSourceURL),
			/*** @field RmlUi.Document.modal boolean Is it modal? */
			"modal", sol::readonly_property(&SolLuaDocument::IsModal),
			/*** @field RmlUi.Document.widget table A table of data that can be accessed in onevent attributes. It doesn't have to be a widget. */
			"widget", sol::readonly_property(&document::getWidget),

			// B
			sol::base_classes, sol::bases<Rml::Element>()
		);
	}

} // end namespace Rml::SolLua
