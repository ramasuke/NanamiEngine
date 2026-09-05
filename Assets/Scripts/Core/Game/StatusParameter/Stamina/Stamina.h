#pragma once
#include <cstdint>

#include "../cereal/include/cereal/cereal.hpp"

namespace GameCore::StatusParameter
{
    //NOTE: 値オブジェクト
    struct Stamina final
    {
        explicit Stamina(float value = 0.0f);

        [[nodiscard]] float Value() const { return value_; }

        Stamina  operator+ (const Stamina& other) const { return Stamina(value_ + other.value_); }
        Stamina  operator- (const Stamina& other) const { return Stamina(value_ - other.value_); }
        Stamina& operator+=(const Stamina& other) { value_ += other.value_; return *this; }
        Stamina& operator-=(const Stamina& other) { value_ -= other.value_; return *this; }
        auto operator<=>(const Stamina&) const = default;

        float operator/(const Stamina& rhs) const;

        void OnDrawGui();

    private:
        [[serialize(0)]] float value_;

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
CEREAL_CLASS_VERSION(GameCore::StatusParameter::Stamina, 0)
#pragma endregion
