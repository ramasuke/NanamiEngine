#include "Engine/Module/GameObject/ComponentGroup/AddComponenet/AddComponent.h"
#include "../../Core/Game/Game.h"
#include "../../Core/Game/Npc/Enemy/AttackArea/Enemy_AttackArea.h"
#include "../../Core/Game/Npc/Enemy/Content/SampleEnemy/SampleEnemy.h"
#include "../../Core/Game/PlayerAvatar/Status/Presenter/PlayerAvatar_OtherPlayer_StatusPresenter.h"
#include "../../Core/Game/PlayerAvatar/SwordMan/Status/Presenter/PlayerAvatar_SwordMan_StatusPresenter.h"
#include "../../Core/Game/PlayerAvatar/MagicCaster/Status/Presenter/PlayerAvatar_MagicCaster_StatusPresenter.h"
#include "../../Core/Game/Scene/Main/Content/FirstTouchDownMainIsLand/Context/FirstTouchDownMainIsLandSceneContext.h"
#include "../../Core/Game/Scene/Main/Content/GrassLand/Context/GrassLandSceneContext.h"
#include "../../Core/Game/Scene/Main/Content/MainIslandScene/Context/MainIsLandSceneContext.h"
#include "../../Core/Game/Scene/Main/Content/Title/Context/TitleSceneContext.h"
#include "../../Core/Game/Scene/Sub/Content/ChattingUI/Context/ChattingUISceneContext.h"
#include "../../Core/Game/Scene/Sub/Content/OtherPlayerStatusUI/Context/OtherPlayerStatusUiSceneContext.h"
#include "../../GamePlay/Network/Game_CustomNetworkRunner.h"
#include "../../GamePlay/Npc/Enemy/BodyPart/GamePlay_Enemy_BodyPartWeakPoint.h"
#include "../../GamePlay/Npc/Enemy/FirstEventDragon/GamePlay_Enemy_FirstEventDragon.h"
#include "../../GamePlay/Npc/Enemy/Hyena/GamePlay_Enemy_Hyena.h"
#include "../../GamePlay/Npc/Enemy/NetworkBehaviourTree/GamePlay_NetworkBehaviourTree.h"
#include "../../GamePlay/Npc/Enemy/Projectile/GamePlay_Enemy_ProjectileAttackArea.h"
#include "../../GamePlay/Npc/Enemy/TrainingDummy/TrainingDummy.h"
#include "../../GamePlay/Npc/Enemy/Tyrannosaurus/GamePlay_Enemy_Tyrannosaurus.h"
#include "../../GamePlay/Npc/Friendly/FriendlyNpc.h"
#include "../../GamePlay/Magic/Component/GamePlay_MagicBlast.h"
#include "../../GamePlay/Magic/Component/GamePlay_MagicChannel.h"
#include "../../GamePlay/Magic/Component/GamePlay_MagicPlacement.h"
#include "../../GamePlay/Magic/Component/GamePlay_MagicProjectile.h"
#include "../../GamePlay/PlayerAvatar/Bullet/PlayerAvatar_Bullet_CannonBullet.h"
#include "../../GamePlay/PlayerAvatar/InteractableArea/InteractableArea.h"
#include "../../GamePlay/PlayerAvatar/HitShakeReceiver/PlayerHitShakeReceiver.h"
#include "../../GamePlay/PlayerAvatar/LockOnDetectionArea/LockOnDetectionArea.h"
#include "../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../GamePlay/Prop/AirShip/Prop_AirShip.h"
#include "../../GamePlay/Prop/ProximityReveal/ProximityReveal.h"
#include "../../GamePlay/Prop/Canon/Prop_Canon.h"
#include "../../GamePlay/Prop/DestructibleObject/DestructibleObject.h"
#include "../../GamePlay/Prop/Grass/Grassable.h"
#include "../../GamePlay/Prop/Grass/GrassRenderer.h"
#include "../../GamePlay/Prop/IslandPedestial/Prop_IslandPedestial.h"
#include "../../GamePlay/Prop/Cloud/CloudEffect.h"
#include "../../GamePlay/Prop/LatticeBarrier/LatticeBarrierEffect.h"
#include "../../GamePlay/Prop/Tree/TreeLeafSway.h"
#include "../../GamePlay/Weather/WindZone.h"
#include "../../GamePlay/Sound/SoundPlayer.h"
#include "../../GamePlay/Sound/SpawnSound.h"
#include "../../GamePlay/Sound/Sample/BgmPlayer.h"
#include "../../GamePlay/Ui/ActionInstructTutorial/SwordMan/Ui_SwordMan_ActionInstructTutorial.h"
#include "../../GamePlay/Ui/BillBoardNpcChatIcon/BillBoardNpcChatIcon.h"
#include "../../GamePlay/Ui/DealDamageTextBillBoard/UI_DealDamageTextBillBoard.h"
#include "../../GamePlay/Ui/NpcChatting/Ui_NpcChatting.h"
#include "../../GamePlay/Ui/OtherPlayerStatusUIGroup/OtherPlayerStatusUiGroup.h"
#include "../../GamePlay/Ui/Sample/UI_SampleTitleLogo.h"
#include "../../GamePlay/Ui/SampleTitleSceneUI/UI_SampleTitleSceneUI.h"
#include "../../GamePlay/Ui/SpellPalette/Ui_SpellPalette.h"
#include "../../GamePlay/Ui/SpellPalette/Ui_SpellSlot.h"
#include "../../GamePlay/Ui/Loading/Hint/Ui_LoadingHintCard.h"
#include "../../GamePlay/Ui/Loading/Map/Ui_LoadingRouteMap.h"
#include "../../GamePlay/Ui/Loading/Ui_LoadingScreen.h"
#include "../../GamePlay/Ui/StageSelect/Difficulty/StageDifficultyPips.h"
#include "../../GamePlay/Ui/StageSelect/MapMarker/StageMapMarker.h"
#include "../../GamePlay/Ui/StageSelect/Presenter/StageSelectPresenter.h"
#include "../../GamePlay/Ui/StageSelect/Stage/Ui_StageSelect_StageUI.h"
#include "../../GamePlay/Ui/StageSelect/UI_StageSelect.h"
#include "../../Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"
#include "../../GamePlay/Ui/CharacterSelect/UI_CharacterSelect.h"
#include "../../GamePlay/Ui/CharacterSelect/Row/Ui_CharacterSelect_Row.h"
#include "../../GamePlay/Ui/CharacterSelect/Presenter/CharacterSelectPresenter.h"
#include "../../GamePlay/Ui/GameOver/Ui_GameOverButton.h"
#include "../../GamePlay/Ui/GameOver/Ui_GameOverScreen.h"
#include "../../GamePlay/Ui/GameOver/DeathCamera/GameOverDeathCamera.h"
#include "../../GamePlay/Ui/GameOver/Presenter/GameOverPresenter.h"
#include "../../GamePlay/Ui/PauseMenu/Ui_PauseMenu.h"
#include "../../GamePlay/Ui/PauseMenu/Presenter/PauseMenuPresenter.h"
#include "../../GamePlay/Prop/CharacterPodium/Prop_CharacterPodium.h"
#include "../../GamePlay/Prop/EventNoticeBoard/Prop_EventNoticeBoard.h"
#include "../../GamePlay/Prop/HerbPatch/Prop_HerbPatch.h"
#include "../../GamePlay/Prop/ChargeStuckObstacle/GamePlay_ChargeStuckObstacle.h"
#include "../../GamePlay/Prop/TreasureChest/Prop_TreasureChest.h"
#include "../../GamePlay/Ui/EventBoard/UI_EventBoard.h"
#include "../../GamePlay/Ui/EventBoard/Row/Ui_EventBoard_Row.h"
#include "../../GamePlay/Ui/EventBoard/Presenter/EventBoardPresenter.h"
#include "../../GamePlay/Ui/Shop/UI_Shop.h"
#include "../../GamePlay/Ui/Shop/Row/Ui_ShopRow.h"
#include "../../GamePlay/Ui/Shop/Receipt/Ui_ShopReceipt.h"
#include "../../GamePlay/Ui/Shop/Presenter/ShopPresenter.h"
#include "../../GamePlay/Prop/MerchantStall/Prop_MerchantStall.h"

