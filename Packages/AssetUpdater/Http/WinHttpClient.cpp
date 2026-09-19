#include "WinHttpClient.h"

#include <fstream>
#include <vector>

#include <windows.h>
#include <winhttp.h>

#include "../Text/Utf8.h"

#pragma comment(lib, "Winhttp.lib")

namespace NanamiEngine::AssetUpdater
{
    namespace
    {
        constexpr DWORD WIN_HTTP_HOST_NAME_CAPACITY  = 256;
        constexpr DWORD WIN_HTTP_URL_PATH_CAPACITY   = 2048;
        constexpr DWORD WIN_HTTP_EXTRA_INFO_CAPACITY = 2048;

        std::string WinHttpClientErrorText(const char* stage)
        {
            return std::string(stage) + " に失敗しました (GetLastError=" + std::to_string(GetLastError()) + ")";
        }

        /** セッション・接続・リクエストはどれも WinHttpCloseHandle で閉じる */
        class WinHttpClientHandle final
        {
        public:
            explicit WinHttpClientHandle(const HINTERNET handle) : handle_(handle) {}
            ~WinHttpClientHandle()
            {
                if (handle_ != nullptr)
                    WinHttpCloseHandle(handle_);
            }

            WinHttpClientHandle(const WinHttpClientHandle&)            = delete;
            WinHttpClientHandle& operator=(const WinHttpClientHandle&) = delete;

            [[nodiscard]] HINTERNET Get() const { return handle_; }
            explicit operator bool() const { return handle_ != nullptr; }

        private:
            HINTERNET handle_ = nullptr;
        };

        struct WinHttpClientResponse
        {
            bool        ok         = false;
            bool        stopped    = false;
            int         statusCode = 0;
            std::string error;
        };

