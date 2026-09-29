#include "Ui_NavigationPresenter.h"

#include <chrono>

#include "Ui_NavigationMemory.h"
#include "../BillBoardNpcChatIcon/BillBoardNpcChatIcon.h"
#include "Packages/ControlLock/ControlLock.h"
#include "../NpcChatting/Ui_NpcChatting.h"
#include "../../../Core/Game/Game.h"
#include "../../../Core/Game/Decoration/Decoration_DecorationCollection.h"
#include "../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_QuestJournal.h"
#include "../../../Core/Game/Scene/Main/Group/Main_GameSceneGroup.h"
#include "../../../Core/Game/Scene/Sub/Content/ChattingUI/ChattingUIScene.h"
#include "../../../Core/Game/Scene/Sub/Group/Sub_GameSceneGroup.h"
#include "../../../Core/Game/Scene/Sub/Type/SubSceneType.h"
#include "../../../Core/Game/Story/Story_StoryProgress.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    bool NavigationPresenter::IsQuiet()
    {
        if (NanamiEngine::ControlLock::Service::Instance().IsLocked())
            return true;

        const auto& subScenes = GameCore::Game::Instance().SubScenes();
        const auto chatting = subScenes.Catch<GameCore::Scene::Sub::ChattingUIScene>(GameCore::Scene::Sub::SceneType::ChattingUI);
        return chatting && chatting->Context().Npc().IsDisplaying();
    }

    void NavigationPresenter::OnStart()
    {
        guide_.Init();

        GameCore::Story::StoryProgress::Instance().OnChanged().Subscribe(
            [this](NanamiEngine::R4::Unit)
        {
            isDirty_ = true;
        }).AddTo(this);
        GameCore::PlayerAvatar::Quest::QuestJournal::Instance().OnChanged().Subscribe(
            [this](NanamiEngine::R4::Unit)
        {
            isDirty_ = true;
        }).AddTo(this);
    }

    void NavigationPresenter::OnUpdate()
    {
        const auto context = GameCore::Game::Instance().Scenes().CurrentContext();
        if (context != lastContext_.lock())
        {
            lastContext_ = context;
            isDirty_     = true;

            if (context)
            {
                const GameCore::Navigation::INavigationSceneSource& source = *context;
                objectivesSubscription_.Set(source.OnNavigationObjectivesChanged().Subscribe(
                    [this](NanamiEngine::R4::Unit)
                {
                    isDirty_ = true;
                }));
            }
            else
                objectivesSubscription_.Dispose();
        }

        if (!isDirty_)
            return;

        isDirty_ = false;
        Evaluate(context);
    }

    void NavigationPresenter::Evaluate(const std::shared_ptr<GameCore::Scene::SceneContextBase>& context)
    {
        auto& memory = NavigationMemory::Instance();

        // NOTE: 切り替えの途中は目的地を引けないので、何も示さない。入場を終えたら決め直す
        const auto guide = guide_.get();
        if (!context || !guide)
        {
            memory.SetCurrent(std::nullopt);
            PointSurpriseAt(nullptr);
            return;
        }

        const GameCore::Condition::ConditionContext conditionContext{
            GameCore::Story::StoryProgress::Instance(),
            &GameCore::PlayerAvatar::Quest::QuestJournal::Instance(),
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()),
            GameCore::Decoration::DecorationCollection::Instance(),
            context.get() };

        const auto* step = guide->FindCurrent(conditionContext);
        if (!step)
        {
            memory.SetCurrent(std::nullopt);
            PointSurpriseAt(nullptr);
            return;
        }

        NavigationObjective objective;
        objective.stepId = step->id_;
        objective.title  = step->title_;

        const GameCore::Navigation::INavigationSceneSource& source = *context;
        for (const auto& option : step->targets_)
        {
            const auto target = source.FindNavigationTarget(option.targetId_);
            if (!target)
                continue;

            objective.title        = option.title_;
            objective.label        = option.label_;
            objective.targetId     = option.targetId_;
            objective.target       = target->object;
            objective.markerHeight = target->markerHeight;
            break;
        }

        PointSurpriseAt(objective.target.lock());
        memory.SetCurrent(std::move(objective));
    }

    void NavigationPresenter::PointSurpriseAt(const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& target)
    {
        std::shared_ptr<BillBoardNpcChatIcon> icon;
        if (target)
        {
            icon = target->Components().Catch<BillBoardNpcChatIcon>().lock();
            for (const auto& child : target->Transform().GetAllChildren())
            {
                if (icon)
                    break;
                
                icon = child->Components().Catch<BillBoardNpcChatIcon>().lock();
            }
        }

        const auto previous = surpriseIcon_.lock();
        if (previous == icon)
            return;

        if (previous)
            previous->SetObjectiveSurprise(false);
        
        if (icon)
            icon->SetObjectiveSurprise(true);
        
        surpriseIcon_ = icon;
    }

    void NavigationPresenter::OnDestroy()
    {
        objectivesSubscription_.Dispose();
    }

    void NavigationPresenter::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("guide_", guide_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::NavigationPresenter);
#pragma endregion
