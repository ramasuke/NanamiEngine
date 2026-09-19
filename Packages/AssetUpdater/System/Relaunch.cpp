#include "Relaunch.h"

#include <cstdlib>
#include <cwchar>

#include <windows.h>

namespace NanamiEngine::AssetUpdater
{
    namespace
    {
        constexpr DWORD RELAUNCH_PATH_CAPACITY = 4096;

        // atexit の中から読むので、破棄の順番を気にしなくていい静的な配列に置く
        wchar_t RelaunchExecutablePath  [RELAUNCH_PATH_CAPACITY]     = {};
        wchar_t RelaunchWorkingDirectory[RELAUNCH_PATH_CAPACITY]     = {};
        wchar_t RelaunchCommandLine     [RELAUNCH_PATH_CAPACITY + 2] = {};
        bool    RelaunchScheduled = false;

        void RelaunchAtExit()
        {
            STARTUPINFOW        startup = {};
            PROCESS_INFORMATION process = {};
            startup.cb = sizeof(startup);
            if (CreateProcessW(RelaunchExecutablePath, RelaunchCommandLine, nullptr, nullptr, FALSE, 0, nullptr,
                               RelaunchWorkingDirectory, &startup, &process))
            {
                CloseHandle(process.hThread);
                CloseHandle(process.hProcess);
            }
        }
    }

    bool ScheduleRelaunchOnExit()
    {
        if (RelaunchScheduled)
            return true;

        const DWORD pathLength = GetModuleFileNameW(nullptr, RelaunchExecutablePath, RELAUNCH_PATH_CAPACITY);
        if (pathLength == 0 || pathLength >= RELAUNCH_PATH_CAPACITY)
            return false;
        const DWORD directoryLength = GetCurrentDirectoryW(RELAUNCH_PATH_CAPACITY, RelaunchWorkingDirectory);
        if (directoryLength == 0 || directoryLength >= RELAUNCH_PATH_CAPACITY)
            return false;
        if (swprintf_s(RelaunchCommandLine, RELAUNCH_PATH_CAPACITY + 2, L"\"%ls\"", RelaunchExecutablePath) < 0)
            return false;
        if (std::atexit(RelaunchAtExit) != 0)
            return false;

        RelaunchScheduled = true;
        return true;
    }
}
