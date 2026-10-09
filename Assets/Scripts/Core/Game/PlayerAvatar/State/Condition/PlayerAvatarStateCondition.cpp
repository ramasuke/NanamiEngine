#include "PlayerAvatarStateCondition.h"

#include "Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../../../GamePlay/PlayerAvatar/InteractableArea/InteractableArea.h"
#include "../../../../../GamePlay/PlayerAvatar/WakeUpArea/WakeUpArea.h"
#include "../../../../../GamePlay/Prop/Canon/Prop_Canon.h"
#include "../../../../../GamePlay/Ui/NpcChatting/Ui_NpcChatting.h"
#include "../../../Game.h"
#include "../../../Scene/Main/Content/FirstTouchDownMainIsLand/Context/FirstTouchDownMainIsLandSceneContext.h"
#include "../../../Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Scene/Sub/Content/ChattingUI/ChattingUIScene.h"
#include "../../../Scene/Sub/Group/Sub_GameSceneGroup.h"
#include "../../../Scene/Sub/Type/SubSceneType.h"

namespace GameCore::PlayerAvatar::State
{
    PlayerAvatarStateCondition::PlayerAvatarStateCondition(
        const std::shared_ptr<IPlayerAvatarStateContext>& stateContext)
        : stateContext_(stateContext)
    {
    }

    bool PlayerAvatarStateCondition::IsGround() const
    {
        return IsGround(stateContext_->GroundCheckRadius());
    }

    bool PlayerAvatarStateCondition::IsGround(const float radius) const
    {
        Physics::LayerMask mask;
        Physics::AddLayer(mask, Physics::Layer::Default);

        // NOTE: Rayだと段差の縁や地形の隙間で抜けて Floating になるため球判定
        return Physics::SphereCast(stateContext_->PlayerAvatarFeatStepPos() + glm::vec3(0.0f, stateContext_->GroundCheckUpOffset() + radius, 0.0f),
                                   radius,
                                   glm::vec3(0, -1, 0), stateContext_->GroundCheckDistance(),
                                   mask).Hit();
    }

    bool PlayerAvatarStateCondition::IsInteractable() const
    {
        // NOTE: 会話UIは全NPCで共有しているため、表示中に別NPCと会話を始めると文章が重なる
        const auto& subScenes = GameCore::Game::Instance().SubScenes();
        if (const auto& chattingUIScene = subScenes.Catch<GameCore::Scene::Sub::ChattingUIScene>(GameCore::Scene::Sub::SceneType::ChattingUI);
            chattingUIScene && chattingUIScene->Context().Npc().IsDisplaying())
            return false;

        return !stateContext_->InteractableArea().CatchInteractTarget().expired();
    }

    bool PlayerAvatarStateCondition::CanWakeUp() const
    {
        return !stateContext_->WakeUpArea().CatchWakeUpTarget().expired();
    }

    bool PlayerAvatarStateCondition::CanUseCannon() const
    {
        // NOTE: 乗る要求は大砲側がインタラクトで立てる
        const auto sceneContext = Game::Instance().Scenes().CatchContext<Scene::FirstTouchDownMainIsLandSceneContext>();
        // NOTE: 大砲のないシーンでは未設定
        if (!sceneContext || !sceneContext->HasPlayerControllabeCanon())
            return false;

        const auto& canon = sceneContext->PlayerControllabeCanon();
        return canon.IsBoardRequested() && !canon.IsLocked();
    }
}
