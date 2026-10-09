#pragma once
#include <compare>
#include <string>

namespace GameCore::Damage
{
    // NOTE: 攻撃の怯み値
    struct FlinchPower final
    {
        constexpr explicit FlinchPower(const int value = 0) : value_(value) {}
        [[nodiscard]] constexpr int Value() const { return value_; }
        [[nodiscard]] constexpr auto operator<=>(const FlinchPower&) const = default;
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
