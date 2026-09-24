#include "AsyncProcess.h"

#include <cstdio>

#include <Windows.h>

namespace NanamiEngine::Core::Application::Process
{
    AsyncProcess::~AsyncProcess()
    {
        Cancel();
        Join();
        if (job_)
            CloseHandle(job_);
    }

    bool AsyncProcess::Start(const std::wstring& commandLine, const std::filesystem::path& workingDirectory)
    {
        if (running_)
            return false;
        Join();
        if (job_)
        {
            CloseHandle(job_);
            job_ = nullptr;
        }

        {
            std::lock_guard lock(mutex_);
            lines_.clear();
            partialLine_.clear();
            exitCode_.reset();
            startTime_ = std::chrono::steady_clock::now();
            endTime_   = startTime_;
        }
        canceled_ = false;

        // エディタが落ちても子プロセス (python から起動した rclone なども) が残らないよう、Job に入れておく
        job_ = CreateJobObjectW(nullptr, nullptr);
        if (!job_)
        {
            PushLine("ERROR: Job Object を作れませんでした (GetLastError " + std::to_string(GetLastError()) + ")");
            return false;
        }
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit = {};
        limit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job_, JobObjectExtendedLimitInformation, &limit, sizeof(limit));

        SECURITY_ATTRIBUTES security = { sizeof(security), nullptr, TRUE };
        HANDLE readPipe  = nullptr;
        HANDLE writePipe = nullptr;
        if (!CreatePipe(&readPipe, &writePipe, &security, 0))
        {
            PushLine("ERROR: パイプを作れませんでした (GetLastError " + std::to_string(GetLastError()) + ")");
            return false;
        }
        SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW        startupInfo = {};
        PROCESS_INFORMATION processInfo = {};
        startupInfo.cb         = sizeof(startupInfo);
        startupInfo.dwFlags    = STARTF_USESTDHANDLES;
        startupInfo.hStdOutput = writePipe;
        startupInfo.hStdError  = writePipe;
        std::wstring       mutableCommandLine = commandLine;
        const std::wstring directory          = workingDirectory.wstring();
        const bool started = CreateProcessW(nullptr, mutableCommandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED,
                                            nullptr, directory.empty() ? nullptr : directory.c_str(), &startupInfo, &processInfo);
        const DWORD startError = GetLastError();
        // 子プロセスの終了でパイプが閉じるよう、こちらの書き込み側は閉じておく
        CloseHandle(writePipe);
        if (!started)
        {
            CloseHandle(readPipe);
            PushLine("ERROR: プロセスを起動できませんでした (GetLastError " + std::to_string(startError) + ")");
            return false;
        }

        if (!AssignProcessToJobObject(job_, processInfo.hProcess))
        {
            const DWORD assignError = GetLastError();
            TerminateProcess(processInfo.hProcess, 1);
            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);
            CloseHandle(readPipe);
            PushLine("ERROR: プロセスを Job に入れられませんでした (GetLastError " + std::to_string(assignError) + ")");
            return false;
        }
        ResumeThread(processInfo.hThread);
        CloseHandle(processInfo.hThread);

        running_ = true;
        worker_  = std::thread([this, readPipe, process = processInfo.hProcess] { Run(readPipe, process); });
        return true;
    }

    void AsyncProcess::Cancel()
    {
        if (!running_ || !job_)
            return;
        canceled_ = true;
        TerminateJobObject(job_, 1);
    }

    bool AsyncProcess::IsRunning() const
    {
        return running_;
    }

    bool AsyncProcess::WasCanceled() const
    {
        return canceled_;
    }

    std::optional<int> AsyncProcess::ExitCode() const
    {
        std::lock_guard lock(mutex_);
        return exitCode_;
    }

    std::string AsyncProcess::ElapsedLabel() const
    {
        std::lock_guard lock(mutex_);
        const auto end     = running_ ? std::chrono::steady_clock::now() : endTime_;
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(end - startTime_).count();
        char buffer[32] = {};
        snprintf(buffer, sizeof(buffer), "%d:%02d", static_cast<int>(seconds / 60), static_cast<int>(seconds % 60));
        return buffer;
    }

    size_t AsyncProcess::CopyLines(std::vector<std::string>& out, const size_t from) const
    {
        std::lock_guard lock(mutex_);
        for (size_t i = from; i < lines_.size(); ++i)
            out.push_back(lines_[i]);
        return lines_.size();
    }

    void AsyncProcess::Run(void* const readPipe, void* const process)
    {
        char  buffer[4096];
        DWORD read = 0;
        // NOTE: Job ごと終了させるとパイプも閉じて抜ける
        while (ReadFile(readPipe, buffer, sizeof(buffer), &read, nullptr) && read > 0)
            AppendOutput(buffer, read);
        CloseHandle(readPipe);

        WaitForSingleObject(process, INFINITE);
        DWORD exitCode = 0;
        const bool gotExitCode = GetExitCodeProcess(process, &exitCode);
        CloseHandle(process);

        {
            std::lock_guard lock(mutex_);
            if (!partialLine_.empty())
            {
                lines_.push_back(std::move(partialLine_));
                partialLine_.clear();
            }
            if (gotExitCode && !canceled_)
                exitCode_ = static_cast<int>(exitCode);
            endTime_ = std::chrono::steady_clock::now();
        }
        running_ = false;
    }

    void AsyncProcess::AppendOutput(const char* const data, const size_t size)
    {
        std::lock_guard lock(mutex_);
        for (size_t i = 0; i < size; ++i)
        {
            const char c = data[i];
            if (c == '\n')
            {
                lines_.push_back(std::move(partialLine_));
                partialLine_.clear();
            }
            else if (c != '\r')
            {
                partialLine_.push_back(c);
            }
        }
    }

    void AsyncProcess::PushLine(std::string line)
    {
        std::lock_guard lock(mutex_);
        lines_.push_back(std::move(line));
    }

    void AsyncProcess::Join()
    {
        if (worker_.joinable())
            worker_.join();
    }
}
