// NOTE: 起動処理の本体はエンジン側の ApplicationLauncher にある
#include <Windows.h>

#include "Engine/Core/Application/Launch/ApplicationLauncher.h"

extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	namespace Launch = NanamiEngine::Core::Application::Launch;

	if (!Launch::ApplyProjectArgument())
		return 1;

#ifdef NANAMI_HOST_LOADS_GAME_MODULE
	// NOTE: ゲーム DLL の静的初期化は Run より前に済ませる。静的 lib のときと同じ順序
	if (!Launch::LoadGameModule())
		return 1;
#endif

	return Launch::RunApplication();
}
