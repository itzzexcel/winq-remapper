#ifndef SVCCOMMS_HPP
#define SVCCOMMS_HPP

#include <Windows.h>
#include <cstdio>

#define PIPE_NAME L"\\\\.\\pipe\\wnq"

inline bool SendWindowSync(HWND window)
{
    if (!IsWindow(window))
    {
        print("[DEBUG] SendWindowSync: invalid window");
        return false;
    }

    print("[DEBUG] SendWindowSync: waiting for pipe...");
    BOOL waited = WaitNamedPipeW(PIPE_NAME, 3000);
    DWORD errWait = GetLastError();
    print("[DEBUG] WaitNamedPipe -> %d err=%lu", waited, errWait);

    if (!waited)
    {
        // Try one quick retry
        Sleep(100);
        waited = WaitNamedPipeW(PIPE_NAME, 1000);
        errWait = GetLastError();
        print("[DEBUG] Retry WaitNamedPipe -> %d err=%lu", waited, errWait);
        if (!waited)
            return false;
    }

    HANDLE pipe = CreateFileW(
        PIPE_NAME,
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    DWORD errCreate = (pipe == INVALID_HANDLE_VALUE) ? GetLastError() : 0;
    print("[DEBUG] CreateFile -> handle=%p err=%lu", pipe, errCreate);

    if (pipe == INVALID_HANDLE_VALUE)
        return false;

    uintptr_t raw = reinterpret_cast<uintptr_t>(window);
    DWORD written = 0;

    BOOL ok = WriteFile(pipe, &raw, sizeof(raw), &written, nullptr);
    DWORD errWrite = ok ? 0 : GetLastError();
    print("[DEBUG] WriteFile -> ok=%d written=%lu err=%lu", ok, written, errWrite);

    CloseHandle(pipe);

    return ok && written == sizeof(raw);
}

#endif // SVCCOMMS_HPP
