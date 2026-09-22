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
        /** @brief 0..1 に正規化した RGB。補間や倍率の計算用 */
        [[nodiscard]] glm::vec3 ToVec3() const;
        /** @brief 0..1 の RGB から作る。範囲外は clamp する */
        [[nodiscard]] static Color32 FromVec3(const glm::vec3& rgb);

        [[nodiscard]] uint8_t R() const { return r_; }
        [[nodiscard]] uint8_t G() const { return g_; }
        [[nodiscard]] uint8_t B() const { return b_; }

        /** @brief カラーピッカーで編集する。変更があれば true */
        bool DrawColorEdit(const char* label);

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