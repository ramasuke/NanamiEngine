#include "GamePlay_MagicPlacement.h"

#include "../../../../../Engine/Core/Application/Time/Time.h"
#include "../../Spawn/GamePlay_PrefabSpawner.h"

namespace GamePlay::Magic
{
    void MagicPlacement::Place(const float lifeTime_secs)
    {
        remainingLifeTime_secs_ = lifeTime_secs;
        isPlaced_ = true;
    }

    void MagicPlacement::OnUpdate()
    {
        if (!isPlaced_)
            return;

        remainingLifeTime_secs_ -= Time::DeltaTime();
        if (remainingLifeTime_secs_ > 0.0f)
            return;

        isPlaced_ = false;
        if (vanishPrefab_)
            Spawn::SpawnPrefab(*vanishPrefab_.get(), Transform().GetWorldPos(), vanishEffectLifeTime_secs_);

        if (const auto entity = Entity().lock())
            entity->OnDestroy();
    }

    void MagicPlacement::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("vanishPrefab_", vanishPrefab_);
        ImGuiHelper::OnDrawInputField("vanishEffectLifeTime_secs_", vanishEffectLifeTime_secs_);
        ImGui::Text("placed: %s  remaining: %.2f", isPlaced_ ? "true" : "false", remainingLifeTime_secs_);
    }
}
