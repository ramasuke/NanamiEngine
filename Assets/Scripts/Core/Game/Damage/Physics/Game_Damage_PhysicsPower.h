#pragma once
#include "cereal/cereal.hpp"
#include "../Flinch/Game_Damage_FlinchPower.h"

namespace GameCore::Damage
{
    struct PhysicsPower final 
    {
        explicit PhysicsPower(int physicsPower = 0, Damage::FlinchPower flinchPower = Damage::FlinchPower());
        [[nodiscard]] int Value() const { return value_; }
        // NOTE: 敵の怯み耐性を超えると怯ませる(攻撃もキャンセルされる)
        [[nodiscard]] Damage::FlinchPower FlinchPower() const { return flinchPower_; }
        // NOTE: 怯み値はそのままに威力だけを差し替える
        [[nodiscard]] PhysicsPower WithValue(const int value) const { return PhysicsPower(value, flinchPower_); }
        void OnDrawGui();
        
        
    private:
        [[serialize(0)]] int value_;
        [[serialize(1)]] Damage::FlinchPower flinchPower_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(CEREAL_NVP(value_));
            archive(CEREAL_NVP(flinchPower_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            if (version >= 0) archive(CEREAL_NVP(value_));
            if (version >= 1) archive(CEREAL_NVP(flinchPower_));
        }
#pragma endregion
    };
}
CEREAL_CLASS_VERSION(GameCore::Damage::PhysicsPower, 1)