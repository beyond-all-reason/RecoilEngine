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

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFOEX cfie = { 0 };
	cfie.cbSize = sizeof(cfie);
	lstrcpyW(cfie.FaceName, L"Lucida Console");
	cfie.dwFontSize.Y = 13;
	SetCurrentConsoleFontEx(hStdOut, FALSE, &cfie);
#endif // _WIN32
}
