#include "Rclone.h"

#include <fstream>
#include <iterator>
#include <thread>
#include <utility>

#include <windows.h>

#include "DistUpload.h"
#include "../Text/Utf8.h"

namespace NanamiEngine::AssetUpdater::Dist
{
    namespace
    {
        constexpr int RCLONE_DIR_NOT_FOUND = 3;

        /** CommandLineToArgvW が元どおりに分けられるよう引用する */
        void RcloneAppendArgument(std::wstring& commandLine, const std::wstring& argument)
        {
            if (!commandLine.empty())
                commandLine += L' ';
            if (!argument.empty() && argument.find_first_of(L" \t\n\v\"") == std::wstring::npos)
            {
                commandLine += argument;
                return;
            }

            commandLine += L'"';
            for (auto it = argument.begin(); ; ++it)
            {
                size_t backslashes = 0;
                while (it != argument.end() && *it == L'\\')
                {
                    ++it;
                    ++backslashes;
                }
                if (it == argument.end())
                {
                    commandLine.append(backslashes * 2, L'\\');
                    break;
                }
                if (*it == L'"')
                {
                    commandLine.append(backslashes * 2 + 1, L'\\');
                    commandLine += L'"';
                }
                else
                {
                    commandLine.append(backslashes, L'\\');
                    commandLine += *it;
                }
            }
            commandLine += L'"';
        }

        std::string RcloneReadAll(const HANDLE pipe)
        {
            std::string text;
            char  buffer[4096];
            DWORD read = 0;
            while (ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr) && read > 0)
                text.append(buffer, read);
            return text;
        }

        std::string RcloneTrim(const std::string& text)
        {
            const size_t first = text.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
                return {};
            const size_t last = text.find_last_not_of(" \t\r\n");
            return text.substr(first, last - first + 1);
        }

        std::unordered_set<std::string> RcloneLines(const std::string& text)
        {
            std::unordered_set<std::string> lines;
            size_t begin = 0;
            while (begin <= text.size())
            {
                size_t end = text.find('\n', begin);
                if (end == std::string::npos)
                    end = text.size();
                if (std::string line = RcloneTrim(text.substr(begin, end - begin)); !line.empty())
                    lines.insert(std::move(line));
                begin = end + 1;
            }
            return lines;
        }

        class RcloneHandle final
        {
        public:
            explicit RcloneHandle(const HANDLE handle = nullptr) : handle_(handle) {}
            ~RcloneHandle() { Reset(); }
            RcloneHandle(const RcloneHandle&)            = delete;
            RcloneHandle& operator=(const RcloneHandle&) = delete;

            void Reset(const HANDLE handle = nullptr)
            {
                if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE)
                    CloseHandle(handle_);
                handle_ = handle;
            }
            [[nodiscard]] HANDLE Get() const { return handle_; }
            HANDLE* Out() { Reset(); return &handle_; }

