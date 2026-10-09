#pragma once
#include <cstdint>

#include "../../StatusParameter/Money/Money.h"
#include "Packages/R4/R4.h"

namespace GameCore::PlayerAvatar
{
    // NOTE: プレイヤーの所持金。セーブに乗せる
    class Wallet final
    {
    public:
        Wallet();
        explicit Wallet(StatusParameter::Money balance);

        [[nodiscard]] StatusParameter::Money Balance   () const { return balance_.Value(); }
        [[nodiscard]] bool                   CanAfford (const StatusParameter::Money price) const { return balance_.Value() >= price; }
        // NOTE: 購読した時点で現在値が流れる
        [[nodiscard]] NanamiEngine::R4::ReadOnlyReactiveProperty<StatusParameter::Money> Observe() const { return balance_.AsReadOnly(); }

        // NOTE: 0以下は無視する
        void Earn(StatusParameter::Money amount);
        // NOTE: 足りなければ何もせず false
        bool TrySpend(StatusParameter::Money price);

    private:
        [[serialize(0)]] NanamiEngine::R4::SerializableReactiveProperty<StatusParameter::Money> balance_;

#pragma region Serialization Function
    public:
        void OnDrawGui();

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(CEREAL_NVP(balance_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(CEREAL_NVP(balance_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::PlayerAvatar::Wallet, 0)
#pragma endregion
