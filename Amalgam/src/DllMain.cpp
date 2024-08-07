#include <Windows.h>
#include "Core/Core.h"
#include "Utils/Minidump/Minidump.h"

DWORD WINAPI MainThread(LPVOID lpParam)
{
	while (!GetModuleHandleA("GameUI.dll"))
		Sleep(2000);

	U::Core.Load();

	while (!U::Core.ShouldUnload())
		Sleep(250);

	U::Core.Unload();

	FreeLibraryAndExitThread((HMODULE)lpParam, EXIT_SUCCESS);
}

INT WINAPI DllMain(HMODULE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
#ifndef _DEBUG
		SetUnhandledExceptionFilter(Minidump::ExceptionFilter);
#endif // _DEBUG

		if(const auto hMainThread = CreateThread(nullptr, 0, MainThread, hinstDLL, 0, nullptr))
			CloseHandle(hMainThread);
	}

	return TRUE;
}