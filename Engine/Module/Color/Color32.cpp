#include "Color32.h"
#include <cmath>
#include "DxLib.h"

NanamiEngine::Color32::Color32(const uint8_t r, const uint8_t g, const uint8_t b)
    : r_(r), g_(g), b_(b)
{
}

int NanamiEngine::Color32::ToDxColor() const
{
    return GetColor(r_, g_, b_);
}

glm::vec3 NanamiEngine::Color32::ToVec3() const
{
    return {r_ / 255.0f, g_ / 255.0f, b_ / 255.0f};
}

NanamiEngine::Color32 NanamiEngine::Color32::FromVec3(const glm::vec3& rgb)
{
    const auto toByte = [](const float v)
    {
        return static_cast<uint8_t>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };
    return Color32(toByte(rgb.r), toByte(rgb.g), toByte(rgb.b));
}

bool NanamiEngine::Color32::DrawColorEdit(const char* label)
{
    glm::vec3 rgb = ToVec3();
    if (!ImGui::ColorEdit3(label, &rgb.x, ImGuiColorEditFlags_Uint8))
        return false;

    *this = FromVec3(rgb);
    return true;
}
