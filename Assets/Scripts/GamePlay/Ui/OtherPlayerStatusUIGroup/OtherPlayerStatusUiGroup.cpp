#include "OtherPlayerStatusUiGroup.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void OtherPlayerStatusUiGroup::AddPlayerStatus(const std::weak_ptr<PlayerStatus>& playerStatus)
    {
        const auto locked = playerStatus.lock();
        if (!locked)
            return;

        const glm::vec3 basePos = Transform().GetLocalPos();
        locked->Transform().SetLocalPos(basePos + spacing_ * static_cast<float>(playerStatuses_.size()));

        playerStatuses_.push_back(playerStatus);
    }

    void OtherPlayerStatusUiGroup::RemovePlayerStatus(const std::weak_ptr<PlayerStatus>& playerStatus)
    {
        const auto target = playerStatus.lock();
        std::erase_if(playerStatuses_, [&](const std::weak_ptr<PlayerStatus>& weak)
        {
            const auto locked = weak.lock();
            return !locked || locked == target;
        });
        Relayout();
    }

    void OtherPlayerStatusUiGroup::Relayout()
    {
        const glm::vec3 basePos = Transform().GetLocalPos();
        for (size_t i = 0; i < playerStatuses_.size(); ++i)
        {
            if (const auto locked = playerStatuses_[i].lock())
                locked->Transform().SetLocalPos(basePos + spacing_ * static_cast<float>(i));
        }
    }

    void OtherPlayerStatusUiGroup::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("spacing_", spacing_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::OtherPlayerStatusUiGroup);
#pragma endregion
