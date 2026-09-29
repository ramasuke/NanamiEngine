#include "Prop_IslandPedestial.h"

#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"
#include "Packages/ControlLock/ControlLock.h"

namespace GamePlay::Prop
{
    void IslandPedestial::OnAwake()
    {
        
    }

    void IslandPedestial::OnStart()
    {
        collisionListener_->OnTriggerEnterAsObservable().Subscribe(
            [this](const Component::CollisionListener::CollisionEnter collision)
        {
            const auto gameObject = collision.second;
            const auto weakPlayerAvatar = gameObject->Components().Catch<GameCore::IPlayerAvatar>();
                
            const auto playerAvatar = weakPlayerAvatar.lock();
            if (!playerAvatar || !playerAvatar->IsOwner())
                return;

            const auto ui = Scene::GameObject::Instantiate(stageSelectUiPrefab_.get(), glm::vec3(0.0f, 0.0f, 0.0f)).lock();
            if (!ui)
                return;

            // NOTE: 閉じるか、出発でシーンごと片付くまで操作を止める
            if (const auto lifetime = ui->Components().Catch<Component::ComponentBase>().lock())
            {
                NanamiEngine::ControlLock::Service::Instance().Acquire().AddTo(*lifetime);
            }
                
        }).AddTo(this);
    }

    void IslandPedestial::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("stageSelectUiPrefab_", stageSelectUiPrefab_);
        ImGuiHelper::OnDrawInputField("collisionListener_", collisionListener_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::IslandPedestial);
#pragma endregion
