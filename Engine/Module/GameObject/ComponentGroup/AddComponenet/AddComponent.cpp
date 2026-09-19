#include "AddComponent.h"
#include "../ComponentHeaders.h"
#include "../../../../../Assets/Scripts/Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/DealDamageTextBillBoard/UI_DealDamageTextBillBoard.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/StageSelect/UI_StageSelect.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/CharacterSelect/UI_CharacterSelect.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/CharacterSelect/Row/Ui_CharacterSelect_Row.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/CharacterSelect/Presenter/CharacterSelectPresenter.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/GameOver/Ui_GameOverButton.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/GameOver/Ui_GameOverScreen.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/GameOver/DeathCamera/GameOverDeathCamera.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/GameOver/Presenter/GameOverPresenter.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/PauseMenu/Ui_PauseMenu.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/PauseMenu/Presenter/PauseMenuPresenter.h"
#include "../../../../../Assets/Scripts/GamePlay/Prop/CharacterPodium/Prop_CharacterPodium.h"
#include "../../../../../Assets/Scripts/GamePlay/Prop/EventNoticeBoard/Prop_EventNoticeBoard.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/EventBoard/UI_EventBoard.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/EventBoard/Row/Ui_EventBoard_Row.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/EventBoard/Presenter/EventBoardPresenter.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/Shop/UI_Shop.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/Shop/Row/Ui_ShopRow.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/Shop/Receipt/Ui_ShopReceipt.h"
#include "../../../../../Assets/Scripts/GamePlay/Ui/Shop/Presenter/ShopPresenter.h"
#include "../../../../../Assets/Scripts/GamePlay/Prop/MerchantStall/Prop_MerchantStall.h"

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
        OnDrawNetworkGui    (addComponent);
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
        OnDrawTryAddComponentGui<Component::QuadRenderer   >(addComponent);
        OnDrawTryAddComponentGui<Component::Animator       >(addComponent);
        OnDrawTryAddComponentGui<Component::BoneSync       >(addComponent);
        OnDrawTryAddComponentGui<Component::ParticleSystem >(addComponent);
        OnDrawTryAddComponentGui<Component::DirectionLight >(addComponent);
        OnDrawTryAddComponentGui<Component::SkyDome3D      >(addComponent);
        OnDrawTryAddComponentGui<Component::Rotator        >(addComponent);
        OnDrawTryAddComponentGui<Component::CameraFollowTransform>(addComponent);
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
        OnDrawTryAddComponentGui<NanamiUi::CircleGaugeRenderer   >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::BlendAnimationRenderer>(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::ImageAnimationRenderer>(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::Slider                >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::TextRenderer          >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::Billboard3D           >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::BillboardAnimation3D  >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::Button                >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::MovieRenderer         >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::GridLayoutGroup       >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::HorizontalLayoutGroup >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::VerticalLayoutGroup   >(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawColliderGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Collider"))
    {
        OnDrawTryAddComponentGui<Component::RigidBody         >(addComponent);
        OnDrawTryAddComponentGui<Component::BoxCollider       >(addComponent);
        OnDrawTryAddComponentGui<Component::SphereCollider    >(addComponent);
        OnDrawTryAddComponentGui<Component::CapsuleCollider   >(addComponent);
        OnDrawTryAddComponentGui<Component::CylinderCollider  >(addComponent);
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

void GameObject::AddComponent::OnDrawNetworkGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Network"))
    {
        if (ImGui::TreeNode("Component"))
        {
            OnDrawTryAddComponentGui<Network::NetworkGameObject>(addComponent);
            OnDrawTryAddComponentGui<Network::NetworkTransform >(addComponent);
            OnDrawTryAddComponentGui<Network::NetworkAnimator  >(addComponent);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        
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
                OnDrawTryAddComponentGui<GameCore::Scene::GrassLandSceneContext>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            if (ImGui::TreeNode("Sub"))
            {
                OnDrawTryAddComponentGui<GameCore::Scene::Sub::ChattingUISceneContext>(addComponent);
                OnDrawTryAddComponentGui<GameCore::Scene::Sub::OtherPlayerStatusUiSceneContext>(addComponent);
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
        if (ImGui::TreeNode("Network"))
        {
            OnDrawTryAddComponentGui<GamePlay::Network::CustomNetworkRunner>(addComponent);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        
        if (ImGui::TreeNode("UI"))
        {
            OnDrawTryAddComponentGui<GamePlay::Ui::NpcChatting         >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::BillBoardNpcChatIcon>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::SampleTitleLogo >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::PlayerStatus    >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::SampleTitleScene>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::SwordManActionInstructTutorial>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::StageSelectUi>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::StageSelectStageUi>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::StageSelectPresenter>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::StageMapMarker>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::StageDifficultyPips>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::CharacterSelectUi>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::CharacterSelectRow>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::CharacterSelectPresenter>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::EventBoardUi>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::EventBoardRow>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::EventBoardPresenter>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::ShopUi>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::ShopRow>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::ShopReceipt>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::ShopPresenter>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::PauseMenuUi>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::PauseMenuRow>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::PauseMenuItemCell>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::PauseMenuPresenter>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::LoadingScreenUi>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::LoadingHintCard>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::GameOverScreenUi>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::GameOverButton>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::GameOverPresenter>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::GameOverDeathCamera>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::DealDamageTextBillBoard>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::OtherPlayerStatusUiGroup>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::SpellPalette>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Ui::SpellSlot>(addComponent);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        if (ImGui::TreeNode("PlayerAvatar"))
        {
            if (ImGui::TreeNode("Swordman"))
            {
                OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar           >(addComponent);
                OnDrawTryAddComponentGui<GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup>(addComponent);
                OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::OtherPlayer::StatusPresenter>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();    
            }
            OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::ChattableArea>(addComponent);
            OnDrawTryAddComponentGui<GameCore::PlayerAvatar::PlayerAttackArea   >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::LockOnDetectionArea>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::PlayerHitShakeReceiver>(addComponent);
            if (ImGui::TreeNode("Bullet"))
            {
                OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::Bullet::CannonBullet>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            if (ImGui::TreeNode("Magic"))
            {
                OnDrawTryAddComponentGui<GamePlay::Magic::MagicProjectile>(addComponent);
                OnDrawTryAddComponentGui<GamePlay::Magic::MagicBlast     >(addComponent);
                OnDrawTryAddComponentGui<GamePlay::Magic::MagicPlacement >(addComponent);
                OnDrawTryAddComponentGui<GamePlay::Magic::MagicChannel   >(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }

            if (ImGui::TreeNode("MagicCaster"))
            {
                OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::MagicCaster::StatusPresenter>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            if (ImGui::TreeNode("OtherPlayer"))
            {
                OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::SwordMan::StatusPresenter>(addComponent);
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
                OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::Hyena>(addComponent);
                OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::Tyrannosaurus>(addComponent);
                OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::NetworkBehaviourTree>(addComponent);
                OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::BodyPartWeakPoint>(addComponent);
                if (ImGui::TreeNode("Attack"))
                {
                    OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::AttackProjectile>(addComponent);
                    OnDrawTryAddComponentGui<GameCore::Npc::Enemy::AttackArea>(addComponent);
                    ImGui::TreePop();
                    ImGui::Spacing();
                }
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
            OnDrawTryAddComponentGui<GamePlay::Prop::CharacterPodium>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Prop::MerchantStall>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Prop::EventNoticeBoard>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Prop::DestructibleObject>(addComponent);
            if (ImGui::TreeNode("Grass"))
            {
                OnDrawTryAddComponentGui<GamePlay::Prop::Grassable    >(addComponent);
                OnDrawTryAddComponentGui<GamePlay::Prop::GrassRenderer>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }
        if (ImGui::TreeNode("Shader"))
        {
            OnDrawTryAddComponentGui<GamePlay::Prop::ProximityReveal     >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Prop::LatticeBarrierEffect>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Prop::CloudEffect         >(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Prop::TreeLeafSway        >(addComponent);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        if (ImGui::TreeNode("Environment"))
        {
            OnDrawTryAddComponentGui<GamePlay::Weather::WindZone>(addComponent);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        if (ImGui::TreeNode("Sound"))
        {
            OnDrawTryAddComponentGui<GamePlay::Sound::SoundPlayer>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Sound::BgmPlayObject>(addComponent);
            OnDrawTryAddComponentGui<GamePlay::Sound::SpawnSound>(addComponent);
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
