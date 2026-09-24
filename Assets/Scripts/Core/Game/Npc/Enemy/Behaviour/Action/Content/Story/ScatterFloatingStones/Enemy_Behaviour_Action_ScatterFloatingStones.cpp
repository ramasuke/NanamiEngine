#include "Enemy_Behaviour_Action_ScatterFloatingStones.h"
#include "Engine/Core/Coroutine/Coroutine.h"
#include "../../../../../../../Story/FloatingStone/Story_FloatingStoneMovie.h"
#include "../../../../../../../../../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::ScatterFloatingStones::DoTick(const TickContext& context)
    {
        if (!stonesRoot_)
            return TickStatus::Failure;

        Coroutine::StartCoroutine(GameCore::Story::FloatingStone::PlayScatterAsync(
            stonesRoot_.get(),
            GameCore::Story::FloatingStone::ScatterShot{ riseHeight_, riseSeconds_, hoverSeconds_, flySeconds_, flyDistance_, flyRise_ }));
        return TickStatus::Success;
    }

    void Action::ScatterFloatingStones::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("stonesRoot_", stonesRoot_);
        ImGuiHelper::OnDrawInputField("riseHeight_", riseHeight_);
        ImGuiHelper::OnDrawInputField("riseSeconds_", riseSeconds_);
        ImGuiHelper::OnDrawInputField("hoverSeconds_", hoverSeconds_);
        ImGuiHelper::OnDrawInputField("flySeconds_", flySeconds_);
        ImGuiHelper::OnDrawInputField("flyDistance_", flyDistance_);
        ImGuiHelper::OnDrawInputField("flyRise_", flyRise_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ScatterFloatingStones);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::ScatterFloatingStones);
#pragma endregion
