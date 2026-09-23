#include "GamePlay_LootDrop.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Data/Drop/Data_DropTable.h"
#include "GamePlay_ItemPickup.h"
#include "GamePlay_MoneyPickup.h"

namespace GamePlay::Pickup
{
    namespace
    {
        std::mt19937& LootRandom()
        {
            static std::mt19937 random{ std::random_device{}() };
            return random;
        }

        glm::vec3 SideDirection(const float angle)
        {
            return glm::vec3(std::cos(angle), 0.0f, std::sin(angle));
        }

        /**
         * @brief total を count 枚のコインに分け、origin から周りへ飛び散らせる。
         *        1枚あたりの額が 1 を下回らないよう、枚数は total までに抑える
         */
        void DropMoney(Asset::PrefabGameObjectFile& prefab,
                       const GameCore::StatusParameter::Money total,
                       const int count,
                       const glm::vec3& origin)
        {
            if (total.Value() <= 0)
                return;

            const int   coinCount = std::clamp(count, 1, total.Value());
            const int   baseValue = total.Value() / coinCount;
            const int   remainder = total.Value() % coinCount;
            const float step      = 2.0f * std::numbers::pi_v<float> / static_cast<float>(coinCount);

            // 円周を等分した向きへ少しずつずらして散らす。固まって落ちないように
            std::uniform_real_distribution startAngle(0.0f, 2.0f * std::numbers::pi_v<float>);
            std::uniform_real_distribution jitter(-0.25f * step, 0.25f * step);
            const float start = startAngle(LootRandom());

            for (int i = 0; i < coinCount; ++i)
            {
                const auto coin = Scene::GameObject::Instantiate(prefab, origin).lock();
                if (!coin)
                    continue;

                const auto pickup = coin->Components().Catch<MoneyPickup>().lock();
                if (!pickup)
                    continue;

                const float angle = start + step * static_cast<float>(i) + jitter(LootRandom());
                const int   value = baseValue + (i < remainder ? 1 : 0);
                pickup->Drop(GameCore::StatusParameter::Money(value), SideDirection(angle));
            }
        }
    }

    void DropLoot(const Asset::DropTable& table, const glm::vec3& origin)
    {
        if (const auto moneyPrefab = table.MoneyPickupPrefab())
            DropMoney(*moneyPrefab, table.TotalMoney(), table.MoneyPickupCount(), origin);

        std::uniform_real_distribution roll(0.0f, 1.0f);
        for (const auto& drop : table.Items())
        {
            if (roll(LootRandom()) < drop.Chance())
                DropItem(drop.Item(), drop.Count(), origin);
        }
    }

    void DropItem(const std::shared_ptr<Asset::ItemData>& item, const int count, const glm::vec3& origin)
    {
        if (!item || count <= 0)
            return;

        const auto prefab = item->PickupPrefab();
        if (!prefab)
            return;

        const auto pickupObject = Scene::GameObject::Instantiate(*prefab, origin).lock();
        if (!pickupObject)
            return;

        const auto pickup = pickupObject->Components().Catch<ItemPickup>().lock();
        if (!pickup)
            return;

        std::uniform_real_distribution angleRange(0.0f, 2.0f * std::numbers::pi_v<float>);
        pickup->Drop(item, count, SideDirection(angleRange(LootRandom())));
    }
}
