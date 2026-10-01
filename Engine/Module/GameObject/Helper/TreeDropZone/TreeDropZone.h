#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <cstddef>

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;

    // ヒエラルキーで parent の子リストの挿入位置を表すドロップ受け皿を描く
    // NOTE: insertIndex はドラッグ中オブジェクトを除く前の子リストでのインデックス
    NANAMI_API void DrawSiblingInsertionDropZone(const std::shared_ptr<IGameObject>& parent, std::size_t insertIndex);
}
