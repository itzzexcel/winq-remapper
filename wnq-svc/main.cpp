// Proudly engineered by itzzexcel
// Under the MIT License
// https://github.com/itzzexcel/winq-remapper

#include <windows.h>
#include "pipes.hpp"

#define SERVICE_NAME L"wnq-svc"

SERVICE_STATUS gStatus{};
SERVICE_STATUS_HANDLE gStatusHandle = nullptr;

void SetState(DWORD state)
{
    gStatus.dwCurrentState = state;
    SetServiceStatus(gStatusHandle, &gStatus);
}

void WINAPI ServiceCtrlHandler(DWORD code)
{
    if (code == SERVICE_CONTROL_STOP || code == SERVICE_CONTROL_SHUTDOWN)
        SetState(SERVICE_STOPPED);
}

void WINAPI ServiceMain(DWORD, LPWSTR *)
{
    gStatusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, ServiceCtrlHandler);
    if (!gStatusHandle)
        return;

    gStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    gStatus.dwWin32ExitCode = 0;
    gStatus.dwServiceSpecificExitCode = 0;
    gStatus.dwCheckPoint = 0;
    gStatus.dwWaitHint = 0;

    SetState(SERVICE_START_PENDING);
    SetState(SERVICE_RUNNING);

    Read();

    SetState(SERVICE_STOPPED);
}

bool InstallAndStartSelf()
{
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scm)
        return false;

    SC_HANDLE svc = CreateServiceW(
        scm,
        SERVICE_NAME,
        SERVICE_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_IGNORE,
        path,
        nullptr, nullptr, nullptr,
        nullptr,
        nullptr);

    if (!svc && GetLastError() == ERROR_SERVICE_EXISTS)
        svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_ALL_ACCESS);

    if (!svc)
    {
        CloseServiceHandle(scm);
        return false;
    }

    StartServiceW(svc, 0, nullptr);

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    SERVICE_TABLE_ENTRYW table[] = {
        {(LPWSTR)SERVICE_NAME, ServiceMain},
        {nullptr, nullptr}};

    if (!StartServiceCtrlDispatcherW(table))
        InstallAndStartSelf();

    return 0;
}
