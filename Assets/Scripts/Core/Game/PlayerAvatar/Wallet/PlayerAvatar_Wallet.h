#pragma once
#include <cstdint>

#include "../../StatusParameter/Money/Money.h"
#include "../../../../../../Libs/LibCore/Rx/SerializableSubject/SerializableSubject.h"

namespace GameCore::PlayerAvatar
{
    /// プレイヤーの所持金。ポーチと違いセーブに乗せるので Status が持つ
    class Wallet final
    {
    public:
        Wallet();
        explicit Wallet(StatusParameter::Money balance);

        [[nodiscard]] StatusParameter::Money Balance   () const { return balance_.get(); }
        [[nodiscard]] bool                   CanAfford (const StatusParameter::Money price) const { return balance_.get() >= price; }
        /** @brief 購読した時点で現在値が流れる */
        [[nodiscard]] LibCore::Rx::ReadOnlyReactiveContext<StatusParameter::Money> Observe() const { return balance_.AsReadOnly(); }

        /** @brief 0以下は無視する */
        void Earn(StatusParameter::Money amount);
        /** @brief 足りなければ何もせず false */
        bool TrySpend(StatusParameter::Money price);

    private:
        [[serialize(0)]] LibCore::Rx::SerializableSubject<StatusParameter::Money> balance_;

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
