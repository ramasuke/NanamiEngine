#include "Condition_DecorationOwnedCondition.h"

#include "Condition_ConditionFactory.h"
#include "../Decoration/Decoration_DecorationCollection.h"
#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GameCore::Condition
{
    bool DecorationOwnedCondition::IsSatisfied(const ConditionContext& context) const
    {
        const auto decoration = decoration_.get();
        return decoration && context.decorations.IsOwned(decoration->GetGuid());
    }

    std::string DecorationOwnedCondition::Describe() const
    {
        const auto decoration = decoration_.get();
        return "DecorationOwned " + (decoration ? decoration->Name() : std::string("(none)"));
    }

    void DecorationOwnedCondition::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("decoration_", decoration_);
    }

    REGISTER_CONDITION(DecorationOwnedCondition)
}

NANAMI_REGISTER_TYPE(GameCore::Condition::DecorationOwnedCondition, GameCore::Condition::ICondition);
