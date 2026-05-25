#include "Prop_IslandPedestial.h"

#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"

namespace GamePlay::Prop
{
    void IslandPedestial::OnAwake()
    {
        
    }

    void IslandPedestial::OnStart()
    {
        collisionListener_->OnTriggerEnterAsObservable().subscribe(
            [this](const Component::CollisionListener::CollisionEnter collision)
        {
            const auto gameObject = collision.second;
            const auto weakPlayerAvatar = gameObject->Components().Catch<GameCore::IPlayerAvatar>();
            if (const auto playerAvatar = weakPlayerAvatar.lock())
            {
                playerAvatar->DisableStateMachine();
                Scene::GameObject::Instantiate(stageSelectUiPrefab_.get(), glm::vec3(0.0f, 0.0f, 0.0f));
            }
        });
    }

    void IslandPedestial::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("stageSelectUiPrefab_", stageSelectUiPrefab_);
        ImGuiHelper::OnDrawInputField("collisionListener_", collisionListener_);
    }
}
