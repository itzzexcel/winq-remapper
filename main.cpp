// Proudly engineered by itzzexcel
// Under the MIT License
// https://github.com/itzzexcel/winq-remapper

#include <filesystem>
#include <unordered_map>
#include <wctype.h>

#include "hooks.hpp"

std::wstring mode = L"default";

bool hoverSetting = false;
bool hoverwFocusSetting = false;
bool isDebugMode = false;
bool wKeyPressed = false;

HHOOK kbHook;
HWND lastHoverWindow = NULL;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    // Kill existing instances
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
                if (_wcsicmp(processEntry.szExeFile, L"winq-remapper.exe") == 0 && processEntry.th32ProcessID != currentProcessId ||
                    _wcsicmp(processEntry.szExeFile, L"wnq-rmp.exe") == 0 && processEntry.th32ProcessID != currentProcessId)
                {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processEntry.th32ProcessID);
                    if (hProcess != nullptr)
                    {
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
    if (firstSpace != std::wstring::npos)
    {
        wideCmdLine = wideCmdLine.substr(firstSpace + 1);
    }
    else
    {
        wideCmdLine = L"";
    }

    // Help arg
    if (wideCmdLine.find(L"--help") != std::wstring::npos)
    {
        AttachToConsole();

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE);

        print("Usage: winq-remapper.exe [--debug] [--mode <hover|hovfocus|default>] [--uninstall]");

        SetConsoleTextAttribute(hConsole, 7);
        return 0;
    }

    if (wideCmdLine.find(L"--attach") != std::wstring::npos)
        AttachToConsole();

    // Uninstall arg
    if (wideCmdLine.find(L"--uninstall") != std::wstring::npos)
        return Uninstall() ? 0 : 1;

    // Debug arg
    if (wideCmdLine.find(L"--debug") != std::wstring::npos)
        isDebugMode = true;

    // Mode arg
    if (wideCmdLine.find(L"--mode") != std::wstring::npos)
    {
        std::wstring modeValue = L"default";

        // Check for --mode=value
        size_t modePos = wideCmdLine.find(L"--mode=");
        if (modePos != std::wstring::npos)
        {
            size_t valueStart = modePos + 7; // Length of "--mode="
            size_t valueEnd = wideCmdLine.find(L' ', valueStart);
            if (valueEnd == std::wstring::npos)
            {
                valueEnd = wideCmdLine.length();
            }
            modeValue = wideCmdLine.substr(valueStart, valueEnd - valueStart);
        }
        else
        {
            // Check for --mode value format
            modePos = wideCmdLine.find(L"--mode");
            if (modePos != std::wstring::npos)
            {
                size_t valueStart = modePos + 6; // Length of "--mode"
                // Skip whitespace
                while (valueStart < wideCmdLine.length() && iswspace(wideCmdLine[valueStart]))
                {
                    valueStart++;
                }
                if (valueStart < wideCmdLine.length())
                {
                    size_t valueEnd = wideCmdLine.find(L' ', valueStart);
                    if (valueEnd == std::wstring::npos)
                        valueEnd = wideCmdLine.length();
                    modeValue = wideCmdLine.substr(valueStart, valueEnd - valueStart);
                }
            }
        }

        // Apply mode settings
        if (modeValue == L"hover")
        {
            hoverSetting = true;
            hoverwFocusSetting = false;
            mode = L"hover";
        }
        else if (modeValue == L"hovfocus")
        {
            hoverSetting = false;
            hoverwFocusSetting = true;
            mode = L"hovfocus";
        }
        else
        {
            hoverSetting = false;
            hoverwFocusSetting = false;
            mode = L"default";
        }
    }
    // Debug console setup
    if (isDebugMode)
    {
        if (GetConsoleWindow() == NULL)
        {
            AllocConsole();
            freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
        }
        print("[DEBUG] Mode: %ls", mode.c_str());
        print("[DEBUG] Hover setting: %d", hoverSetting);
        print("[DEBUG] Hover with focus setting: %d", hoverwFocusSetting);
        print("[DEBUG] Full command line: %ls", wideCmdLine.c_str());
    }
    else
    {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }

    HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"STATIC", L"wnq/rmp", WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    RegisterStartup(hInstance, wideCmdLine);

    kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
    if (!kbHook)
        return 1;

    SetTimer(NULL, 1, 50, HoverTimerProc);

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