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

#include "SolLuaDataModel.h"
#include "Rml/SolLua/bind/bind.h"
#include "sol2/sol.hpp"

#include <optional>
#include <string>


namespace Rml::SolLua
{
	SolLuaDataModel::SolLuaDataModel(sol::state_view s) : Lua{ s } {}

	SolLuaObjectDef::SolLuaObjectDef(SolLuaDataModel* model)
		: VariableDefinition(DataVariableType::Scalar), m_model(model)
	{
	}

	DataVariableReference::DataVariableReference(sol::table parent, std::string key, std::string pkc, int arrayindex)
		: key(key), parent(parent), arrayindex(arrayindex)
	{
		if (arrayindex > -1) {
			keychain = pkc + "$#$" + key;
		} else {
			keychain = pkc + "$$$" + key;
		}
	}

	sol::object DataVariableReference::getObject()
	{
		sol::object object = this->parent.raw_get<sol::object>(this->key);
		if (this->arrayindex > -1) {
			object = this->parent.raw_get<sol::object>(this->arrayindex);
		}
		return object;
	}

	bool SolLuaObjectDef::Get(void* ptr, Rml::Variant& variant)
	{
		auto dv = static_cast<DataVariableReference*>(ptr);
		sol::object obj = dv->getObject();

		switch (obj.get_type()) {
			case sol::type::boolean:
				variant = obj.as<bool>();
				break;
			case sol::type::string:
				variant = obj.as<std::string>();
				break;
			case sol::type::number:
				variant = obj.as<lua_Number>();
				break;
			case sol::type::userdata:
				if (obj.is<Rml::Vector2i>())
					variant = obj.as<Vector2i>();
				else if (obj.is<Rml::Vector2f>())
					variant = obj.as<Vector2f>();
				else if (obj.is<Rml::Colourb>())
					variant = obj.as<Rml::Colourb>();
				else if (obj.is<Rml::Colourf>())
					variant = obj.as<Rml::Colourf>();
				else
					variant = Rml::Variant{};
				break;
			default: // sol::type::lua_nil or other unhandled types
				variant = Rml::Variant{};
		}

		return true;
	}

	bool SolLuaObjectDef::Set(void* ptr, const Rml::Variant& variant)
	{
		auto dv = static_cast<DataVariableReference*>(ptr);
		auto v = makeObjectFromVariant(&variant, m_model->Lua);
		if (dv->arrayindex > -1) {
			dv->parent.raw_set(dv->arrayindex, v);
		} else {
			dv->parent.raw_set(dv->key, v);
		}

		return true;
	}

	int SolLuaObjectDef::Size(void* ptr)
	{
		auto dv = static_cast<DataVariableReference*>(ptr);
		sol::object obj = dv->getObject();

		// Non-table types have no children to iterate over.
		if (obj.get_type() != sol::type::table)
			return 0;

		auto t = obj.as<sol::table>();
		return static_cast<int>(t.size());
	}

	DataVariable SolLuaObjectDef::Child(void* ptr, const Rml::DataAddressEntry& address)
	{
		// Child must be called on a table.
		DataVariableReference* dvr = static_cast<DataVariableReference*>(ptr);
		auto object = dvr->getObject();

		if (object.get_type() != sol::type::table) {
			return DataVariable{ m_model->ObjectDef.get(), ptr };
		}

		// Get our table object.
		// Get the pointer as a string for use with holding onto the object.
		sol::table table = object.as<sol::table>();

		// Accessing by name.
		if (address.index == -1)
		{
			// Try to get the object.
			auto e = table.raw_get<sol::object>(address.name);
			if (e.get_type() == sol::type::lua_nil) {
				return DataVariable{ m_model->ObjectDef.get(), ptr };
			}

			// Hold a reference to it and return the pointer.
			auto cdvr = DataVariableReference(table, address.name, dvr->keychain);
			auto it = m_model->ObjectMap.insert_or_assign(cdvr.keychain, cdvr);
			return DataVariable{ m_model->ObjectDef.get(), &(it.first->second) };
		}
		// Accessing by index.
		else
		{
			// See if we have a key with the index.
			auto has_index = table.raw_get<sol::object>(address.index+1);
			if (has_index.get_type() != sol::type::lua_nil)	{
				auto cdvr = DataVariableReference(table, std::to_string(address.index+1), dvr->keychain, address.index + 1);
				auto it = m_model->ObjectMap.insert_or_assign(cdvr.keychain, cdvr);
				return DataVariable{ m_model->ObjectDef.get(), &(it.first->second) };
			}

			// Iterate through the entries and grab the nth entry.
			int idx = 1;
			for (auto& [k, v] : table.pairs()) {
				if (idx == address.index+1)
				{
					auto cdvr = DataVariableReference(table, std::to_string(idx), dvr->keychain, idx);
					auto it = m_model->ObjectMap.insert_or_assign(cdvr.keychain, cdvr);
					return DataVariable{ m_model->ObjectDef.get(), &(it.first->second) };
				}
				++idx;
			}

			// Index out of range.
			return DataVariable{};
		}

		// Failure.
		return DataVariable{};
	}

} // end namespace Rml::SolLua
