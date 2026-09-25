#include "PlaceBombEffect.h"

#include "geometric.hpp"
#include "../ItemEffectFactory.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../../../../../GamePlay/Magic/GamePlay_MagicAim.h"
#include "../../../../../../GamePlay/Magic/Component/GamePlay_MagicBlast.h"

namespace GameCore::PlayerAvatar::Item
{
    void PlaceBombEffect::Apply(IItemEffectTarget&, const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& user) const
    {
        if (!bombPrefab_ || !user)
            return;

        const auto& userTransform = user->Transform();
        const glm::quat userRotation = userTransform.GetWorldRot();

        glm::vec3 forward = userRotation * glm::vec3(0.0f, 0.0f, -1.0f);
        forward.y = 0.0f;
        const float length = glm::length(forward);
        forward = length > 0.0001f ? forward / length : glm::vec3(0.0f, 0.0f, -1.0f);

        const glm::vec3 position = GamePlay::Magic::ProjectToGround(userTransform.GetWorldPos() + forward * distance_);
        const glm::quat rotation = GamePlay::Magic::LookRotation(glm::vec3(0.0f), forward, userRotation);

        const auto bomb = Scene::GameObject::Instantiate(*bombPrefab_.get(), position, rotation).lock();
        if (!bomb)
            return;

        if (const auto blast = bomb->Components().Catch<GamePlay::Magic::MagicBlast>().lock())
            blast->Arm(user, power_, fuse_secs_);
    }

    void PlaceBombEffect::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("bombPrefab_", bombPrefab_);
        LibCore::ImGuiHelper::OnDrawInputField("power_", power_);
        LibCore::ImGuiHelper::OnDrawInputField("fuse_secs_", fuse_secs_);
        LibCore::ImGuiHelper::OnDrawInputField("distance_", distance_);
    }

    REGISTER_ITEM_EFFECT(PlaceBombEffect)
}

NANAMI_REGISTER_TYPE(GameCore::PlayerAvatar::Item::PlaceBombEffect, GameCore::PlayerAvatar::Item::IItemEffect);
