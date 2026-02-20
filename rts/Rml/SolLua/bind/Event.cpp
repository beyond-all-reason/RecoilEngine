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


namespace Rml::SolLua
{

	namespace functions
	{
		SolObjectMap getParameters(Rml::Event& self, sol::this_state s)
		{
			SolObjectMap result;

			const auto& parameters = self.GetParameters();
			for (auto& [key, value] : parameters)
			{
				if (self.GetId() == Rml::EventId::Tabchange && value.GetType() == Rml::Variant::INT)
				{
					auto object = sol::make_object(s, value.Get<int>());
					result.insert(std::make_pair(key, object));
				}
				else
				{
					auto object = makeObjectFromVariant(&value, s);
					result.insert(std::make_pair(key, object));
				}
			}

			return result;
		}
	}

	void bind_event(sol::table& namespace_table)
	{
		//--
		/***
		 * @enum RmlUi.RmlEventPhase
		 */
		namespace_table.new_enum("RmlEventPhase",
			/*** @field RmlUi.RmlEventPhase.None integer */
			"None", Rml::EventPhase::None,
			/*** @field RmlUi.RmlEventPhase.Capture integer */
			"Capture", Rml::EventPhase::Capture,
			/*** @field RmlUi.RmlEventPhase.Target integer */
			"Target", Rml::EventPhase::Target,
			/*** @field RmlUi.RmlEventPhase.Bubble integer */
			"Bubble", Rml::EventPhase::Bubble
		);

		/***
		 * @alias RmlUi.EventParametersProxy.MouseButton
		 * | 0 # Left
		 * | 1 # Right
		 * | 2 # Middle
		 */

		/***
		 * RmlUi uses 0 and 1 to represent boolean values here.
		 * @alias RmlUi.EventParametersProxy.TrueFalse
		 * | 0 # False
		 * | 1 # True
		 */

		/***
		 * Keyboard and mouse input data associated with an event.
		 * @class RmlUi.EventParametersProxy
		 * @x_helper
		 * @field button RmlUi.EventParametersProxy.MouseButton
		 * @field mouse_x integer
		 * @field mouse_y integer
		 * @field scroll_lock_key RmlUi.EventParametersProxy.TrueFalse
		 * @field meta_key RmlUi.EventParametersProxy.TrueFalse
		 * @field ctrl_key RmlUi.EventParametersProxy.TrueFalse
		 * @field shift_key RmlUi.EventParametersProxy.TrueFalse
		 * @field caps_lock_key RmlUi.EventParametersProxy.TrueFalse
		 * @field alt_key RmlUi.EventParametersProxy.TrueFalse
		 */

		/***
		 * An event that happens to an element.
		 * @class RmlUi.Event
		 */

		namespace_table.new_usertype<Rml::Event>("Event", sol::no_constructor,
			// M
			/***
			 * function RmlUi.Event:StopPropogation
			 */
			"StopPropagation", &Rml::Event::StopPropagation,
			//--
			/***
			 * function RmlUi.Event:StopImmediatePropogation
			 */
			"StopImmediatePropagation", &Rml::Event::StopImmediatePropagation,

			// G+S

			// G
			/*** @field RmlUi.Event.current_element RmlUi.Element */
			"current_element", sol::readonly_property(&Rml::Event::GetCurrentElement),
			/*** @field RmlUi.Event.type string */
			"type", sol::readonly_property(&Rml::Event::GetType),
			/*** @field RmlUi.Event.target_element RmlUi.Element What element it happened to */
			"target_element", sol::readonly_property(&Rml::Event::GetTargetElement),
			/*** @field RmlUi.Event.parameters RmlUi.EventParametersProxy */
			"parameters", sol::readonly_property(&functions::getParameters),
			//--
			/*** @field RmlUi.Event.event_phase RmlUi.RmlEventPhase */
			"event_phase", sol::readonly_property(&Rml::Event::GetPhase),
			/*** @field RmlUi.Event.interruptible boolean */
			"interruptible", sol::readonly_property(&Rml::Event::IsInterruptible),
			/*** @field RmlUi.Event.propagating boolean */
			"propagating", sol::readonly_property(&Rml::Event::IsPropagating),
			/*** @field RmlUi.Event.immediate_propagating boolean */
			"immediate_propagating", sol::readonly_property(&Rml::Event::IsImmediatePropagating)
		);
	}

} // end namespace Rml::SolLua
