#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <cstdint>

#define PIPE_NAME L"\\\\.\\pipe\\wnq"
#define print(...) printf(__VA_ARGS__), printf("\n")

DWORD FindProcess(const wchar_t *name)
{
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    if (Process32FirstW(snap, &pe))
    {
        do
        {
            if (!_wcsicmp(pe.szExeFile, name))
            {
                CloseHandle(snap);
                return pe.th32ProcessID;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return 0;
}

struct FindHwndData
{
    DWORD pid;
    HWND hwnd;
};

BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam)
{
    FindHwndData *data = (FindHwndData *)lParam;

    DWORD pid{};
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == data->pid && IsWindowVisible(hwnd))
    {
        data->hwnd = hwnd;
        return FALSE;
    }

    return TRUE;
}

HWND FindMainWindow(DWORD pid)
{
    FindHwndData data{};
    data.pid = pid;
    data.hwnd = nullptr;

    EnumWindows(EnumProc, (LPARAM)&data);

    return data.hwnd;
}

int main()
{
    print("=== wnq pipe test client ===");

    DWORD pid = FindProcess(L"notepad.exe");
    if (!pid)
    {
        print("[DEBUG] Error: notepad.exe not running");
        return 1;
    }

    print("[DEBUG] Found notepad PID = %lu", pid);

    HWND hwnd = FindMainWindow(pid);
    if (!hwnd)
    {
        print("[DEBUG] Error: Could not find hwnd");
        return 2;
    }

    print("[DEBUG] HWND = %p", hwnd);

    if (!WaitNamedPipeW(PIPE_NAME, 5000))
    {
        print("[DEBUG] Error: WaitNamedPipe failed: %lu", GetLastError());
        return 3;
    }

    HANDLE pipe = CreateFileW(
        PIPE_NAME,
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (pipe == INVALID_HANDLE_VALUE)
    {
        print("[DEBUG] Error: CreateFile failed: %lu", GetLastError());
        return 4;
    }

    print("[DEBUG] Connected to pipe");

    uintptr_t raw = reinterpret_cast<uintptr_t>(hwnd);
    DWORD written = 0;

    BOOL ok = WriteFile(
        pipe,
        &raw,
        sizeof(raw),
        &written,
        nullptr);

    if (!ok || written != sizeof(raw))
    {
        print("[DEBUG] Error: WriteFile failed: ok=%d written=%lu err=%lu",
              ok, written, GetLastError());
        CloseHandle(pipe);
        return 5;
    }

    print("[DEBUG] Successfully sent HWND %p (%llu bytes)",
          hwnd, (unsigned long long)written);

    CloseHandle(pipe);

    print("[DEBUG] Done.");
    return 0;
}
