#include "Health.h"

#include "../../../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace GameCore::StatusParameter
{
    Health::Health(const int value)
        : value_(value)
    {
    }

    void Health::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("value_", value_);
    }

    float Health::operator/(const Health& rhs) const
    {
        assert(rhs.value_ != 0 && "Health division by zero");
        return static_cast<float>(this->value_) / static_cast<float>(rhs.value_);
    }
}
