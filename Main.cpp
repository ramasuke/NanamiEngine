#include "DxLib.h"
#include <memory>

#include "Engine/Core/Application/Configuration/ApplicationConfiguration.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Application/Editor/EditorApplication.h"
#include "Engine/Core/Application/Game/GameApplication.h"

void StartApplicationAsync()
{
	std::unique_ptr<NanamiEngine::Core::Application::ApplicationBase> application = nullptr;
	if constexpr (NanamiEngine::Core::Application::Configuration::APPLICATION_MODE == NanamiEngine::Core::Application::Configuration::ApplicationMode::Editor)
	{
		application = std::make_unique<NanamiEngine::Core::Application::EditorApplication>();
	}
	else if constexpr (NanamiEngine::Core::Application::Configuration::APPLICATION_MODE == NanamiEngine::Core::Application::Configuration::ApplicationMode::Game)
	{
		// const std::unique_ptr<NanamiEngine::Core::Application::ApplicationBase> application = std::make_unique<NanamiEngine::Core::Application::Game::GameApplication>();
	}
	application->Run();
	application->OnExit();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	StartApplicationAsync();
	return 0;
}