        /** 200 以外は本文を読まずに返す。onChunk が false を返したら読むのをやめる */
        WinHttpClientResponse WinHttpClientRequest(const HINTERNET session,
                                                   const std::string& url,
                                                   const std::function<bool(const char*, std::size_t)>& onChunk)
        {
            WinHttpClientResponse response;

            const std::wstring wideUrl = Utf8ToWide(url);
            if (wideUrl.empty())
            {
                response.error = "URL が空か、UTF-8 として不正です";
                return response;
            }

            wchar_t hostNameBuffer [WIN_HTTP_HOST_NAME_CAPACITY]  = {};
            wchar_t urlPathBuffer  [WIN_HTTP_URL_PATH_CAPACITY]   = {};
            wchar_t extraInfoBuffer[WIN_HTTP_EXTRA_INFO_CAPACITY] = {};

            URL_COMPONENTS components    = {};
            components.dwStructSize      = sizeof(components);
            components.lpszHostName      = hostNameBuffer;
            components.dwHostNameLength  = WIN_HTTP_HOST_NAME_CAPACITY;
            components.lpszUrlPath       = urlPathBuffer;
            components.dwUrlPathLength   = WIN_HTTP_URL_PATH_CAPACITY;
            components.lpszExtraInfo     = extraInfoBuffer;
            components.dwExtraInfoLength = WIN_HTTP_EXTRA_INFO_CAPACITY;

            if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.size()), 0, &components))
            {
                response.error = WinHttpClientErrorText("URL の解析");
                return response;
            }

            const std::wstring hostName(components.lpszHostName, components.dwHostNameLength);
            const std::wstring objectName = std::wstring(components.lpszUrlPath, components.dwUrlPathLength)
                                          + std::wstring(components.lpszExtraInfo, components.dwExtraInfoLength);

            const WinHttpClientHandle connection(WinHttpConnect(session, hostName.c_str(), components.nPort, 0));
            if (!connection)
            {
                response.error = WinHttpClientErrorText("WinHttpConnect");
                return response;
            }

            const DWORD requestFlags = components.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
            const WinHttpClientHandle request(WinHttpOpenRequest(connection.Get(),
                                                                 L"GET",
                                                                 objectName.c_str(),
                                                                 nullptr,
                                                                 WINHTTP_NO_REFERER,
                                                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                                 requestFlags));
            if (!request)
            {
                response.error = WinHttpClientErrorText("WinHttpOpenRequest");
                return response;
            }

            if (!WinHttpSendRequest(request.Get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
            {
                response.error = WinHttpClientErrorText("WinHttpSendRequest");
                return response;
            }
            if (!WinHttpReceiveResponse(request.Get(), nullptr))
            {
                response.error = WinHttpClientErrorText("WinHttpReceiveResponse");
                return response;
            }

            DWORD statusCode     = 0;
            DWORD statusCodeSize = sizeof(statusCode);
            if (!WinHttpQueryHeaders(request.Get(),
                                     WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                     WINHTTP_HEADER_NAME_BY_INDEX,
                                     &statusCode,
                                     &statusCodeSize,
                                     WINHTTP_NO_HEADER_INDEX))
            {
                response.error = WinHttpClientErrorText("WinHttpQueryHeaders");
                return response;
            }
            response.statusCode = static_cast<int>(statusCode);
            if (statusCode != 200)
            {
                response.error = "HTTP " + std::to_string(statusCode) + " が返りました";
                return response;
            }

            std::vector<char> buffer;
            for (;;)
            {
                DWORD available = 0;
                if (!WinHttpQueryDataAvailable(request.Get(), &available))
                {
                    response.error = WinHttpClientErrorText("WinHttpQueryDataAvailable");
                    return response;
                }
                if (available == 0)
                    break;

                if (buffer.size() < available)
                    buffer.resize(available);

                DWORD read = 0;
                if (!WinHttpReadData(request.Get(), buffer.data(), available, &read))
                {
                    response.error = WinHttpClientErrorText("WinHttpReadData");
                    return response;
                }
                if (!onChunk(buffer.data(), read))
                {
                    response.stopped = true;
                    return response;
                }
            }

            response.ok = true;
            return response;
        }
    }

    WinHttpClient::WinHttpClient(const std::wstring& userAgent, const int timeoutMilliSeconds)
    {
        session_ = WinHttpOpen(userAgent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (session_ == nullptr)
        {
            openError_ = WinHttpClientErrorText("WinHttpOpen");
            return;
        }

        // 既定の解決・接続タイムアウトは分単位で、サーバーに届かないとタイトル画面が長く待たされる
        WinHttpSetTimeouts(session_, timeoutMilliSeconds, timeoutMilliSeconds, timeoutMilliSeconds, timeoutMilliSeconds);
    }

    WinHttpClient::~WinHttpClient()
    {
        if (session_ != nullptr)
            WinHttpCloseHandle(session_);
    }

    HttpGetResult WinHttpClient::GetString(const std::string& url)
    {
        HttpGetResult result;
        if (session_ == nullptr)
        {
            result.error = openError_;
            return result;
        }

        const WinHttpClientResponse response = WinHttpClientRequest(session_, url, [&result](const char* data, const std::size_t size)
        {
            result.body.append(data, size);
            return true;
        });

        result.ok         = response.ok;
        result.statusCode = response.statusCode;
        result.error      = response.error;
        return result;
    }

    HttpDownloadResult WinHttpClient::DownloadToFile(const std::string& url,
                                                     const std::filesystem::path& destination,
                                                     const std::stop_token& stopToken,
                                                     const std::function<void(std::uint64_t)>& onReceived)
    {
        HttpDownloadResult result;
        if (session_ == nullptr)
        {
            result.error = openError_;
            return result;
        }

        std::ofstream stream(destination, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            result.error = "書き込めません: " + WideToUtf8(destination.wstring());
            return result;
        }

        bool writeFailed = false;
        const WinHttpClientResponse response = WinHttpClientRequest(session_, url, [&](const char* data, const std::size_t size)
        {
            if (stopToken.stop_requested())
                return false;

            stream.write(data, static_cast<std::streamsize>(size));
            if (!stream)
            {
                writeFailed = true;
                return false;
            }
            onReceived(size);
            return true;
        });
        stream.close();

        result.statusCode = response.statusCode;
        if (writeFailed || (response.ok && !stream))
        {
            result.error = "書き込めません: " + WideToUtf8(destination.wstring());
            return result;
        }
        if (response.stopped)
        {
            result.cancelled = true;
            return result;
        }
        if (!response.ok)
        {
            result.error = response.error;
            return result;
        }

        result.ok = true;
        return result;
    }
}
