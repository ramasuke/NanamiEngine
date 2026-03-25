#pragma once
#include "../cereal/include/cereal/cereal.hpp"
#include <string>
#include "vec3.hpp"
#include "vec2.hpp"

namespace GameCore::StatusParameter
{
    //NOTE: 値オブジェクト
    struct MoveSpeed final
    {
        explicit MoveSpeed(float value = 0.0f);
        MoveSpeed(const MoveSpeed&) = default;
        MoveSpeed  operator+ (const MoveSpeed& other) const { return MoveSpeed(value_ + other.value_); }
        MoveSpeed  operator- (const MoveSpeed& other) const { return MoveSpeed(value_ - other.value_); }
        MoveSpeed  operator* (const double scale    ) const { return MoveSpeed(value_ * scale       ); }
        glm::vec2  operator* (const glm::vec2& vec  ) const { return vec * value_;                     }
        glm::vec3  operator* (const glm::vec3& vec  ) const { return vec * value_;                     }
        MoveSpeed  operator/ (const double divisor  ) const { return MoveSpeed(value_ / divisor     ); }
        MoveSpeed& operator+=(const MoveSpeed& other) { value_ += other.value_; return *this; }
        MoveSpeed& operator-=(const MoveSpeed& other) { value_ -= other.value_; return *this; }
        bool operator==(const MoveSpeed& other) const { return value_ == other.value_; }
        bool operator!=(const MoveSpeed& other) const { return !(*this == other);      }
        bool operator< (const MoveSpeed& other) const { return value_ <  other.value_; }
        bool operator<=(const MoveSpeed& other) const { return value_ <= other.value_; }
        bool operator> (const MoveSpeed& other) const { return value_ >  other.value_; }
        bool operator>=(const MoveSpeed& other) const { return value_ >= other.value_; }
        [[nodiscard]] float       Value   () const { return value_;                 }
        [[nodiscard]] std::string ToString() const { return std::to_string(value_); }

    private:
        [[serialize(0)]] float value_;
#pragma region Serialization Function
public:
void OnDrawGui();

        template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(CEREAL_NVP(value_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    if (version >= 0) archive(CEREAL_NVP(value_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::StatusParameter::MoveSpeed, 0)
#pragma endregion
