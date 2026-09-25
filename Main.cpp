// WinMain / NvOptimusEnablement は exe 側に要る (docs/HotReload.md §8)。エンジン lib / DLL ではなく、
// ゲーム exe プロジェクトが NanamiEngine.Game.props 経由でこのファイルをコンパイルする。DLL 構成では DxLib を見ない
#include <Windows.h>
#include <shellapi.h>
#include <exception>
#include <memory>
#include <string>
#include <string_view>

#include "Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Editor/EditorApplication.h"
#include "Engine/Core/Application/Game/GameApplication.h"
#include "Engine/Module/Exception/Engine_Module_Exception.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"

extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

#pragma comment(lib, "Shell32.lib")

// -project <フォルダ> があればそこを作業ディレクトリにする
bool ApplyProjectArgument()
{
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv == nullptr)
		return true;

	bool succeeded = true;
	for (int i = 1; i < argc; ++i)
	{
		const std::wstring_view argument = argv[i];
		if (argument != L"-project" && argument != L"--project")
			continue;

		if (i + 1 >= argc)
		{
			MessageBoxW(nullptr, L"-project の後にプロジェクトフォルダを指定してください", L"NanamiEngine", MB_OK | MB_ICONERROR);
			succeeded = false;
		}
		else if (!SetCurrentDirectoryW(argv[i + 1]))
		{
			const std::wstring message = std::wstring(L"プロジェクトフォルダを開けません: ") + argv[i + 1];
			MessageBoxW(nullptr, message.c_str(), L"NanamiEngine", MB_OK | MB_ICONERROR);
			succeeded = false;
		}
		break;
	}
	LocalFree(argv);
	return succeeded;
}

void StartApplicationAsync()
{
	std::unique_ptr<NanamiEngine::Core::Application::ApplicationBase> application = nullptr;
	if constexpr (NanamiEngine::Core::Application::Configuration::APPLICATION_MODE == NanamiEngine::Core::Application::Configuration::ApplicationMode::Editor)
	{
		application = std::make_unique<NanamiEngine::Core::Application::EditorApplication>();
	}
	else if constexpr (NanamiEngine::Core::Application::Configuration::APPLICATION_MODE == NanamiEngine::Core::Application::Configuration::ApplicationMode::Game)
	{
		application = std::make_unique<NanamiEngine::Core::Application::Game::GameApplication>();
	}
	application->Run();
	application->OnExit();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	// WARNING: ログや設定を読む前に呼ぶこと
	if (!ApplyProjectArgument())
		return 1;

	//起動時の Scene 破損など回復できないエラーはダイアログを出して終了する
	try
	{
		StartApplicationAsync();
	}
	catch (const NanamiEngine::Module::Exception::NanamiException& exception)
	{
		NanamiEngine::Module::LogError(std::string("[Fatal] ") + exception.what());
		MessageBoxA(nullptr, exception.what(), "NanamiEngine - Fatal Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	catch (const std::exception& exception)
	{
		NanamiEngine::Module::LogError(std::string("[Fatal] unexpected: ") + exception.what());
		MessageBoxA(nullptr, exception.what(), "NanamiEngine - Unexpected Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	return 0;
}