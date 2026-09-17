#pragma once
#include <cstdint>

#include "../cereal/include/cereal/cereal.hpp"

namespace GameCore::StatusParameter
{
    //NOTE: 値オブジェクト
    struct Money final
    {
        explicit Money(int value = 0);

        [[nodiscard]] int Value() const { return value_; }

        Money  operator+ (const Money& other) const { return Money(value_ + other.value_); }
        Money  operator- (const Money& other) const { return Money(value_ - other.value_); }
        Money& operator+=(const Money& other) { value_ += other.value_; return *this; }
        Money& operator-=(const Money& other) { value_ -= other.value_; return *this; }
        auto operator<=>(const Money&) const = default;

        void OnDrawGui();

    private:
        [[serialize(0)]] int value_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(CEREAL_NVP(value_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(CEREAL_NVP(value_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::StatusParameter::Money, 0)
#pragma endregion
