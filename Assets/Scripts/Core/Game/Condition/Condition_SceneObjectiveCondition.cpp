#include "Condition_SceneObjectiveCondition.h"

#include "Condition_ConditionFactory.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../Navigation/Navigation_INavigationSceneSource.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
    bool SceneObjectiveCondition::IsSatisfied(const ConditionContext& context) const
    {
        return context.scene && context.scene->HasNavigationObjective(objectiveId_);
    }

    std::string SceneObjectiveCondition::Describe() const
    {
        return "SceneObjective " + objectiveId_;
    }

    void SceneObjectiveCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("objectiveId_", objectiveId_);
    }

    REGISTER_CONDITION(SceneObjectiveCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::SceneObjectiveCondition, GameCore::Condition::ICondition);
