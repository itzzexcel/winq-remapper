#ifndef UTILS_HPP
#define UTILS_HPP

#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <iostream>
#include <psapi.h>

#define print(...)       \
    printf(__VA_ARGS__); \
    printf("\n");

extern std::wstring mode;

extern bool hoverSetting;
extern bool hoverwFocusSetting;
extern bool isDebugMode;
extern bool wKeyPressed;
extern bool enableForceKeybind;

extern HWND lastHoverWindow;

static HWND GetCurrentMouseHoverWindow()
{
    if (hoverSetting || hoverwFocusSetting)
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
    }
    else
    {
        HWND hwnd = GetForegroundWindow();
        if (hwnd)
            return hwnd;
    }
    return NULL;
}

DWORD GetForegroundLockTimeout()
{
    DWORD timeout = 0;
    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &timeout, 0);
    return timeout;
}

void SetForegroundWindowForced(HWND hWnd)
{
    DWORD originalTimeout = GetForegroundLockTimeout();
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, SPIF_SENDCHANGE);
    SetForegroundWindow(hWnd);
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)(ULONG_PTR)originalTimeout, SPIF_SENDCHANGE);
}

void CheckHoverWindowChange()
{
    HWND currentHover = GetCurrentMouseHoverWindow();

    char windowTitle[256] = {0};
    char className[256] = {0};

    if (currentHover != lastHoverWindow)
    {
        if (currentHover)
        {
            GetWindowTextA(currentHover, windowTitle, 255);
            GetClassNameA(currentHover, className, 255);
            if (hoverwFocusSetting)
            {
                LONG style = GetWindowLong(currentHover, GWL_STYLE);
                RECT windowRect;
                GetWindowRect(currentHover, &windowRect);
                if ((style & WS_VISIBLE) && !(style & WS_MINIMIZE))
                {
                    POINT cursorPos;
                    GetCursorPos(&cursorPos);
                    if (PtInRect(&windowRect, cursorPos))
                    {

                        // Hacky thing
                        HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, [](int code, WPARAM wp, LPARAM lp) -> LRESULT
                                                      { return CallNextHookEx(NULL, code, wp, lp); }, GetModuleHandle(NULL), 0);
                        SetForegroundWindow(currentHover);
                        UnhookWindowsHookEx(hook);
                    }
                }
            }
        }
        lastHoverWindow = currentHover;
        std::string trimmedTitle(windowTitle);
        if (trimmedTitle.length() > 45)
            trimmedTitle = trimmedTitle.substr(0, 45) + "...";
        print("[DEBUG] HWND: 0x%p | Class: %s | Title: %s", currentHover, className, trimmedTitle.c_str());
    }
}

// Attach to console
void AttachToConsole()
{
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        AllocConsole();
    freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
}

VOID CALLBACK HoverTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    CheckHoverWindowChange();
}

#endif // UTILS_HPP