// NOTE: unity build での名前衝突を避けるため無名名前空間にしない
namespace Editor::AddComponentMenu
{
    using NanamiEngine::Module::GameObject::AddComponent;
    using ComponentPtr = std::shared_ptr<NanamiEngine::Module::Component::ComponentBase>;

    void OnDrawGameCoreGui(ComponentPtr& addComponent)
    {
        if (ImGui::TreeNode("GameCore"))
        {
            AddComponent::OnDrawTryAddComponentGui<GameCore::Game>(addComponent);
            if (ImGui::TreeNode("Scene"))
            {
                if (ImGui::TreeNode("Main"))
                {
                    AddComponent::OnDrawTryAddComponentGui<GameCore::Scene::TitleSceneContext>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GameCore::Scene::FirstTouchDownMainIsLandSceneContext>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GameCore::Scene::MainIslandSceneContext>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GameCore::Scene::GrassLandSceneContext>(addComponent);
                    ImGui::TreePop();
                    ImGui::Spacing();
                }
                if (ImGui::TreeNode("Sub"))
                {
                    AddComponent::OnDrawTryAddComponentGui<GameCore::Scene::Sub::ChattingUISceneContext>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GameCore::Scene::Sub::OtherPlayerStatusUiSceneContext>(addComponent);
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

    void OnDrawGamePlayGui(ComponentPtr& addComponent)
    {
        if (ImGui::TreeNode("GamePlay"))
        {
            if (ImGui::TreeNode("Network"))
            {
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Network::CustomNetworkRunner>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            
            if (ImGui::TreeNode("UI"))
            {
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::NpcChatting         >(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::BillBoardNpcChatIcon>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::SampleTitleLogo >(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::PlayerStatus    >(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::SampleTitleScene>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::SwordManActionInstructTutorial>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::StageSelectUi>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::StageSelectStageUi>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::StageSelectPresenter>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::StageMapMarker>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::StageDifficultyPips>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::CharacterSelectUi>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::CharacterSelectRow>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::CharacterSelectPresenter>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::EventBoardUi>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::EventBoardRow>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::EventBoardPresenter>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::ShopUi>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::ShopRow>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::ShopReceipt>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::ShopPresenter>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::PauseMenuUi>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::PauseMenuRow>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::PauseMenuItemCell>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::PauseMenuPresenter>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::LoadingScreenUi>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::LoadingHintCard>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::LoadingRouteMap>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::GameOverScreenUi>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::GameOverButton>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::GameOverPresenter>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::GameOverDeathCamera>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::DealDamageTextBillBoard>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::OtherPlayerStatusUiGroup>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::SpellPalette>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Ui::SpellSlot>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            if (ImGui::TreeNode("PlayerAvatar"))
            {
                if (ImGui::TreeNode("Swordman"))
                {
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar           >(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GameCore::PlayerAvatar::SwordMan::SwordManAvatarCameraGroup>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::OtherPlayer::StatusPresenter>(addComponent);
                    ImGui::TreePop();
                    ImGui::Spacing();    
                }
                AddComponent::OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::InteractableArea>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GameCore::PlayerAvatar::PlayerAttackArea   >(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::LockOnDetectionArea>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::PlayerHitShakeReceiver>(addComponent);
                if (ImGui::TreeNode("Bullet"))
                {
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::Bullet::CannonBullet>(addComponent);
                    ImGui::TreePop();
                    ImGui::Spacing();
                }
                if (ImGui::TreeNode("Magic"))
                {
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Magic::MagicProjectile>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Magic::MagicBlast     >(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Magic::MagicPlacement >(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Magic::MagicChannel   >(addComponent);
                    ImGui::TreePop();
                    ImGui::Spacing();
                }

                if (ImGui::TreeNode("MagicCaster"))
                {
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::MagicCaster::StatusPresenter>(addComponent);
                    ImGui::TreePop();
                    ImGui::Spacing();
                }
                if (ImGui::TreeNode("OtherPlayer"))
                {
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::PlayerAvatar::SwordMan::StatusPresenter>(addComponent);
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
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Npc::Friendly::FriendlyNpc>(addComponent);
                    ImGui::TreePop();
                    ImGui::Spacing();
                }
                if (ImGui::TreeNode("Enemy"))
                {
                    AddComponent::OnDrawTryAddComponentGui<GameCore::Npc::Enemy::SampleEnemy>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::TrainingDummy>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::FirstEventDragon>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::Hyena>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::Tyrannosaurus>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::NetworkBehaviourTree>(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::BodyPartWeakPoint>(addComponent);
                    if (ImGui::TreeNode("Attack"))
                    {
                        AddComponent::OnDrawTryAddComponentGui<GamePlay::Npc::Enemy::AttackProjectile>(addComponent);
                        AddComponent::OnDrawTryAddComponentGui<GameCore::Npc::Enemy::AttackArea>(addComponent);
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
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::AirShip        >(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::Canon          >(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::IslandPedestial>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::CharacterPodium>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::MerchantStall>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::EventNoticeBoard>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::HerbPatch>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::TreasureChest>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::DestructibleObject>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::ChargeStuckObstacle>(addComponent);
                if (ImGui::TreeNode("Grass"))
                {
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::Grassable    >(addComponent);
                    AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::GrassRenderer>(addComponent);
                    ImGui::TreePop();
                    ImGui::Spacing();
                }
                ImGui::TreePop();
                ImGui::Spacing();
            }
            if (ImGui::TreeNode("Shader"))
            {
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::ProximityReveal     >(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::LatticeBarrierEffect>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::CloudEffect         >(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Prop::TreeLeafSway        >(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            if (ImGui::TreeNode("Environment"))
            {
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Weather::WindZone>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            if (ImGui::TreeNode("Sound"))
            {
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Sound::SoundPlayer>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Sound::BgmPlayObject>(addComponent);
                AddComponent::OnDrawTryAddComponentGui<GamePlay::Sound::SpawnSound>(addComponent);
                ImGui::TreePop();
                ImGui::Spacing();
            }
            ImGui::TreePop();
            ImGui::Spacing();
        }
    }

    void OnDrawGameMenuGui(ComponentPtr& addComponent)
    {
        OnDrawGameCoreGui(addComponent);
        OnDrawGamePlayGui(addComponent);
    }

    const bool registered = AddComponent::RegisterMenu(&OnDrawGameMenuGui);
}
