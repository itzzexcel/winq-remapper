#include <Windows.h>
#include <string>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <tlhelp32.h>

// Proudly engineered by itzzexcel
// Licensed under the MIT License
// https://github.com/itzzexcel/winq-remapper

#define print(...)       \
printf(__VA_ARGS__); \
printf("\n");

std::wstring mode = L"default";

bool hoverSetting = false;
bool hoverwFocusSetting = false;

HHOOK kbHook;
boolean wKeyPressed = false;
HWND lastHoverWindow = NULL;
bool isDebugMode = false;

static HWND GetCurrentMouseHoverWindow()
{
	if (hoverSetting == true || hoverwFocusSetting == true)
	{	
		POINT pt;
		GetCursorPos(&pt);
		HWND hwnd = WindowFromPoint(pt);
		if (hwnd)
		{
			HWND mainWindow = hwnd;
			HWND parent;
			
			while ((parent = GetParent(mainWindow)) != NULL)
			mainWindow = parent;
			
			if (mainWindow == hwnd)
			{
				HWND owner = GetWindow(hwnd, GW_OWNER);
				if (owner != NULL)
				mainWindow = owner;
			}
			
			return mainWindow;
		}
	} else {
		HWND hwnd = GetForegroundWindow();
		if (hwnd)
		return hwnd;
	}
	
	return NULL;
}

DWORD GetForegroundLockTimeout() {
	DWORD timeout = 0;
	SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &timeout, 0);
	return timeout;
}

void SetForegroundWindowForced(HWND hWnd) {
	DWORD originalTimeout = GetForegroundLockTimeout();
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, SPIF_SENDCHANGE);
	SetForegroundWindow(hWnd);
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)(ULONG_PTR)originalTimeout, SPIF_SENDCHANGE);
}

void CheckHoverWindowChange()
{
	HWND currentHover = GetCurrentMouseHoverWindow();
	
	if (currentHover != lastHoverWindow)
	{
		if (currentHover)
		{
			char windowTitle[256] = {0};
			char className[256] = {0};
			GetWindowTextA(currentHover, windowTitle, 255);
			GetClassNameA(currentHover, className, 255);
			
			if (hoverwFocusSetting) {
				LONG style = GetWindowLong(currentHover, GWL_STYLE);
				RECT windowRect;
				GetWindowRect(currentHover, &windowRect);
				
				if ((style & WS_VISIBLE) && 
				!(style & WS_MINIMIZE)) {
					POINT cursorPos;
					GetCursorPos(&cursorPos);
					if (PtInRect(&windowRect, cursorPos)) {
						// I'm not quite sure of how to implement this... hmmm...
						// I would add a delay for like, 1 second??? but idrk
						
						// Hacky
						HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, [](int code, WPARAM wp, LPARAM lp) -> LRESULT {
							return CallNextHookEx(NULL, code, wp, lp);
						}, GetModuleHandle(NULL), 0);
						
						SetForegroundWindow(currentHover);
						UnhookWindowsHookEx(hook);
					}
				}
			}
			
			if (isDebugMode)
			print("[DEBUG] Hover changed to HWND: 0x%p | Class: %s | Title: %s", currentHover, className, windowTitle);
		}
		else
		{
			if (isDebugMode)
			print("[DEBUG] Hover changed to NULL");
		}
		
		lastHoverWindow = currentHover;
	}
}

VOID CALLBACK HoverTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	CheckHoverWindowChange();
}

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
			HWND hwnd = GetCurrentMouseHoverWindow();
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
	// Get the current processes and if find anyone called winq-remapper kill it
	DWORD currentProcessId = GetCurrentProcessId();
	PROCESSENTRY32W processEntry{};
	processEntry.dwSize = sizeof(PROCESSENTRY32W);
	HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (processSnapshot != INVALID_HANDLE_VALUE)
	{
		if (Process32FirstW(processSnapshot, &processEntry))
		{
			do
			{
				if (_wcsicmp(processEntry.szExeFile, L"winq-remapper.exe") == 0 && 
				processEntry.th32ProcessID != currentProcessId)
				{
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processEntry.th32ProcessID);
					if (hProcess != nullptr) {
						TerminateProcess(hProcess, 0);
						CloseHandle(hProcess);
					}
				}
			} while (Process32NextW(processSnapshot, &processEntry));
		}
		CloseHandle(processSnapshot);
	}
	
	std::wstring wideCmdLine = GetCommandLineW();
	size_t firstSpace = wideCmdLine.find(L' ');
	if (firstSpace != std::wstring::npos) {
		wideCmdLine = wideCmdLine.substr(firstSpace + 1);
	} else {
		wideCmdLine = L"";
	}
	
	if (strstr(lpCmdLine, "--uninstall") != NULL)
	return Uninstall() ? 0 : 1;
	if (strstr(lpCmdLine, "--debug") != NULL)
	isDebugMode = true;
	if (strstr(lpCmdLine, "--mode") != NULL)
	{
		std::unordered_map<std::wstring, int> modeMap = {
			{L"hover", 1},
			{L"hovfocus", 2},
			{L"default", 3}
		};
		
		mode = wideCmdLine.substr(wideCmdLine.find(L"--mode=") + 0x08);
		
		switch (modeMap[mode]) {
			case 1:
			hoverSetting = true;
			mode = L"hover";
			break;
			case 2:
			hoverwFocusSetting = true;
			mode = L"hovfocus";
			break;
			case 3:
			default:
			hoverwFocusSetting = false;
			hoverSetting = false;
			mode = L"default";
			break;
		}
	}
	if (strstr(lpCmdLine, "--help"))
	{
		print("Usage: winq-remapper.exe [--debug] [--mode <hover|hovfocus|default>] [--uninstall]");
		return 0;
	}
	
	
	if (isDebugMode)
	{
		if (GetConsoleWindow() == NULL)
		{
			AllocConsole();
			freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
		}
		print("[DEBUG] Starting winq-remapper with hover detection...");
		print("[DEBUG] Mode: %ls", mode.c_str());
		print("[DEBUG] Hover setting: %d", hoverSetting);
		print("[DEBUG] Hover with focus setting: %d", hoverwFocusSetting);
		print("[DEBUG] Full command line: %ls", wideCmdLine.c_str());
	}
	else
	ShowWindow(GetConsoleWindow(), SW_HIDE);
	
	HWND hwnd = CreateWindowExW(
		WS_EX_TOOLWINDOW,
		L"STATIC",
		L"winqremapperwindow",
		WS_OVERLAPPED,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);
		
		HKEY hKey;
		RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
		
		std::wstring exePath = std::filesystem::absolute(std::filesystem::path(__argv[0])).wstring();
		std::wstring cmdLine = L"\"" + exePath + L"\" " + wideCmdLine;
		
		print("[DEBUG] Command line: %ls", cmdLine.c_str());
		
		if (RegSetValueExW(hKey, L"winqremapper", 0, REG_SZ, (BYTE*)cmdLine.c_str(), (cmdLine.size() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS)
		{
			print("[DEBUG] Registry key set successfully.");
		}
		else
		{
			print("[DEBUG] Failed to set registry key.");
		}
		
		RegCloseKey(hKey);
		
		kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
		if (!kbHook)
		return 1;
		
		SetTimer(NULL, 1, 50, HoverTimerProc);
		
		if (isDebugMode && mode == L"hover")
		print("[DEBUG] Setup complete. Move mouse around to see hover detection.");
		
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		
		KillTimer(NULL, 1);
		UnhookWindowsHookEx(kbHook);
		return 0;
	}