        private:
            HANDLE handle_ = nullptr;
        };
    }

    std::string ReadLocalFile(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
            throw DistUploadError("読めませんでした: " + WideToUtf8(path.wstring()));
        return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    }

    Rclone::Rclone(std::string exe, const std::string& remote, LineSink log)
        : exe_(std::move(exe))
        , remote_(remote)
        , log_(std::move(log))
    {
        while (!remote_.empty() && remote_.back() == '/')
            remote_.pop_back();
    }

    Rclone::~Rclone()
    {
        std::lock_guard lock(jobMutex_);
        if (job_)
            CloseHandle(job_);
    }

    void Rclone::Cancel()
    {
        canceled_ = true;
        std::lock_guard lock(jobMutex_);
        if (job_)
            TerminateJobObject(job_, 1);
    }

    std::string Rclone::Remote(const std::string& name) const
    {
        return name.empty() ? remote_ : remote_ + "/" + name;
    }

    Rclone::RunResult Rclone::Run(const std::vector<std::string>& args, const bool capture)
    {
        if (canceled_)
            throw DistCanceledError();

        std::wstring commandLine;
        RcloneAppendArgument(commandLine, Utf8ToWide(exe_));
        for (const std::string& arg : args)
            RcloneAppendArgument(commandLine, Utf8ToWide(arg));

        // NOTE: エディタが落ちても rclone が残らないよう Job に入れる。Cancel はこれを終了させる
        const HANDLE job = CreateJobObjectW(nullptr, nullptr);
        if (!job)
            throw DistUploadError("Job Object を作れませんでした (GetLastError " + std::to_string(GetLastError()) + ")");
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit = {};
        limit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit, sizeof(limit));
        {
            std::lock_guard lock(jobMutex_);
            if (job_)
                CloseHandle(job_);
            job_ = job;
        }

        SECURITY_ATTRIBUTES security = { sizeof(security), nullptr, TRUE };
        RcloneHandle outRead, outWrite, errRead, errWrite;
        if (!CreatePipe(outRead.Out(), outWrite.Out(), &security, 0) || (capture && !CreatePipe(errRead.Out(), errWrite.Out(), &security, 0)))
            throw DistUploadError("パイプを作れませんでした (GetLastError " + std::to_string(GetLastError()) + ")");
        SetHandleInformation(outRead.Get(), HANDLE_FLAG_INHERIT, 0);
        if (capture)
            SetHandleInformation(errRead.Get(), HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOW        startupInfo = {};
        PROCESS_INFORMATION processInfo = {};
        startupInfo.cb         = sizeof(startupInfo);
        startupInfo.dwFlags    = STARTF_USESTDHANDLES;
        startupInfo.hStdOutput = outWrite.Get();
        startupInfo.hStdError  = capture ? errWrite.Get() : outWrite.Get();
        const bool started = CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED,
                                            nullptr, nullptr, &startupInfo, &processInfo);
        const DWORD startError = GetLastError();
        // 子プロセスの終了でパイプが閉じるよう、こちらの書き込み側は閉じておく
        outWrite.Reset();
        errWrite.Reset();
        if (!started)
        {
            if (startError == ERROR_FILE_NOT_FOUND || startError == ERROR_PATH_NOT_FOUND)
                throw DistUploadError("rclone が見つかりません: " + exe_);
            throw DistUploadError("rclone を起動できませんでした (GetLastError " + std::to_string(startError) + ")");
        }
        const RcloneHandle process(processInfo.hProcess);
        const RcloneHandle thread(processInfo.hThread);
        if (!AssignProcessToJobObject(job, processInfo.hProcess))
        {
            TerminateProcess(processInfo.hProcess, 1);
            throw DistUploadError("rclone を Job に入れられませんでした (GetLastError " + std::to_string(GetLastError()) + ")");
        }
        ResumeThread(processInfo.hThread);

        RunResult result;
        if (capture)
        {
            // NOTE: 片方のパイプが詰まって止まらないよう、stderr は別スレッドで読む
            std::thread errReader([&result, pipe = errRead.Get()] { result.err = RcloneReadAll(pipe); });
            result.out = RcloneReadAll(outRead.Get());
            errReader.join();
        }
        else
        {
            std::string partial;
            char  buffer[4096];
            DWORD read = 0;
            while (::ReadFile(outRead.Get(), buffer, sizeof(buffer), &read, nullptr) && read > 0)
            {
                for (DWORD i = 0; i < read; ++i)
                {
                    if (buffer[i] == '\n')
                    {
                        log_(partial);
                        partial.clear();
                    }
                    else if (buffer[i] != '\r')
                    {
                        partial.push_back(buffer[i]);
                    }
                }
            }
            if (!partial.empty())
                log_(partial);
        }

        WaitForSingleObject(processInfo.hProcess, INFINITE);
        DWORD exitCode = 1;
        GetExitCodeProcess(processInfo.hProcess, &exitCode);
        result.exitCode = static_cast<int>(exitCode);
        if (canceled_)
            throw DistCanceledError();
        return result;
    }

    std::unordered_set<std::string> Rclone::ListBlobHashes()
    {
        const RunResult result = Run({ "lsf", Remote("files"), "--files-only" }, true);
        if (result.exitCode == RCLONE_DIR_NOT_FOUND)
            return {};
        if (result.exitCode != 0)
            throw DistUploadError("files/ の一覧を取れませんでした: " + RcloneTrim(result.err));
        return RcloneLines(result.out);
    }

    void Rclone::UploadBlobs(const std::filesystem::path& stagingDir)
    {
        const RunResult result = Run({
            "copy", WideToUtf8(stagingDir.wstring()), Remote("files"),
            "--immutable",
            "--transfers", "16",
            "--checkers", "16",
            "--header-upload", DIST_BLOB_CACHE_CONTROL,
            "--stats", "5s", "--stats-one-line", "--stats-log-level", "NOTICE",
        }, false);
        if (result.exitCode != 0)
            throw DistUploadError("ブロブのアップロードに失敗しました (上のログを参照)");
    }

    std::unordered_set<std::string> Rclone::RootFileNames()
    {
        const RunResult result = Run({ "lsf", remote_, "--files-only" }, true);
        if (result.exitCode == RCLONE_DIR_NOT_FOUND)
            return {};
        if (result.exitCode != 0)
            throw DistUploadError("remote の一覧を取れませんでした: " + RcloneTrim(result.err));
        return RcloneLines(result.out);
    }

    std::string Rclone::ReadFile(const std::string& remoteName)
    {
        RunResult result = Run({ "cat", Remote(remoteName) }, true);
        if (result.exitCode != 0)
            throw DistUploadError(remoteName + " を読めませんでした: " + RcloneTrim(result.err));
        return std::move(result.out);
    }

    void Rclone::UploadFile(const std::filesystem::path& local, const std::string& remoteName, const std::string& header)
    {
        const RunResult result = Run({ "copyto", WideToUtf8(local.wstring()), Remote(remoteName), "--header-upload", header }, true);
        if (result.exitCode != 0)
            throw DistUploadError(remoteName + " を上げられませんでした: " + RcloneTrim(result.err));
    }

    std::optional<AssetManifest> Rclone::ReadLiveManifest()
    {
        if (!RootFileNames().contains("manifest.json"))
            return std::nullopt;
        AssetManifest manifest;
        std::string   error;
        if (!AssetManifest::TryParse(ReadFile("manifest.json"), manifest, error))
            throw DistUploadError("公開中の manifest.json を読めませんでした: " + error);
        return manifest;
    }

    bool Rclone::VersionedConflicts(const std::filesystem::path& local, const std::string& remoteName)
    {
        if (!RootFileNames().contains(remoteName))
            return false;
        return ReadFile(remoteName) != ReadLocalFile(local);
    }

    bool Rclone::UploadFileOnce(const std::filesystem::path& local, const std::string& remoteName, const std::string& header)
    {
        if (RootFileNames().contains(remoteName))
        {
            if (ReadFile(remoteName) == ReadLocalFile(local))
                return false;
            throw DistUploadError(remoteName + " はすでに別の内容で存在します。Version を上げて build し直してください");
        }
        UploadFile(local, remoteName, header);
        return true;
    }
}
