#pragma once
#include <algorithm>

#include "../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine
{
    struct Color32 final
    {
        explicit Color32(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);

        [[nodiscard]] int ToDxColor() const;

    private:
        uint8_t r_;
        uint8_t g_;
        uint8_t b_;

#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            int r = r_, g = g_, b = b_;
            LibCore::ImGuiHelper::OnDrawInputField("r_", r);
            LibCore::ImGuiHelper::OnDrawInputField("g_", g);
            LibCore::ImGuiHelper::OnDrawInputField("b_", b);
            r_ = static_cast<uint8_t>(std::clamp(r, 0, 255));
            g_ = static_cast<uint8_t>(std::clamp(g, 0, 255));
            b_ = static_cast<uint8_t>(std::clamp(b, 0, 255));
        }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(r_));
            archive(CEREAL_NVP(g_));
            archive(CEREAL_NVP(b_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(r_));
            if (version >= 0) archive(CEREAL_NVP(g_));
            if (version >= 0) archive(CEREAL_NVP(b_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Color32, 0);