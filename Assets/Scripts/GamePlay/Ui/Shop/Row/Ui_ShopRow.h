#pragma once
#include <functional>
#include <memory>

#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Color/Color32.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "../../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../../Data/Item/Data_ItemData.h"

namespace GamePlay::Ui
{
    /** @brief 黒板の1行に書く中身 */
    struct ShopRowContent final
    {
        std::shared_ptr<Asset::ItemData> item;
        int  price       = 0;
        int  owned       = 0;
        bool isAffordable = true;
    };

    /**
     * @brief 品書きの黒板の1行。紙片に貼ったアイコン・品名・手持ち・値段をチョークで書き、選ばれた行だけ丸で囲む。
     * 行は表示窓の分だけ作って使い回すので、中身は Bind のたびに差し替える。
     */
    class ShopRow final : public Component::ComponentBase
    {
    public:
        void Bind(const ShopRowContent& content);
        void SubscribeOnClick(std::function<void()> onClick);
        void SetHighlighted(bool isHighlighted) const;

    private:
        [[serialize(0)]] FIELD(Component::ImageRenderer) iconRenderer_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) ownedText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) priceText_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) selectMark_;
        [[serialize(0)]] FIELD(NanamiUi::Button) selectButton_;
        [[serialize(0)]] Color32 priceColor_ = Color32(250, 226, 150);
        [[serialize(0)]] Color32 unaffordablePriceColor_ = Color32(214, 120, 104);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(iconRenderer_));
            archive(CEREAL_NVP(nameText_));
            archive(CEREAL_NVP(ownedText_));
            archive(CEREAL_NVP(priceText_));
            archive(CEREAL_NVP(selectMark_));
            archive(CEREAL_NVP(selectButton_));
            archive(CEREAL_NVP(priceColor_));
            archive(CEREAL_NVP(unaffordablePriceColor_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(iconRenderer_));
            if (version >= 0) archive(CEREAL_NVP(nameText_));
            if (version >= 0) archive(CEREAL_NVP(ownedText_));
            if (version >= 0) archive(CEREAL_NVP(priceText_));
            if (version >= 0) archive(CEREAL_NVP(selectMark_));
            if (version >= 0) archive(CEREAL_NVP(selectButton_));
            if (version >= 0) archive(CEREAL_NVP(priceColor_));
            if (version >= 0) archive(CEREAL_NVP(unaffordablePriceColor_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ShopRow, 0)
