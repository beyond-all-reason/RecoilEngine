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
#include "../plugin/SolLuaEventListener.h"

#include <unordered_map>


namespace Rml::SolLua
{

	namespace functions
	{
		void addEventListener(Rml::Element& self, const Rml::String& event, sol::protected_function func, const bool in_capture_phase = false)
		{
			auto e = new SolLuaEventListener{ func, &self };
			self.AddEventListener(event, e, in_capture_phase);
		}

		void addEventListener(Rml::Element& self, const Rml::String& event, const Rml::String& code, sol::this_state s)
		{
			auto state = sol::state_view{ s };
			auto e = new SolLuaEventListener{ state, code, &self };
			self.AddEventListener(event, e, false);
		}

		void addEventListener(Rml::Element& self, const Rml::String& event, const Rml::String& code, sol::this_state s, const bool in_capture_phase)
		{
			auto state = sol::state_view{ s };
			auto e = new SolLuaEventListener{ state, code, &self };
			self.AddEventListener(event, e, in_capture_phase);
		}

		auto getAttribute(Rml::Element& self, const Rml::String& name, sol::this_state s)
		{
			sol::state_view state{ s };

			auto attr = self.GetAttribute(name);
			return makeObjectFromVariant(attr, s);
		}

		auto getElementsByTagName(Rml::Element& self, const Rml::String& tag)
		{
			Rml::ElementList result;
			self.GetElementsByTagName(result, tag);
			return result;
		}

		auto getElementsByClassName(Rml::Element& self, const Rml::String& class_name)
		{
			Rml::ElementList result;
			self.GetElementsByClassName(result, class_name);
			return result;
		}

		auto getAttributes(Rml::Element& self, sol::this_state s)
		{
			SolObjectMap result;

			const auto& attributes = self.GetAttributes();
			for (auto& [key, value] : attributes)
			{
				auto object = makeObjectFromVariant(&value, s);
				result.insert(std::make_pair(key, object));
			}

			return result;
		}

		auto getOwnerDocument(Rml::Element& self)
		{
			auto document = self.GetOwnerDocument();
			auto soldocument = dynamic_cast<SolLuaDocument*>(document);
			return soldocument;
		}

		auto getQuerySelectorAll(Rml::Element& self, const Rml::String& selector)
		{
			Rml::ElementList result;
			self.QuerySelectorAll(result, selector);
			return result;
		}
	}

	namespace child
	{
		auto getMaxChildren(Rml::Element& self)
		{
			std::function<int()> result = std::bind(&Rml::Element::GetNumChildren, &self, false);
			return result;
		}
	}

	namespace style
	{
		auto nextPair(sol::user<Rml::PropertiesIteratorView&> iter_state, sol::this_state s)
		{
			auto& iter = iter_state.value();

			if (iter.AtEnd())
				return std::make_tuple(sol::object(sol::lua_nil), sol::object(sol::lua_nil));

			auto result = std::make_tuple(
				sol::object(s, sol::in_place, iter.GetName()),
				sol::object(s, sol::in_place, iter.GetProperty().ToString())
			);
			++iter;

			return result;
		}

		struct StyleProxy
		{
			explicit StyleProxy(Rml::Element* element) : m_element(element) {}

			std::string Get(const std::string& name)
			{
				auto prop = m_element->GetProperty(name);
				if (prop == nullptr) return {};
				return prop->ToString();
			}

			void Set(const sol::this_state L, const std::string& name, const sol::object& value)
			{
				if (value.get_type() == sol::type::nil) {
					m_element->RemoveProperty(name);
					return;
				}

				if (value.get_type() == sol::type::string) {
					auto str = value.as<std::string&>();

					if (str.empty()) {
						m_element->RemoveProperty(name);
					} else {
						m_element->SetProperty(name, str);
					}

					return;
				}

				sol::type_error(L, sol::type::string, value.get_type());
			}

			auto Pairs()
			{
				auto iter = m_element->IterateLocalProperties();
				return std::make_tuple(
					&nextPair,
					sol::user<Rml::PropertiesIteratorView>(std::move(iter)),
					sol::lua_nil
				);
			}

		private:
			Rml::Element* m_element;
		};

		auto getElementStyleProxy(Rml::Element* self)
		{
			return StyleProxy{ self };
		}
	}

