#include <Windows.h>
#include <string>
#include <filesystem>

// Proudly engineered by itzzexcel
// Licensed under the MIT License
// https://github.com/itzzexcel/winq-remapper

HHOOK kbHook;
boolean wKeyPressed = false;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		KBDLLHOOKSTRUCT *kbStruct = (KBDLLHOOKSTRUCT *)lParam;

		if (kbStruct->vkCode == VK_LWIN || kbStruct->vkCode == VK_RWIN)
		{
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
				wKeyPressed = true;
			else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
				wKeyPressed = false;
		}

		if ((kbStruct->vkCode == 'Q' || kbStruct->vkCode == 'q') &&
			(wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) &&
			wKeyPressed)
		{
			HWND hwnd = GetForegroundWindow();
			if (hwnd)
				PostMessage(hwnd, WM_CLOSE, 0, 0);

			keybd_event(VK_LWIN, 0, 0, 0);
			keybd_event(VK_BACK, 0, 0, 0);

			wKeyPressed = false;

			return 1;
		}
	}

	return CallNextHookEx(kbHook, nCode, wParam, lParam);
}

bool Uninstall()
{
	HKEY hKey;
	bool success = false;

	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
	{
		if (RegDeleteValueW(hKey, L"winqremapper") == ERROR_SUCCESS)
			success = true;
		RegCloseKey(hKey);
	}

	if (success)
		MessageBoxW(NULL, L"Removed from startup.", L"Uninstall Complete", MB_OK | MB_ICONINFORMATION);
	else
		MessageBoxW(NULL, L"Failed to remove from startup.", L"Uninstall Failed", MB_OK | MB_ICONERROR);

	return success;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if (strstr(lpCmdLine, "--uninstall") != NULL)
		return Uninstall() ? 0 : 1;

	ShowWindow(GetConsoleWindow(), SW_HIDE);

	HWND hwnd = CreateWindowExW(
		WS_EX_TOOLWINDOW,
		L"STATIC",
		L"winqremapperwindow",
		WS_OVERLAPPED,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);

	HKEY hKey;
	std::wstring exePath = std::filesystem::absolute(std::filesystem::path(__argv[0])).wstring();
	RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
	RegSetValueExW(hKey, L"winqremapper", 0, REG_SZ, (BYTE *)exePath.c_str(), (exePath.size() + 1) * sizeof(wchar_t));
	RegCloseKey(hKey);

	kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
	if (!kbHook)
		return 1;

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(kbHook);
	return 0;
}