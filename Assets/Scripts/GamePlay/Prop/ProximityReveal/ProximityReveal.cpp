#include "ProximityReveal.h"
#include "../../../../../Assets/Scripts/Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../../Engine/Module/GameObject/Transform/Transform.h"

namespace GamePlay::Prop
{
    void ProximityReveal::OnAwake()
    {
        modelRenderer_ = RequireComponent<Component::ModelRenderer>();
    }

    void ProximityReveal::OnUpdate()
    {
        const auto renderer = modelRenderer_.lock();
        if (!renderer || renderer->cbHandle_ == -1)
            return;

        const auto player = GameCore::PlayerAvatar::Owner();
        if (!player)
            return;

        const glm::vec3 pp = player->PlayerTransform().GetWorldPos();

        auto* cb              = static_cast<ProximityCB*>(GetBufferShaderConstantBuffer(renderer->cbHandle_));
        cb->playerPos[0]      = pp.x;
        cb->playerPos[1]      = pp.y;
        cb->playerPos[2]      = pp.z;
        cb->revealRadius      = revealRadius_;
        cb->transitionWidth   = transitionWidth_;
        UpdateShaderConstantBuffer(renderer->cbHandle_);
    }

    void ProximityReveal::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("revealRadius_",    revealRadius_);
        ImGuiHelper::OnDrawInputField("transitionWidth_", transitionWidth_);
    }
}
