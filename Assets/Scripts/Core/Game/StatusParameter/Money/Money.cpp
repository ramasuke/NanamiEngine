#include "Money.h"

#include "../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::StatusParameter
{
    Money::Money(const int value)
        : value_(value)
    {
    }

    void Money::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("value_", value_);
    }
}
