#include "AddComponent.h"
#include "../ComponentHeaders.h"
#include "../../../../../Assets/Scripts/Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"

std::shared_ptr<Component::ComponentBase> GameObject::AddComponent::OnDrawGui()
{
    if (ImGui::Button("Add Component"))
    {
        ImGui::OpenPopup("Add Component Menu");
    }

    std::shared_ptr<Component::ComponentBase> addComponent;
    if (ImGui::BeginPopup("Add Component Menu"))
    {
        OnDrawRendererGui   (addComponent);
        OnDrawUiRendererGui (addComponent);
        OnDrawSoundGui      (addComponent);
        OnDrawColliderGui   (addComponent);
        OnDrawCinemachineGui(addComponent);
        OnDrawGameCoreGui   (addComponent);
        OnDrawGamePlayGui   (addComponent);
        ImGui::EndPopup();
        ImGui::Spacing();
    }
    
    return addComponent;
}

void GameObject::AddComponent::OnDrawRendererGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Renderer"))
    {
        OnDrawTryAddComponentGui<Component::SphereRenderer >(addComponent);
        OnDrawTryAddComponentGui<Component::ModelRenderer  >(addComponent);
        OnDrawTryAddComponentGui<Component::Animator       >(addComponent);
        OnDrawTryAddComponentGui<Component::ParticleSystem >(addComponent);
        OnDrawTryAddComponentGui<Component::DirectionLight >(addComponent);
        OnDrawTryAddComponentGui<Component::SkyDome3D      >(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawSoundGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Sound"))
    {
        OnDrawTryAddComponentGui<Component::AudioSource        >(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawUiRendererGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("UiRenderer"))
    {
        OnDrawTryAddComponentGui<Component::ImageRenderer        >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::BlendImageRenderer    >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::BlendAnimationRenderer>(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::ImageAnimationRenderer>(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::Slider                >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::TextRenderer          >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::Billboard3D           >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::Button                >(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawColliderGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Collider"))
    {
        OnDrawTryAddComponentGui<Component::BoxCollider       >(addComponent);
        OnDrawTryAddComponentGui<Component::SphereCollider    >(addComponent);
        OnDrawTryAddComponentGui<Component::CapsuleCollider   >(addComponent);
        OnDrawTryAddComponentGui<Component::StaticMeshCollider>(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
    if (ImGui::TreeNode("Collision"))
    {
        OnDrawTryAddComponentGui<Component::CollisionListener >(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawCinemachineGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Cinema chineCamera"))
    {
        OnDrawTryAddComponentGui<CineMachine::CinemachineCameraBrain  >(addComponent);
        OnDrawTryAddComponentGui<CineMachine::CineMachineVirtualCamera>(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawGameCoreGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("GameCore"))
    {
        
        OnDrawTryAddComponentGui<GameCore::Game>(addComponent);
        if (ImGui::TreeNode("Scene"))
        {
            if (ImGui::TreeNode("Main"))
            {
                OnDrawTryAddComponentGui<GameCore::Scene::TitleSceneContext>(addComponent);
                OnDrawTryAddComponentGui<GameCore::Scene::FirstTouchDownMainIsLandSceneContext>(addComponent);
                OnDrawTryAddComponentGui<GameCore::Scene::MainIslandSceneContext>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            if (ImGui::TreeNode("Sub"))
            {
                OnDrawTryAddComponentGui<GameCore::Scene::Sub::ChattingUISceneContext>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawGamePlayGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("GamePlay"))
    {
        if (ImGui::TreeNode("UI"))
        {
            OnDrawTryAddComponentGui<GamePlay::Ui::NpcChatting         >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::BillBoardNpcChatIcon>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::SampleTitleLogo >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::PlayerStatus    >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::SampleTitleScene>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::SwordManActionInstructTutorial>(addComponent);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        if (ImGui::TreeNode("PlayerAvatar"))
        {
            if (ImGui::TreeNode("Swordman"))
            {
                OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar           >(addComponent);
                OnDrawTryAddComponentGui<GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();    
            }
            OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::ChattableArea>(addComponent);
            OnDrawTryAddComponentGui<GameCore::PlayerAvatar::PlayerAttackArea   >(addComponent);
            if (ImGui::TreeNode("Bullet"))
            {
                OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::Bullet::CannonBullet>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }
        if (ImGui::TreeNode("Npc"))
        {
            if (ImGui::TreeNode("Friendly"))
            {
                OnDrawTryAddComponentGui<GamePlay::Npc::Friendly::FriendlyNpc>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            if (ImGui::TreeNode("Enemy"))
            {
                OnDrawTryAddComponentGui<GameCore::Npc::Enemy::SampleEnemy>(addComponent);
                OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::TrainingDummy>(addComponent);
                OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::FirstEventDragon>(addComponent);
                OnDrawTryAddComponentGui<GameCore::Npc::Enemy::AttackArea>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }

        if (ImGui::TreeNode("Prop"))
        {
            OnDrawTryAddComponentGui<GamePlay::Prop::AirShip        >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Prop::Canon          >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Prop::IslandPedestial>(addComponent);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        if (ImGui::TreeNode("Sound"))
        {
            OnDrawTryAddComponentGui<GamePlay::Sound::SoundPlayer>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Sound::BgmPlayObject>(addComponent);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

std::string GameObject::AddComponent::StripNamespace(const std::string& name)
{
    const auto pos = name.rfind("::");
    return pos != std::string::npos ? name.substr(pos + 2) : name;
}
