#pragma once
// Game.dll が export する唯一の入口。Host は GetProcAddress("PocGetGameApi") で取る。
// shared_ptr / std::string を境界越しに渡す = /MD (CRT 共有) が前提 (docs/HotReload.md §1)
#include <memory>
#include <string>

#include "../Engine/PocEngine.h"

namespace Poc
{
    struct GameApi
    {
        int version;
        std::shared_ptr<Component> (*CreateGameComponent)();
        std::shared_ptr<Component> (*CreateGameOnly)();
        /** ゲーム側のコードで保存・復元する (エンジンの型が渡されてもよい) */
        std::string                (*SaveFromGame)(const std::shared_ptr<Component>& component);
        std::shared_ptr<Component> (*LoadFromGame)(const std::string& json);
        /** IUpdatable 経由で仮想関数を呼ぶ。dynamic_cast が DLL 境界を越えることの確認 */
        int                        (*Tick)(Component& component);
    };
}

#if defined(_WIN32)
#define POC_GAME_EXPORT extern "C" __declspec(dllexport)
#else
#define POC_GAME_EXPORT extern "C" __attribute__((visibility("default")))
#endif
