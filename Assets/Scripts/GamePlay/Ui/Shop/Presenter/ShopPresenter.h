#pragma once
#include <memory>

#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "../../../../../../Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../../../Data/Shop/Data_ShopData.h"
#include "../Model/ShopModel.h"
#include "../UI_Shop.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Prop
{
    class MerchantStall;
}

namespace GamePlay::Ui
{
    /**
     * @brief 店の画面の開閉と入力。品揃えはこのプレハブが持つ .shopData から読む。
     * ↑↓で品、←→で個数(押し続けると連続)、A で買ってその場で保存、B で閉じる。
     * 開いている間はアバターを止め、露店があればそのカメラへ寄せる。
     */
    class ShopPresenter final : public Component::ComponentBase,
                                public LifeCycleCallback::IStartable,
                                public LifeCycleCallback::IUpdatable
    {
    public:
        /** @brief OnStart より前に呼ぶ */
        void Bind(const std::weak_ptr<Prop::MerchantStall>& stall);

    private:
        struct Keys
        {
            bool prev    = false;
            bool next    = false;
            bool less    = false;
            bool more    = false;
            bool confirm = false;
            bool cancel  = false;
        };

        void OnStart  () override;
        void OnUpdate () override;
        void OnDestroy() override;

        [[nodiscard]] static Keys ReadKeys();
        void UpdateQuantity(const Keys& keys);
        void ChangeQuantity(int delta);
        void Purchase();
        void Refresh() const;
        void PlaySound(const FIELD(Asset::SoundFile)& sound) const;
        void Close();

        [[serialize(0)]] FIELD(Asset::ShopData) shop_;
        [[serialize(0)]] FIELD(Asset::SoundFile) purchaseSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile) refuseSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile) cursorSound_;
        [[serialize(0)]] float quantityRepeatDelay_secs_    = 0.35f;
        [[serialize(0)]] float quantityRepeatInterval_secs_ = 0.08f;

        std::shared_ptr<ShopUi> view_;
        std::unique_ptr<ShopModel> model_;
        std::weak_ptr<GameCore::IPlayerAvatar> suspendedAvatar_;
        std::weak_ptr<Prop::MerchantStall> stall_;

        Keys previousKeys_;
        int quantityHoldDirection_ = 0;
        float quantityHold_secs_ = 0.0f;
        float quantityRepeat_secs_ = 0.0f;
        bool isClosing_ = false;
        bool isClosed_ = false;
        bool isDuplicate_ = false;

        // 話しかけるたびに二重に生えるのを防ぐ
        static bool isOpen_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(shop_));
            archive(CEREAL_NVP(purchaseSound_));
            archive(CEREAL_NVP(refuseSound_));
            archive(CEREAL_NVP(cursorSound_));
            archive(CEREAL_NVP(quantityRepeatDelay_secs_));
            archive(CEREAL_NVP(quantityRepeatInterval_secs_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(shop_));
            if (version >= 0) archive(CEREAL_NVP(purchaseSound_));
            if (version >= 0) archive(CEREAL_NVP(refuseSound_));
            if (version >= 0) archive(CEREAL_NVP(cursorSound_));
            if (version >= 0) archive(CEREAL_NVP(quantityRepeatDelay_secs_));
            if (version >= 0) archive(CEREAL_NVP(quantityRepeatInterval_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ShopPresenter, 0)
