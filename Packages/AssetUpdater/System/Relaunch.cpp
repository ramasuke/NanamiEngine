#include "Relaunch.h"

#include <cstdlib>
#include <cwchar>

#include <windows.h>

namespace NanamiEngine::AssetUpdater
{
    wchar_t Relauncher::executablePath_  [PATH_CAPACITY]     = {};
    wchar_t Relauncher::workingDirectory_[PATH_CAPACITY]     = {};
    wchar_t Relauncher::commandLine_     [PATH_CAPACITY + 2] = {};
    bool    Relauncher::scheduled_ = false;

    bool Relauncher::ScheduleOnExit()
    {
        if (scheduled_)
            return true;

        constexpr DWORD capacity = static_cast<DWORD>(PATH_CAPACITY);
        const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath_, capacity);
        if (pathLength == 0 || pathLength >= capacity)
            return false;
        const DWORD directoryLength = GetCurrentDirectoryW(capacity, workingDirectory_);
        if (directoryLength == 0 || directoryLength >= capacity)
            return false;
        if (swprintf_s(commandLine_, PATH_CAPACITY + 2, L"\"%ls\"", executablePath_) < 0)
            return false;
        if (std::atexit(RelaunchAtExit) != 0)
            return false;

        scheduled_ = true;
        return true;
    }

    void Relauncher::RelaunchAtExit()
    {
        STARTUPINFOW        startup = {};
        PROCESS_INFORMATION process = {};
        startup.cb = sizeof(startup);
        if (CreateProcessW(executablePath_, commandLine_, nullptr, nullptr, FALSE, 0, nullptr,
                           workingDirectory_, &startup, &process))
        {
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
        }
    }
}
