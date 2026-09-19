#include "Mana.h"

#include <cassert>

#include "../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::StatusParameter
{
    Mana::Mana(const float value)
        : value_(value)
    {
    }

    void Mana::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("value_", value_);
    }

    float Mana::operator/(const Mana& rhs) const
    {
        assert(rhs.value_ != 0.0f && "Mana division by zero");
        return value_ / rhs.value_;
    }
}
