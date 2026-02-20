#include "ConsoleInit.hpp"

#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
	#include <windows.h>
#endif

void Recoil::InitConsole()
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif // _WIN32
}
