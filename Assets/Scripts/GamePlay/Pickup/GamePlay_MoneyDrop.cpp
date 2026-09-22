#include "GamePlay_MoneyDrop.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "GamePlay_MoneyPickup.h"

namespace GamePlay::Pickup
{
    void DropMoney(Asset::PrefabGameObjectFile& prefab,
                   const GameCore::StatusParameter::Money total,
                   const int count,
                   const glm::vec3& origin)
    {
        if (total.Value() <= 0)
            return;

        static std::mt19937 random{ std::random_device{}() };

        const int   coinCount = std::clamp(count, 1, total.Value());
        const int   baseValue = total.Value() / coinCount;
        const int   remainder = total.Value() % coinCount;
        const float step      = 2.0f * std::numbers::pi_v<float> / static_cast<float>(coinCount);

        // 円周を等分した向きへ少しずつずらして散らす。固まって落ちないように
        std::uniform_real_distribution startAngle(0.0f, 2.0f * std::numbers::pi_v<float>);
        std::uniform_real_distribution jitter(-0.25f * step, 0.25f * step);
        const float start = startAngle(random);

        for (int i = 0; i < coinCount; ++i)
        {
            const auto coin = Scene::GameObject::Instantiate(prefab, origin).lock();
            if (!coin)
                continue;

            const auto pickup = coin->Components().Catch<MoneyPickup>().lock();
            if (!pickup)
                continue;

            const float angle = start + step * static_cast<float>(i) + jitter(random);
            const int   value = baseValue + (i < remainder ? 1 : 0);
            pickup->Drop(GameCore::StatusParameter::Money(value), glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
        }
    }
}
