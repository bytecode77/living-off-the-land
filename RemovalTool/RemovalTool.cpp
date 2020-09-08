#include <Windows.h>
#include "../Global/NativeRegistry.h"

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	LPCSTR error = NULL;
	BOOL removed = FALSE;

	// Delete registry value HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\(Default), where Injector.exe is stored.
	HKEY key;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer", 0, KEY_ALL_ACCESS, &key) == ERROR_SUCCESS)
	{
		DWORD size;
		LSTATUS result = RegQueryValueExA(key, NULL, 0, NULL, NULL, &size);
		if (result == ERROR_FILE_NOT_FOUND)
		{
		}
		else if (result == ERROR_SUCCESS)
		{
			if (size >= 2)
			{
				LPBYTE buffer = new BYTE[size];
				if (RegQueryValueExA(key, NULL, 0, NULL, buffer, &size) == ERROR_SUCCESS)
				{
					if (buffer[0] == 'M' && buffer[1] == 'Z')
					{
						if (RegDeleteValueA(key, NULL) == ERROR_SUCCESS)
						{
							removed = TRUE;
						}
						else
						{
							error = "Could not delete registry value 'HKEY_CURRENT_USER\\Software\\Microsoft\\Internet Explorer\\(Default)'.";
						}
					}
				}
				else
				{
					error = "Could not read registry value 'HKEY_CURRENT_USER\\Software\\Microsoft\\Internet Explorer\\(Default)'.";
				}
			}
		}
		else
		{
			error = "Could not query registry value 'HKEY_CURRENT_USER\\Software\\Microsoft\\Internet Explorer\\(Default)'.";
		}
	}
	else
	{
		error = "Could not open registry key 'HKEY_CURRENT_USER\\Software\\Microsoft\\Internet Explorer'.";
	}

	if (!error)
	{
		// Delete registry value HKCU\...\Run\{NULL}X, where the startup script is stored.
		// This is a null embedded registry value. Only the native API can delete it.
		nt_cpp::Udc path = nt_cpp::GetCurrentUserPath() + L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\\0X";
		if (nt_cpp::QueryValue(path).type != 0)
		{
			if (nt_cpp::DeleteValue(path))
			{
				removed = TRUE;
			}
			else
			{
				error = "Could not delete registry value 'HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\{NULL}X'.";
			}
		}
	}

	if (error != NULL) MessageBoxA(NULL, error, "Living Off The Land - Removal Tool", MB_OK | MB_ICONERROR);
	else if (removed) MessageBoxA(NULL, "Removed successfully.", "Living Off The Land - Removal Tool", MB_OK | MB_ICONASTERISK);
	else MessageBoxA(NULL, "Nothing to remove.", "Living Off The Land - Removal Tool", MB_OK | MB_ICONASTERISK);
}