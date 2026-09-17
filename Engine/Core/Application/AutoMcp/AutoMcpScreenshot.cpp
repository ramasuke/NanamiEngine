#include "AutoMcpScreenshot.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

#include "DxLib.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    namespace
    {
        constexpr auto SCREENSHOT_DIRECTORY = "AutoMcp/Screenshots";
        constexpr int  SCREENSHOT_KEEP_COUNT = 20;

        int& ScreenshotSequence()
        {
            static int sequence = 0;
            return sequence;
        }

        std::filesystem::path ScreenshotPath(const int sequence, const std::string& extension)
        {
            return std::filesystem::path(SCREENSHOT_DIRECTORY) / ("screenshot_" + std::to_string(sequence) + extension);
        }

        void RemoveScreenshot(const int sequence)
        {
            std::error_code error;
            std::filesystem::remove(ScreenshotPath(sequence, ".jpg"), error);
            std::filesystem::remove(ScreenshotPath(sequence, ".png"), error);
        }

        void PrepareDirectory()
        {
            std::error_code error;
            // 前回起動時の残りは連番が重なるので、このセッションの初回に片付ける
            if (ScreenshotSequence() == 0)
                std::filesystem::remove_all(SCREENSHOT_DIRECTORY, error);

            std::filesystem::create_directories(SCREENSHOT_DIRECTORY, error);
            if (error)
                throw AutoMcpError("failed to create " + std::string(SCREENSHOT_DIRECTORY) + ": " + error.message());
        }

        int MakeScreenWithoutAsyncLoad(const int width, const int height)
        {
            // 非同期読み込みのままだと読み込み中ハンドルが返り、GetDrawScreenGraph が失敗する
            const int useASyncLoad = GetUseASyncLoadFlag();
            SetUseASyncLoadFlag(FALSE);
            const int handle = MakeScreen(width, height, FALSE);
            SetUseASyncLoadFlag(useASyncLoad);
            return handle;
        }
    }

    AutoMcpCapture AutoMcpScreenshot::Grab()
    {
        AutoMcpCapture capture;
        GetDrawScreenSize(&capture.width, &capture.height);
        if (capture.width <= 0 || capture.height <= 0)
            return capture;

        capture.sourceHandle = MakeScreenWithoutAsyncLoad(capture.width, capture.height);
        if (capture.sourceHandle == -1)
            return capture;

        if (GetDrawScreenGraph(0, 0, capture.width, capture.height, capture.sourceHandle, TRUE) == -1)
            Release(capture);

        return capture;
    }

    void AutoMcpScreenshot::Save(const AutoMcpCapture& capture, const std::string& format, const int maxWidth, const int quality, JsonValue& result, JsonAllocator& allocator)
    {
        if (capture.sourceHandle == -1)
            throw AutoMcpError("failed to capture the draw screen");

        const bool isPng = format == "png";
        if (!isPng && format != "jpeg" && format != "jpg")
            throw AutoMcpError("format must be \"jpeg\" or \"png\"");

        PrepareDirectory();

        const int outputWidth  = std::clamp(maxWidth, 1, capture.width);
        const int outputHeight = (std::max)(1, capture.height * outputWidth / capture.width);

        const int targetHandle = MakeScreenWithoutAsyncLoad(outputWidth, outputHeight);
        if (targetHandle == -1)
            throw AutoMcpError("failed to create a screenshot render target");

        const int sequence = ++ScreenshotSequence();
        const std::filesystem::path path = ScreenshotPath(sequence, isPng ? ".png" : ".jpg");
        const std::string nativePath = path.string();

        int blendMode      = 0;
        int blendParameter = 0;
        GetDrawBlendMode(&blendMode, &blendParameter);
        const int drawMode = GetDrawMode();

        SetDrawScreen(targetHandle);
        ClearDrawScreen();
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        SetDrawMode(DX_DRAWMODE_BILINEAR);
        DrawExtendGraph(0, 0, outputWidth, outputHeight, capture.sourceHandle, FALSE);

        const int saveResult = isPng
            ? SaveDrawScreenToPNG (0, 0, outputWidth, outputHeight, nativePath.c_str(), -1)
            : SaveDrawScreenToJPEG(0, 0, outputWidth, outputHeight, nativePath.c_str(), std::clamp(quality, 1, 100), TRUE);

        SetDrawMode(drawMode);
        SetDrawBlendMode(blendMode, blendParameter);
        SetDrawScreen(DX_SCREEN_BACK);
        DeleteGraph(targetHandle);

        if (saveResult == -1)
            throw AutoMcpError("failed to save " + nativePath);

        RemoveScreenshot(sequence - SCREENSHOT_KEEP_COUNT);

        std::error_code error;
        const std::filesystem::path absolutePath = std::filesystem::absolute(path, error);
        const std::u8string utf8Path = (error ? path : absolutePath).u8string();

        result.AddMember("path", MakeString(std::string(utf8Path.begin(), utf8Path.end()), allocator), allocator);
        result.AddMember("format", MakeString(isPng ? "png" : "jpeg", allocator), allocator);
        result.AddMember("width", outputWidth, allocator);
        result.AddMember("height", outputHeight, allocator);
        result.AddMember("sourceWidth", capture.width, allocator);
        result.AddMember("sourceHeight", capture.height, allocator);
    }

    void AutoMcpScreenshot::Release(AutoMcpCapture& capture)
    {
        if (capture.sourceHandle != -1)
            DeleteGraph(capture.sourceHandle);

        capture.sourceHandle = -1;
    }
}
