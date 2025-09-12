// Proudly engineered by itzzexcel
// Licensed under the MIT License
// https://github.com/itzzexcel/winq-remapper

#include "hooks.hpp"

#include <filesystem>
#include <unordered_map>

std::wstring mode = L"default";
bool hoverSetting = false;
bool hoverwFocusSetting = false;
bool isDebugMode = false;
HHOOK kbHook;
bool wKeyPressed = false;
HWND lastHoverWindow = NULL;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    
    // Kill existing instances
    DWORD currentProcessId = GetCurrentProcessId();
    PROCESSENTRY32W processEntry{};
    processEntry.dwSize = sizeof(PROCESSENTRY32W);
    HANDLE processSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (processSnapshot != INVALID_HANDLE_VALUE) {
        if (Process32FirstW(processSnapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, L"winq-remapper.exe") == 0 &&
                    processEntry.th32ProcessID != currentProcessId) {
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

    // cmd argument parser
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

    if (strstr(lpCmdLine, "--mode") != NULL) {
        std::unordered_map<std::wstring, int> modeMap = {
            {L"hover", 1},
            {L"hovfocus", 2},
            {L"default", 3}
        };
        mode = wideCmdLine.substr(wideCmdLine.find(L"--mode=") + 8);
        switch (modeMap[mode]) {
            case 1: hoverSetting = true; mode = L"hover"; break;
            case 2: hoverwFocusSetting = true; mode = L"hovfocus"; break;
            default: hoverwFocusSetting = false; hoverSetting = false; mode = L"default"; break;
        }
    }

    if (strstr(lpCmdLine, "--help")) {
        print("Usage: winq-remapper.exe [--debug] [--mode <hover|hovfocus|default>] [--uninstall]");
        return 0;
    }

    // Debug console setup
    if (isDebugMode) {
        if (GetConsoleWindow() == NULL) {
            AllocConsole();
            freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
        }
        print("[DEBUG] Starting winq-remapper with hover detection...");
        print("[DEBUG] Mode: %ls", mode.c_str());
        print("[DEBUG] Hover setting: %d", hoverSetting);
        print("[DEBUG] Hover with focus setting: %d", hoverwFocusSetting);
        print("[DEBUG] Full command line: %ls", wideCmdLine.c_str());
    } else {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }

    HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"wnq/rmp", WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    RegisterStartup(hInstance, wideCmdLine);

    kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!kbHook)
        return 1;

    SetTimer(NULL, 1, 50, HoverTimerProc);

    if (isDebugMode && mode == L"hover")
        print("[DEBUG] Setup complete. Move mouse around to see hover detection.");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(NULL, 1);
    UnhookWindowsHookEx(kbHook);
    return 0;
}