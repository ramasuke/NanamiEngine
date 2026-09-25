#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <filesystem>
#include <functional>
#include <stop_token>
#include <string>

namespace NanamiEngine::AssetUpdater
{
    struct NANAMI_API HttpGetResult
    {
        bool        ok         = false;
        int         statusCode = 0;
        std::string body;
        std::string error;
    };

    struct NANAMI_API HttpDownloadResult
    {
        bool        ok         = false;
        bool        cancelled  = false;
        int         statusCode = 0;
        std::string error;
    };

    /** 同期通信。呼び出したスレッドをブロックする。1つのセッションで接続を使い回す */
    class NANAMI_API WinHttpClient final
    {
    public:
        WinHttpClient(const std::wstring& userAgent, int timeoutMilliSeconds);
        ~WinHttpClient();

        WinHttpClient(const WinHttpClient&)            = delete;
        WinHttpClient& operator=(const WinHttpClient&) = delete;

        [[nodiscard]] HttpGetResult GetString(const std::string& url);
        /** 受け取るたびにそのバイト数を onReceived に渡す */
        [[nodiscard]] HttpDownloadResult DownloadToFile(const std::string& url,
                                                        const std::filesystem::path& destination,
                                                        const std::stop_token& stopToken,
                                                        const std::function<void(std::uint64_t)>& onReceived);

    private:
        // windows.h をヘッダに持ち込まないため HINTERNET を void* で持つ
        void*       session_ = nullptr;
        std::string openError_;
    };
}
