#include <Windows.h>

// This last stage is the actual payload. It is be injected by the Injector (RunPE).

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MessageBoxA
	(
		NULL,
		"Living Off The Land - Demo Payload\n"
		"\n"
		"This is a native executable, displaying a MessageBox.\n"
		"It is executed after user logon. Persistence is achieved without any files. Only two registry values are required, one of which is not visible due to embedded null characters.\n"
		"\n"
		"In addition, this process is created using the process hollowing technique (RunPE).\n"
		"\n"
		"\n"
		"Read more:\n"
		"https://bytecode77.com/living-off-the-land\n"
		"https://github.com/bytecode77/living-off-the-land",
		"Living Off The Land",
		MB_OK | MB_ICONASTERISK
	);
	return 0;
}