#include "DxLib.h"
#include <exception>
#include <memory>
#include <string>

#include "Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Editor/EditorApplication.h"
#include "Engine/Core/Application/Game/GameApplication.h"
#include "Engine/Module/Exception/Engine_Module_Exception.h"
#include "Engine/Module/Log/NanamiEngine_Module_Log.h"

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
	// 最後の安全網。起動時の Scene 破損など回復できない失敗は、CRT の abort ではなく原因が読めるダイアログを出して終了する
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