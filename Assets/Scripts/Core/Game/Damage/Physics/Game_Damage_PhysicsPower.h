#pragma once
#include "cereal/cereal.hpp"

namespace GameCore::Damage
{
    struct PhysicsPower final 
    {
        explicit PhysicsPower(int physicsPower = 0);
        [[nodiscard]] int Value() const { return value_; }
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
            if (version >= 0) archive(CEREAL_NVP(value_));
        }
#pragma endregion
    };
}
CEREAL_CLASS_VERSION(GameCore::Damage::PhysicsPower, 0)