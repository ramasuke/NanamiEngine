#include "Engine_Physics_MotionType.h"

#include "ImGuiHelper.h"

namespace NanamiEngine::Module::Physics
{
    bool DrawChoiceMotionTypeGui(const char* label, MotionType& motionType)
    {
        static constexpr const char* MOTION_TYPE_NAMES[] = { "Static", "Kinematic", "Dynamic" };

        int index = static_cast<int>(motionType);
        if (!ImGui::Combo(label, &index, MOTION_TYPE_NAMES, IM_ARRAYSIZE(MOTION_TYPE_NAMES)))
            return false;

        motionType = static_cast<MotionType>(index);
        return true;
    }
}
