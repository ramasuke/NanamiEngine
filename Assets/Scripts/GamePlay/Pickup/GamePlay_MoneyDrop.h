#pragma once
#include "vec3.hpp"
#include "../../../../Engine/Module/Namespace/EngineNamespace.h"
#include "../../Core/Game/StatusParameter/Money/Money.h"

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace GamePlay::Pickup
{
    /**
     * @brief total を count 枚のコイン(MoneyPickup 付きのプレハブ)に分け、origin から周りへ飛び散らせる。
     *        1枚あたりの額が 1 を下回らないよう、枚数は total までに抑える
     */
    void DropMoney(Asset::PrefabGameObjectFile& prefab,
                   GameCore::StatusParameter::Money total,
                   int count,
                   const glm::vec3& origin);
}
