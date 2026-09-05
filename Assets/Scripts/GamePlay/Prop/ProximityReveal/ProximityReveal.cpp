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
        if (!renderer)
            return;

        // 定数バッファは ModelRenderer 側で遅延生成される(シェーダー未設定なら -1)
        const int cbHandle = renderer->GetOrCreateShaderConstantBufferHandle();
        if (cbHandle == -1)
            return;

        const auto player = GameCore::PlayerAvatar::Owner();
        if (!player)
            return;

        const glm::vec3 pp = player->PlayerTransform().GetWorldPos();

        auto* cb = static_cast<ProximityCB*>(GetBufferShaderConstantBuffer(cbHandle));
        if (!cb)
            return;

        cb->playerPos[0]      = pp.x;
        cb->playerPos[1]      = pp.y;
        cb->playerPos[2]      = pp.z;
        cb->revealRadius      = revealRadius_;
        cb->transitionWidth   = transitionWidth_;
        UpdateShaderConstantBuffer(cbHandle);
    }

    void ProximityReveal::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("revealRadius_",    revealRadius_);
        ImGuiHelper::OnDrawInputField("transitionWidth_", transitionWidth_);
    }
}
