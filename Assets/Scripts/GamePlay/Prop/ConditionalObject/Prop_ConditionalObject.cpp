#include "Prop_ConditionalObject.h"

#include "../../../Core/Game/Condition/Condition_Clock.h"
#include "../../../Core/Game/Decoration/Decoration_DecorationCollection.h"
#include "../../../Core/Game/Condition/Condition_ConditionList.h"
#include "../../../Core/Game/PlayerAvatar/Quest/PlayerAvatar_QuestJournal.h"
#include "../../../Core/Game/Story/Story_StoryProgress.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Prop
{
    void ConditionalObject::OnStart()
    {
        Apply();
        GameCore::Story::StoryProgress::Instance().OnChanged().Subscribe(
            [this](NanamiEngine::R4::Unit)
        {
            Apply();
        }).AddTo(this);
        GameCore::Decoration::DecorationCollection::Instance().OnChanged().Subscribe(
            [this](NanamiEngine::R4::Unit)
        {
            Apply();
        }).AddTo(this);
#if NANAMI_DEBUG_SHEET_ENABLED
        GameCore::Condition::Clock::OnDebugNowChanged().Subscribe(
            [this](NanamiEngine::R4::Unit)
        {
            Apply();
        }).AddTo(this);
#endif
    }

    void ConditionalObject::Apply()
    {
        const GameCore::Condition::ConditionContext context{
            GameCore::Story::StoryProgress::Instance(),
            &GameCore::PlayerAvatar::Quest::QuestJournal::Instance(),
            GameCore::Condition::Clock::Now(),
            GameCore::Decoration::DecorationCollection::Instance() };
        const bool isShown = GameCore::Condition::ConditionList::AreAllSatisfied(conditions_, context);

        if (target_)
            target_->SetEnable(isShown);

        const auto spawned = spawned_.lock();
        if (isShown && !spawned && prefab_)
        {
            if (const auto shown = Scene::GameObject::Instantiate(*prefab_.get(), Entity().lock()).lock())
            {
                shown->Transform().SetLocalPos(glm::vec3(0.0f));
                shown->Transform().SetLocalRot(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
                spawned_ = shown;
            }
        }
        else if (!isShown && spawned)
        {
            spawned->OnDestroy();
            spawned_.reset();
        }
    }

    void ConditionalObject::OnDrawGui()
    {
        GameCore::Condition::ConditionList::DrawListGui("conditions_", conditions_);
        ImGuiHelper::OnDrawInputField("target_", target_);
        ImGuiHelper::OnDrawInputField("prefab_", prefab_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Prop::ConditionalObject);
#pragma endregion