	void bind_element(sol::table& namespace_table)
	{
		/***
		 * Event listener interface
		 * @class RmlUi.EventListener
		 */
		/***
		 * @function RmlUi.EventListener.ProcessEvent
		 * @param event RmlUi.Event
		 */
		/***
		 * @function RmlUi.EventListener.OnAttach
		 * @param element RmlUi.Element
		 */
		/***
		 * @function RmlUi.EventListener.OnDetach
		 * @param element RmlUi.Element
		 */ 
		namespace_table.new_usertype<Rml::EventListener>("EventListener", sol::no_constructor,
			// M
			"OnAttach", &Rml::EventListener::OnAttach,
			"OnDetach", &Rml::EventListener::OnDetach,
			"ProcessEvent", &Rml::EventListener::ProcessEvent
		);

		///////////////////////////
		/***
		 * @alias RmlUi.StyleProxy { [string]: string }
		 */
		namespace_table.new_usertype<style::StyleProxy>("StyleProxy", sol::no_constructor,
														sol::meta_function::index, &style::StyleProxy::Set,
			sol::meta_function::new_index, &style::StyleProxy::Set,
			sol::meta_function::pairs, &style::StyleProxy::Pairs
		);

		/***  
		 * Represents an element in the RmlUi document tree. This class cannot be constructed directly; use a Document object to instantiate elements. This is the foundational piece of the DOM.
		 * @class RmlUi.Element
		 * @field attributes RmlUi.ElementAttributesProxy Read-only. Proxy for accessing element attributes.
		 * @field child_nodes RmlUi.ElementChildNodesProxy Read-only. Proxy for accessing child nodes of the element.
		 * @field class_name string Gets or sets the class names assigned to the element.
		 * @field client_height integer Read-only. The inner height of the element in pixels, including padding but not the horizontal scrollbar height, border, or margin.
		 * @field client_left integer Read-only. The width of the left border of the element in pixels.
		 * @field client_top integer Read-only. The width of the top border of the element in pixels.
		 * @field client_width integer Read-only. The inner width of the element in pixels, including padding but not the vertical scrollbar width, border, or margin.
		 * @field first_child RmlUi.Element? Read-only. The first child element, or nil if there are no children.
		 * @field id string Gets or sets the unique identifier of the element.
		 * @field inner_rml string Gets or sets the inner RML (markup) content of the element.
		 * @field last_child RmlUi.Element? Read-only. The last child element, or nil if there are no children.
		 * @field next_sibling RmlUi.Element? Read-only. The next sibling element, or nil if there is none.
		 * @field offset_height integer Read-only. The height of the element including vertical padding and borders, in pixels.
		 * @field offset_left integer Read-only. The distance from the inner left edge of the offset parent, in pixels.
		 * @field offset_parent RmlUi.Element Read-only. The closest positioned ancestor element.
		 * @field offset_top integer Read-only. The distance from the inner top edge of the offset parent, in pixels.
		 * @field offset_width integer Read-only. The width of the element including horizontal padding and borders, in pixels.
		 * @field owner_document RmlUi.Document Read-only. The document that owns this element.
		 * @field parent_node RmlUi.Element? Read-only. The parent node of this element, or nil if there is none.
		 * @field previous_sibling RmlUi.Element? Read-only. The previous sibling element, or nil if there is none.
		 * @field scroll_height integer Read-only. The total height of the element's content, including content not visible on the screen due to overflow.
		 * @field scroll_left integer Gets or sets the number of pixels that the content of the element is scrolled from the left.
		 * @field scroll_top integer Gets or sets the number of pixels that the content of the element is scrolled from the top.
		 * @field scroll_width integer Read-only. The total width of the element's content, including content not visible on the screen due to overflow.
		 * @field style RmlUi.ElementStyleProxy Read-only. Proxy for accessing and modifying the element's style properties.
		 * @field tag_name string Read-only. The tag name of the element.
		 * @field address string Read-only. The address of the element in the document tree.
		 * @field absolute_left integer Read-only. The absolute left position of the element relative to the document.
		 * @field absolute_top integer Read-only. The absolute top position of the element relative to the document.
		 * @field baseline integer Read-only. The baseline position of the element.
		 * @field line_height integer Read-only. The computed line height of the element.
		 * @field visible boolean Read-only. True if the element is visible, false otherwise.
		 * @field z_index integer Read-only. The computed z-index of the element.
		*/

		/***
		 * Adds an event listener to the element.
		 * @function RmlUi.Element:AddEventListener
		 * @param event string
		 * @param listener function|string
		 * @param in_capture_phase boolean
		 */

		/***
		 * Appends element as a child to this element.
		 * @function RmlUi.Element:AppendChild
		 * @param element RmlUi.ElementPtr
		 * @return RmlUi.ElementPtr
		 */

		/***
		 * Removes input focus from this element.
		 * @function RmlUi.Element:Blur
		 */

		/***
		 * Fakes a click on this element.
		 * @function RmlUi.Element:Click
		 */

		/***
		 * Dispatches an event to this element.
		 * @function RmlUi.Element:DispatchEvent
		 * @param event string
		 * @param parameters table
		 * @param interruptible string
		 * @return boolean
		 */

		/***
		 * Gives input focus to this element.
		 * @function RmlUi.Element:Focus
		 */

		/***
		 * Returns the value of the attribute named name. If no such attribute exists, the empty string will be returned.
		 * @function RmlUi.Element:GetAttribute
		 * @param name string
		 * @return any
		 */

		/***
		 * Returns the descendant element with an id of id.
		 * @function RmlUi.Element:GetElementById
		 * @param id string
		 * @return RmlUi.Element
		 */

		/***
		 * Returns a list of all descendant elements with the tag of tag_name.
		 * @function RmlUi.Element:GetElementsByTagName
		 * @param tag_name string
		 * @return RmlUi.ElementPtr[]
		 */

		/***
		 * Returns True if the element has a value for the attribute named name, False if not.
		 * @function RmlUi.Element:HasAttribute
		 * @param name string
		 * @return boolean
		 */

		/***
		 * Returns True if the element has at least one child node, false if not.
		 * @function RmlUi.Element:HasChildNodes
		 * @return boolean
		 */

		/***
		 * Inserts the element element as a child of this element, directly before adjacent_element in the list of children.
		 * @function RmlUi.Element:InsertBefore
		 * @param element RmlUi.ElementPtr
		 * @param adjacent_element RmlUi.Element
		 * @return RmlUi.ElementPtr
		 */

		/***
		 * Returns true if the class name is set on the element, false if not.
		 * @function RmlUi.Element:IsClassSet
		 * @param name string
		 * @return boolean
		 */

		/***
		 * Removes the attribute named name from the element.
		 * @function RmlUi.Element:RemoveAttribute
		 * @param name string
		 */

		/***
		 * Removes the child element element from this element.
		 * @function RmlUi.Element:RemoveChild
		 * @param element RmlUi.Element
		 * @return boolean
		 */

		/***
		 * Replaces the child element replaced_element with inserted_element in this element's list of children. If replaced_element is not a child of this element, inserted_element will be appended onto the list instead.
		 * @function RmlUi.Element:ReplaceChild
		 * @param inserted_element RmlUi.ElementPtr
		 * @param replaced_element RmlUi.Element
		 * @return boolean
		 */

		/***
		 * Scrolls this element into view if its ancestors have hidden overflow.
		 * @function RmlUi.Element:ScrollIntoView
		 * @param align_with_top boolean
		 */

		/***
		 * Sets the value of the attribute named name to value.
		 * @function RmlUi.Element:SetAttribute
		 * @param name string
		 * @param value string
		 */

		/***
		 * Sets (if value is true) or clears (if value is false) the class name on the element.
		 * @function RmlUi.Element:SetClass
		 * @param name string
		 * @param value boolean
		 */
		namespace_table.new_usertype<Rml::Element>("Element", sol::no_constructor,
			// M
			"AddEventListener", sol::overload(
				[](Rml::Element& s, const Rml::String& e, sol::protected_function f) { functions::addEventListener(s, e, f, false); },
				sol::resolve<void(Rml::Element&, const Rml::String&, sol::protected_function, bool)>(&functions::addEventListener),
				sol::resolve<void(Rml::Element&, const Rml::String&, const Rml::String&, sol::this_state)>(&functions::addEventListener),
				sol::resolve<void(Rml::Element&, const Rml::String&, const Rml::String&, sol::this_state, bool)>(&functions::addEventListener)
			),
			"AppendChild", [](Rml::Element& self, Rml::ElementPtr& e) {
				return self.AppendChild(std::move(e));
			},
			"Blur", &Rml::Element::Blur,
			"Click", &Rml::Element::Click,
			"DispatchEvent", sol::resolve<bool(const Rml::String&, const Rml::Dictionary&)>(&Rml::Element::DispatchEvent),
			"Focus", &Rml::Element::Focus,
			"GetAttribute", &functions::getAttribute,
			"GetElementById", &Rml::Element::GetElementById,
			"GetElementsByTagName", &functions::getElementsByTagName,
			"QuerySelector", &Rml::Element::QuerySelector,
			"QuerySelectorAll", &functions::getQuerySelectorAll,
			"HasAttribute", &Rml::Element::HasAttribute,
			"HasChildNodes", &Rml::Element::HasChildNodes,
			"InsertBefore", [](Rml::Element& self, Rml::ElementPtr& element, Rml::Element* adjacent_element) {
				return self.InsertBefore(std::move(element), adjacent_element);
			},
			"IsClassSet", &Rml::Element::IsClassSet,
			"RemoveAttribute", &Rml::Element::RemoveAttribute,
			"RemoveChild", &Rml::Element::RemoveChild,
			"ReplaceChild", [](Rml::Element& self, Rml::ElementPtr& inserted_element, Rml::Element* replaced_element) {
				return self.ReplaceChild(std::move(inserted_element), replaced_element);
			},
			"ScrollIntoView", [](Rml::Element& self, sol::variadic_args va) {
				if (va.size() == 0)
					self.ScrollIntoView(true);
				else
					self.ScrollIntoView(va[0].as<bool>());
			},
			"SetAttribute", static_cast<void(Rml::Element::*)(const Rml::String&, const Rml::String&)>(&Rml::Element::SetAttribute),
			"SetClass", &Rml::Element::SetClass,
			//--
			"GetElementsByClassName", &functions::getElementsByClassName,
			"Clone", &Rml::Element::Clone,
			"Closest", &Rml::Element::Closest,
			"SetPseudoClass", &Rml::Element::SetPseudoClass,
			"IsPseudoClassSet", &Rml::Element::IsPseudoClassSet,
			"ArePseudoClassesSet", &Rml::Element::ArePseudoClassesSet,
			"GetActivePseudoClasses", &Rml::Element::GetActivePseudoClasses,
			"IsPointWithinElement", &Rml::Element::IsPointWithinElement,
			"ProcessDefaultAction", &Rml::Element::ProcessDefaultAction,
			"GetValue",[](Rml::Element& self) {
				if (self.GetTagName() == "input") {
					return dynamic_cast<Rml::ElementFormControlInput*>(&self)->GetValue();
				} else if (self.GetTagName() == "textarea") {
					return dynamic_cast<Rml::ElementFormControlTextArea*>(&self)->GetValue();
				}
				return std::string();
			},
			"GetChild", [](Rml::Element& self, int index) { return self.GetChild(index); },

			// G+S
			"class_name", sol::property(&Rml::Element::GetClassNames, &Rml::Element::SetClassNames),
			"id", sol::property(&Rml::Element::GetId, &Rml::Element::SetId),
			"inner_rml", sol::property(sol::resolve<Rml::String() const>(&Rml::Element::GetInnerRML), &Rml::Element::SetInnerRML),
			"scroll_left", sol::property(&Rml::Element::GetScrollLeft, &Rml::Element::SetScrollLeft),
			"scroll_top", sol::property(&Rml::Element::GetScrollTop, &Rml::Element::SetScrollTop),

			// G
			"attributes", sol::readonly_property(&functions::getAttributes),
			"child_nodes", sol::readonly_property(&getIndexedTable<Rml::Element, Rml::Element, &Rml::Element::GetChild, &child::getMaxChildren>),
			"client_left", sol::readonly_property(&Rml::Element::GetClientLeft),
			"client_height", sol::readonly_property(&Rml::Element::GetClientHeight),
			"client_top", sol::readonly_property(&Rml::Element::GetClientTop),
			"client_width", sol::readonly_property(&Rml::Element::GetClientWidth),
			"first_child", sol::readonly_property(&Rml::Element::GetFirstChild),
			"last_child", sol::readonly_property(&Rml::Element::GetLastChild),
			"next_sibling", sol::readonly_property(&Rml::Element::GetNextSibling),
			"offset_height", sol::readonly_property(&Rml::Element::GetOffsetHeight),
			"offset_left", sol::readonly_property(&Rml::Element::GetOffsetLeft),
			"offset_parent", sol::readonly_property(&Rml::Element::GetOffsetParent),
			"offset_top", sol::readonly_property(&Rml::Element::GetOffsetTop),
			"offset_width", sol::readonly_property(&Rml::Element::GetOffsetWidth),
			"owner_document", sol::readonly_property(&functions::getOwnerDocument),
			"parent_node", sol::readonly_property(&Rml::Element::GetParentNode),
			"previous_sibling", sol::readonly_property(&Rml::Element::GetPreviousSibling),
			"scroll_height", sol::readonly_property(&Rml::Element::GetScrollHeight),
			"scroll_width", sol::readonly_property(&Rml::Element::GetScrollWidth),
			"style", sol::readonly_property(&style::getElementStyleProxy),
			"tag_name", sol::readonly_property(&Rml::Element::GetTagName),
			//--
			"address", sol::readonly_property([](Rml::Element& self) { return self.GetAddress(); }),
			"absolute_left", sol::readonly_property(&Rml::Element::GetAbsoluteLeft),
			"absolute_top", sol::readonly_property(&Rml::Element::GetAbsoluteTop),
			"baseline", sol::readonly_property(&Rml::Element::GetBaseline),
			"line_height", sol::readonly_property(&Rml::Element::GetLineHeight),
			"visible", sol::readonly_property(&Rml::Element::IsVisible),
			"z_index", sol::readonly_property(&Rml::Element::GetZIndex)
		);

	}

} // end namespace Rml::SolLua
