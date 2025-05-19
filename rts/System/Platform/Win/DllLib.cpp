/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief Windows shared library loader implementation
 * Windows Shared Object loader class implementation
 */

#include "DllLib.h"
#include <nowide/convert.hpp>

/**
 * Attempts to LoadLibrary on the given DLL
 */
DllLib::DllLib(const char* fileName)
	: dll(nullptr)
{
	dll = LoadLibrary(nowide::widen(fileName).c_str());
}

/**
 * Does a FreeLibrary on the given DLL
 */
void DllLib::Unload() {

	FreeLibrary(dll);
	dll = nullptr;
}

bool DllLib::LoadFailed() {
	return dll == nullptr;
}

/**
 * Does a FreeLibrary on the given DLL
 */
DllLib::~DllLib()
{
	Unload();
}

/**
 * Attempts to locate the given symbol with GetProcAddress
 */
void* DllLib::FindAddress(const char* symbol)
{
	return reinterpret_cast<void*>(GetProcAddress(dll,symbol));
}
