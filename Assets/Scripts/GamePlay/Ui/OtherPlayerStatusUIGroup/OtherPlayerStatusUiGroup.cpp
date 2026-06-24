#include "OtherPlayerStatusUiGroup.h"

#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"

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

    void OtherPlayerStatusUiGroup::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("spacing_", spacing_);
    }
}
