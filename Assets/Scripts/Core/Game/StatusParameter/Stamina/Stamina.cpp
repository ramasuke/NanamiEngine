#include "Stamina.h"

#include <cassert>

#include "../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::StatusParameter
{
    Stamina::Stamina(const float value)
        : value_(value)
    {
    }

    void Stamina::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("value_", value_);
    }

    float Stamina::operator/(const Stamina& rhs) const
    {
        assert(rhs.value_ != 0.0f && "Stamina division by zero");
        return value_ / rhs.value_;
    }
}
