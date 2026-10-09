#pragma once
#include <string>

#include "Game_Damage_FlinchPower.h"

namespace GameCore::Damage
{
    // NOTE: 敵の怯み耐性。負にすると怯み値 0 の攻撃でも怯む(雑魚敵用)
    struct FlinchResistance final
    {
        constexpr explicit FlinchResistance(const int value = 0) : value_(value) {}
        [[nodiscard]] constexpr int Value() const { return value_; }
        [[nodiscard]] constexpr bool IsFlinchedBy(const FlinchPower power) const { return power.Value() > value_; }
        void OnDrawInputField(const std::string& label);

    private:
        int value_;

#pragma region Serialization Function
    public:
        // NOTE: 素の int として保存する(元が int だった既存データをそのまま読める)
        template<class Archive>
        int save_minimal(const Archive&) const { return value_; }

        template<class Archive>
        void load_minimal(const Archive&, const int& value) { value_ = value; }
#pragma endregion
    };
}
