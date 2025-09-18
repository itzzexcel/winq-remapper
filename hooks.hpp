#ifndef HOOKS_HPP
#define HOOKS_HPP

#include "utils.hpp"

extern HHOOK kbHook;

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
            if (hwnd) {
                if (!PostMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0)) {
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                }
            }
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
        MessageBoxExA(NULL, "Removed from startup.", "Uninstall Complete", MB_OK | MB_ICONINFORMATION, 0);
    else
        MessageBoxExA(NULL, "Failed to remove from startup.", "Uninstall Failed", MB_OK | MB_ICONERROR, 0);
    return success;
}

void RegisterStartup(HINSTANCE hInstance, const std::wstring &wideCmdLine)
{
    HKEY hKey;
    RegCreateKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

    wchar_t exePath[MAX_PATH];

    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0)
    {
        print("[DEBUG] Invalid executable path");
        return;
    }

    std::wstring cmdLine = L"\"" + std::wstring(exePath) + L"\"";
    if (!wideCmdLine.empty())
    {
        if (wideCmdLine.find_first_of(L"&|<>^") == std::wstring::npos)
        {
            cmdLine += L" " + wideCmdLine;
        }
    }

    print("[DEBUG] Command line: %ls", cmdLine.c_str());

    DWORD result = RegSetValueExW(
        hKey,
        L"winqremapper",
        0,
        REG_SZ,
        reinterpret_cast<const BYTE *>(cmdLine.c_str()),
        static_cast<DWORD>((cmdLine.size() + 1) * sizeof(wchar_t)));

    print("[DEBUG] %s", (result == ERROR_SUCCESS) ? "Registry key set successfully." : "Failed to set registry key.");

    RegCloseKey(hKey);
}

#endif // HOOKS_HPP