#pragma once
#include <assert.h>
#include <cstdint>

#include "cereal/cereal.hpp"

namespace GameCore::PlayerAvatar
{
    struct EnhancePower final
    {
        explicit EnhancePower(const int value = 0) : value_(value) {}

        [[nodiscard]] int Value() const { return value_; }

        EnhancePower& operator+=(const EnhancePower& rhs) { value_ += rhs.value_; return *this; }
        EnhancePower& operator-=(const EnhancePower& rhs) { value_ -= rhs.value_; return *this; }
        auto operator<=>(const EnhancePower&) const = default;
        
        friend float operator/(const EnhancePower& lhs, const EnhancePower& rhs)
        {
            assert(rhs.value_ != 0 && "EnahancePower division by zero");
            return static_cast<float>(lhs.value_) / static_cast<float>(rhs.value_);
        }

    private:
        [[serialize(0)]] int value_;
        
#pragma region Serialization Function
    public:
        void OnDrawGui();
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(value_);
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(value_);
        }
#pragma endregion
    };

    inline EnhancePower operator+(EnhancePower lhs, const EnhancePower& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    inline EnhancePower operator-(EnhancePower lhs, const EnhancePower& rhs)
    {
        lhs -= rhs;
        return lhs;
    }
    inline EnhancePower operator*(const EnhancePower& lhs, const int rhs)
    {
        return EnhancePower(lhs.Value() * rhs);
    }

    inline EnhancePower operator*(const int lhs, const EnhancePower& rhs)
    {
        return EnhancePower(lhs * rhs.Value());
    }
}

CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::EnhancePower, 0)