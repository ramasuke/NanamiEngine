#pragma once
#include <memory>

#include "vec3.hpp"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::Asset
{
    class DropTable;
    class ItemData;
}

namespace GamePlay::Pickup
{
    /** @brief お金を毎回、アイテムを行ごとの確率で独立に抽選して origin から散らす */
    void DropLoot(const Asset::DropTable& table, const glm::vec3& origin);

    /** @brief item の拾い物プレハブを count 個分として1つ出し、ランダムな向きへ跳ね上げる。プレハブ未設定なら何もしない */
    void DropItem(const std::shared_ptr<Asset::ItemData>& item, int count, const glm::vec3& origin);
}
