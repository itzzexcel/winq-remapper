// Proudly engineered by itzzexcel
// Under the MIT License
// https://github.com/itzzexcel/winq-remapper

#ifndef HOOKS_HPP
#define HOOKS_HPP

#include "utils.hpp"
#include "svc-comms.hpp"

extern HHOOK kbHook;

HWND GetAppHost(HWND frameHost)
{
    HWND childWindow = nullptr;
    EnumChildWindows(frameHost, [](HWND hwnd, LPARAM lParam) -> BOOL
                     {
        HWND* result = (HWND*)lParam;
        WCHAR className[256];
        if (GetClassNameW(hwnd, className, 256)) {
            if (wcscmp(className, L"Windows.UI.Core.CoreWindow") == 0 ||
                wcscmp(className, L"ApplicationFrameInputSinkWindow") == 0) {
                *result = hwnd;
                return FALSE;
            }
        }
        return TRUE; }, (LPARAM)&childWindow);
    return childWindow;
}

bool IsApplicationFrameHost(HWND hwnd)
{
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (process)
    {
        WCHAR processName[MAX_PATH];
        if (GetModuleBaseNameW(process, nullptr, processName, MAX_PATH))
        {
            CloseHandle(process);
            return wcscmp(processName, L"ApplicationFrameHost.exe") == 0;
        }
        CloseHandle(process);
    }
    return false;
}

bool IsTargetElevated(DWORD pid)
{
    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProc)
        return true;

    HANDLE hToken;
    bool elevated = false;
    if (OpenProcessToken(hProc, TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION elevation;
        DWORD size;
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &size))
            elevated = elevation.TokenIsElevated;
        CloseHandle(hToken);
    }
    CloseHandle(hProc);

    return elevated;
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
            {
                HWND targetWindow = hwnd;

                if (IsApplicationFrameHost(hwnd))
                {
                    HWND uwpWindow = GetAppHost(hwnd);
                    if (uwpWindow)
                        targetWindow = uwpWindow;
                }

                if (IsWindow(targetWindow))
                {
                    if (enableForceKeybind)
                    {
                        print("[DEBUG] Sending HWND %p to svc", targetWindow);
                        SendWindowSync(targetWindow);
                    }
                    else
                    {
                        PostMessage(targetWindow, WM_CLOSE, 0, 0);
                    }
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

bool UninstallService()
{
    bool success = false;

    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
        return false;

    SC_HANDLE svc = OpenServiceW(scm, L"wnq-svc", SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS);
    if (!svc)
    {
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_STATUS status{};
    ControlService(svc, SERVICE_CONTROL_STOP, &status);

    if (DeleteService(svc))
        success = true;

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);

    if (success)
        MessageBoxW(nullptr, L"Service uninstalled successfully.", L"Success", MB_OK | MB_ICONINFORMATION);
    else
        MessageBoxW(nullptr, L"Failed to uninstall service.", L"Error", MB_OK | MB_ICONERROR);

    return success;
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

    bool svcSuccess = false;

    if (UninstallService())
    {
        MessageBoxExA(NULL, "wnq-svc service removed.", "Uninstall Complete", MB_OK | MB_ICONINFORMATION, 0);
        svcSuccess = true;
    }
    else
    {
        MessageBoxExA(NULL, "Failed to remove wnq-svc service.\nTry running your terminal as administrator.", "Uninstall Failed", MB_OK | MB_ICONERROR, 0);
    }

    if (success)
        MessageBoxExA(NULL, "Removed from startup.", "Uninstall Complete", MB_OK | MB_ICONINFORMATION, 0);
    else
        MessageBoxExA(NULL, "Failed to remove from startup.", "Uninstall Failed", MB_OK | MB_ICONERROR, 0);

    return success && svcSuccess;
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

    print("[DEBUG] Attempting to run wnq-svc.exe as administrator.");

    wchar_t currentDir[MAX_PATH];
    if (GetCurrentDirectoryW(MAX_PATH, currentDir) != 0)
    {
        std::wstring svcPath = std::wstring(currentDir) + L"\\wnq-svc.exe";

        SHELLEXECUTEINFOW shExInfo = {0};
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = nullptr;
        shExInfo.lpVerb = L"runas";
        shExInfo.lpFile = svcPath.c_str();
        shExInfo.lpParameters = L"";
        shExInfo.nShow = SW_NORMAL;

        if (ShellExecuteExW(&shExInfo))
        {
            print("[DEBUG] Service executable launched as administrator.");
            if (shExInfo.hProcess)
                CloseHandle(shExInfo.hProcess);
        }
        else
        {
            print("[DEBUG] Failed to launch service executable as administrator.");
        }
    }
    else
    {
        print("[DEBUG] Failed to get current directory.");
    }
}

#endif // HOOKS_HPP