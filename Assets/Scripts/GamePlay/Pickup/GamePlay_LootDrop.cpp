#include "GamePlay_LootDrop.h"

#include <cmath>
#include <numbers>
#include <random>

#include "../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Data/Drop/Data_DropTable.h"
#include "GamePlay_ItemPickup.h"
#include "GamePlay_MoneyDrop.h"

namespace GamePlay::Pickup
{
    namespace
    {
        std::mt19937& LootRandom()
        {
            static std::mt19937 random{ std::random_device{}() };
            return random;
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
        const float angle = angleRange(LootRandom());
        pickup->Drop(item, count, glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
    }
}
