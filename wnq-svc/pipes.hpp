#ifndef PIPES_HPP
#define PIPES_HPP

#include <Windows.h>
#include <cstdio>
#include <sddl.h>

#define print(...) printf(__VA_ARGS__), printf("\n")
#define PIPE_NAME L"\\\\.\\pipe\\wnq"

void HandleNewMessage(uintptr_t rawHwnd)
{
    HWND hwnd = reinterpret_cast<HWND>(rawHwnd);
    print("[DEBUG] Got Window 0x%p", (void *)rawHwnd);

    PostMessage(hwnd, WM_CLOSE, 0, 0);

    Sleep(200);

    if (IsWindow(hwnd) && IsWindowVisible(hwnd))
    {
        // fallback: terminate
        DWORD processId = 0;
        GetWindowThreadProcessId(hwnd, &processId);
        if (processId != 0)
        {
            HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
            if (process)
            {
                if (!TerminateProcess(process, 0))
                    print("[DEBUG] Error: TerminateProcess failed: %lu", GetLastError());
                else
                    print("[DEBUG] Info: Process %lu terminated", processId);
                CloseHandle(process);
            }
            else
            {
                print("[DEBUG] Error: OpenProcess failed: %lu", GetLastError());
            }
        }
    }
    else
    {
        print("[DEBUG] Info: Window closed by WM_CLOSE");
    }
}

void Read()
{
    // Create a permissive security descriptor so we can rule out ACL issues while debugging.
    // WARNING: this allows Everyone full access — only for debugging.
    PSECURITY_DESCRIPTOR psd = nullptr;
    SECURITY_ATTRIBUTES sa{};
    if (ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:(A;OICI;GA;;;WD)", SDDL_REVISION_1, &psd, nullptr))
    {
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = psd;
        sa.bInheritHandle = FALSE;
    }
    else
    {
        psd = nullptr; // we'll pass nullptr and use default security
    }

    while (true)
    {
        HANDLE hPipe = CreateNamedPipeW(
            PIPE_NAME,
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            sizeof(uintptr_t),
            sizeof(uintptr_t),
            0,
            (psd ? &sa : nullptr));

        DWORD errCreate = (hPipe == INVALID_HANDLE_VALUE) ? GetLastError() : 0;
        print("[DEBUG] CreateNamedPipe -> handle=%p err=%lu", hPipe, errCreate);
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            if (psd)
                LocalFree(psd);
            return;
        }

        BOOL connected = ConnectNamedPipe(hPipe, nullptr);
        DWORD errConnect = GetLastError();
        if (!connected && errConnect != ERROR_PIPE_CONNECTED)
        {
            print("[DEBUG] ConnectNamedPipe failed: connected=%d GetLastError=%lu", connected, errConnect);
            CloseHandle(hPipe);
            continue;
        }

        print("[DEBUG] Pipe connected");

        // Read loop: expects client to write sizeof(uintptr_t) per message then close the handle.
        while (true)
        {
            uintptr_t hwndRaw = 0;
            DWORD dwRead = 0;
            BOOL ok = ReadFile(hPipe, &hwndRaw, sizeof(hwndRaw), &dwRead, nullptr);
            if (!ok)
            {
                DWORD err = GetLastError();
                if (err == ERROR_BROKEN_PIPE)
                {
                    print("[DEBUG] Client disconnected (ERROR_BROKEN_PIPE)");
                }
                else
                {
                    print("[DEBUG] ReadFile failed: %lu", err);
                }
                break; // exit read-loop -> close pipe -> create new one
            }

            if (dwRead != sizeof(hwndRaw))
            {
                print("[DEBUG] Incomplete value read: %u bytes", dwRead);
                continue; // keep reading
            }

            HandleNewMessage(hwndRaw);
        }

        FlushFileBuffers(hPipe);
        CloseHandle(hPipe);
        print("[DEBUG] Disconnected / cleaned up");
    }

    if (psd)
        LocalFree(psd);
}

#endif // PIPES_HPP
