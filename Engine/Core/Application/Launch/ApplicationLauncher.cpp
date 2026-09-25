#include "ApplicationLauncher.h"

#include <Windows.h>
#include <shellapi.h>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "../ApplicationBase.h"
#include "../Configuration/ApplicationConfiguration.h"
#include "../Editor/EditorApplication.h"
#include "../Game/GameApplication.h"
#include "../HotReload/GameModule.h"
#include "../../../Module/Exception/Engine_Module_Exception.h"
#include "../../../Module/Log/NanamiEngine_Module_Log.h"

#pragma comment(lib, "Shell32.lib")

namespace NanamiEngine::Core::Application::Launch
{
    namespace
    {
        // argv[0] (exe パス) を除いたコマンドライン引数
        std::vector<std::wstring> CommandLineArguments()
        {
            std::vector<std::wstring> arguments;
            int argc = 0;
            LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
            if (argv == nullptr)
                return arguments;

            for (int i = 1; i < argc; ++i)
                arguments.emplace_back(argv[i]);
            LocalFree(argv);
            return arguments;
        }
    }

    bool ApplyProjectArgument()
    {
        const std::vector<std::wstring> arguments = CommandLineArguments();
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            if (arguments[i] != L"-project" && arguments[i] != L"--project")
                continue;

            if (i + 1 >= arguments.size())
            {
                MessageBoxW(nullptr, L"-project の後にプロジェクトフォルダを指定してください", L"NanamiEngine", MB_OK | MB_ICONERROR);
                return false;
            }
            if (!SetCurrentDirectoryW(arguments[i + 1].c_str()))
            {
                const std::wstring message = L"プロジェクトフォルダを開けません: " + arguments[i + 1];
                MessageBoxW(nullptr, message.c_str(), L"NanamiEngine", MB_OK | MB_ICONERROR);
                return false;
            }
            break;
        }
        return true;
    }

    bool LoadGameModule()
    {
        std::filesystem::path dllPath;
        const std::vector<std::wstring> arguments = CommandLineArguments();
        for (size_t i = 0; i + 1 < arguments.size(); ++i)
        {
            if (arguments[i] == L"-game" || arguments[i] == L"--game")
                dllPath = arguments[i + 1];
        }
        if (dllPath.empty())
        {
            wchar_t exePath[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            dllPath = std::filesystem::path(exePath).replace_extension(L".dll");
        }

        std::string error;
        if (HotReload::GameModule::Instance().LoadInitial(dllPath, error))
            return true;

        MessageBoxA(nullptr, error.c_str(), "NanamiEngine - Game DLL", MB_OK | MB_ICONERROR);
        return false;
    }

    int RunApplication()
    {
        //起動時の Scene 破損など回復できないエラーはダイアログを出して終了する
        try
        {
            std::unique_ptr<ApplicationBase> application = nullptr;
            if constexpr (Configuration::APPLICATION_MODE == Configuration::ApplicationMode::Editor)
            {
                application = std::make_unique<EditorApplication>();
            }
            else if constexpr (Configuration::APPLICATION_MODE == Configuration::ApplicationMode::Game)
            {
                application = std::make_unique<Game::GameApplication>();
            }
            application->Run();
            application->OnExit();
        }
        catch (const Module::Exception::NanamiException& exception)
        {
            Module::LogError(std::string("[Fatal] ") + exception.what());
            MessageBoxA(nullptr, exception.what(), "NanamiEngine - Fatal Error", MB_OK | MB_ICONERROR);
            return 1;
        }
        catch (const std::exception& exception)
        {
            Module::LogError(std::string("[Fatal] unexpected: ") + exception.what());
            MessageBoxA(nullptr, exception.what(), "NanamiEngine - Unexpected Error", MB_OK | MB_ICONERROR);
            return 1;
        }
        return 0;
    }
}
