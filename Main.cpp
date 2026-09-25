// WinMain / NvOptimusEnablement は exe 側に要る (docs/HotReload.md §8)。エンジン lib / DLL ではなく、
// ゲーム exe プロジェクトが NanamiEngine.Game.props 経由でこのファイルをコンパイルする。DLL 構成では DxLib を見ない
// NOTE: 起動処理の本体はエンジン側の ApplicationLauncher にある
#include <Windows.h>

#include "Engine/Core/Application/Launch/ApplicationLauncher.h"

extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	namespace Launch = NanamiEngine::Core::Application::Launch;

	// WARNING: ログや設定を読む前に呼ぶこと
	if (!Launch::ApplyProjectArgument())
		return 1;

#ifdef NANAMI_HOST_LOADS_GAME_MODULE
	// ゲーム DLL の静的初期化は Run より前に済ませる。静的 lib のときと同じ順序
	if (!Launch::LoadGameModule())
		return 1;
#endif

	return Launch::RunApplication();
}
