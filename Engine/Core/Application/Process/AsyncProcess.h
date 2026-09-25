#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace NanamiEngine::Core::Application::Process
{
    /** @brief 外部プロセスをワーカースレッドで動かし、stdout / stderr を行ごとに溜める。出力は UTF-8 前提 */
    class NANAMI_API AsyncProcess final
    {
    public:
        AsyncProcess() = default;
        ~AsyncProcess();
        AsyncProcess(const AsyncProcess&)            = delete;
        AsyncProcess& operator=(const AsyncProcess&) = delete;

        /** @brief 前回の出力を捨てて起動する。実行中か起動に失敗したら false で、失敗理由は出力の行に入る */
        bool Start(const std::wstring& commandLine, const std::filesystem::path& workingDirectory);
        /** @brief 子孫プロセスごと終了させる。終了は待たない */
        void Cancel();

        [[nodiscard]] bool               IsRunning   () const;
        [[nodiscard]] bool               WasCanceled () const;
        /** @brief 実行中、起動失敗、中止のときは空 */
        [[nodiscard]] std::optional<int> ExitCode    () const;
        /** @brief 直近の Start からの経過時間 ("m:ss")。終了後は止まる */
        [[nodiscard]] std::string        ElapsedLabel() const;
        /** @brief from 行目以降を out に足し、全体の行数を返す */
        size_t                           CopyLines   (std::vector<std::string>& out, size_t from) const;

    private:
        void Run(void* readPipe, void* process);
        void AppendOutput(const char* data, size_t size);
        void PushLine(std::string line);
        void Join();

        std::thread       worker_;
        std::atomic<bool> running_  = false;
        std::atomic<bool> canceled_ = false;
        void*             job_      = nullptr;

        mutable std::mutex                     mutex_;
        std::vector<std::string>               lines_;
        std::string                            partialLine_;
        std::optional<int>                     exitCode_;
        std::chrono::steady_clock::time_point  startTime_;
        std::chrono::steady_clock::time_point  endTime_;
    };
}
