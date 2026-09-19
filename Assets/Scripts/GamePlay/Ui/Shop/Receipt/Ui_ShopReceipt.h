#pragma once
#include <memory>
#include <vector>

#include "cereal/types/vector.hpp"
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../../Data/Item/Data_ItemData.h"
#include "../Model/ShopModel.h"

namespace GamePlay::Ui
{
    /** @brief 勘定書きに書く中身 */
    struct ShopReceiptContent final
    {
        std::shared_ptr<Asset::ItemData> item;
        int price       = 0;
        int quantity    = 1;
        int maxQuantity = 0;
        int owned       = 0;
        int balance     = 0;
        ShopRefusal refusal = ShopRefusal::None;
    };

    /**
     * @brief 黒板の右に紐で吊るした勘定書き。選んだ品の説明・単価・個数・合計を書き、買えないときは朱で理由を出す。
     * 「お勘定」「単価」などの見出しと罫線は紙の絵に焼いてあり、ここでは書き込む値だけを差し替える。
     */
    class ShopReceipt final : public Component::ComponentBase,
                              public LifeCycleCallback::IUpdatable
    {
    public:
        /** @param content item が nullptr なら中身を全部隠す(品が1つも無い店) */
        void Show(const ShopReceiptContent& content) const;
        /** @brief 買えたときに朱の「毎度」判を押す */
        void PlayPaidStamp();

    private:
        void OnUpdate() override;

        [[serialize(0)]] FIELD(GameObject::IGameObject) contentRoot_;
        [[serialize(0)]] FIELD(Component::ImageRenderer) iconRenderer_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) ownedText_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::TextRenderer)> descriptionLines_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) unitPriceText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) quantityText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) decreaseMark_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) increaseMark_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) totalText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) afterPaymentText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) refusalText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) paidStamp_;
        [[serialize(0)]] int markActiveBlendRate_   = 255;
        [[serialize(0)]] int markInactiveBlendRate_ = 70;
        [[serialize(0)]] float stampDuration_secs_ = 0.6f;
        [[serialize(0)]] float stampStartScale_ = 1.6f;

        float stampElapsed_secs_ = -1.0f;
        glm::vec3 stampBaseScale_ = glm::vec3(1.0f);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(contentRoot_));
            archive(CEREAL_NVP(iconRenderer_));
            archive(CEREAL_NVP(nameText_));
            archive(CEREAL_NVP(ownedText_));
            archive(CEREAL_NVP(descriptionLines_));
            archive(CEREAL_NVP(unitPriceText_));
            archive(CEREAL_NVP(quantityText_));
            archive(CEREAL_NVP(decreaseMark_));
            archive(CEREAL_NVP(increaseMark_));
            archive(CEREAL_NVP(totalText_));
            archive(CEREAL_NVP(afterPaymentText_));
            archive(CEREAL_NVP(refusalText_));
            archive(CEREAL_NVP(paidStamp_));
            archive(CEREAL_NVP(markActiveBlendRate_));
            archive(CEREAL_NVP(markInactiveBlendRate_));
            archive(CEREAL_NVP(stampDuration_secs_));
            archive(CEREAL_NVP(stampStartScale_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(contentRoot_));
            if (version >= 0) archive(CEREAL_NVP(iconRenderer_));
            if (version >= 0) archive(CEREAL_NVP(nameText_));
            if (version >= 0) archive(CEREAL_NVP(ownedText_));
            if (version >= 0) archive(CEREAL_NVP(descriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(unitPriceText_));
            if (version >= 0) archive(CEREAL_NVP(quantityText_));
            if (version >= 0) archive(CEREAL_NVP(decreaseMark_));
            if (version >= 0) archive(CEREAL_NVP(increaseMark_));
            if (version >= 0) archive(CEREAL_NVP(totalText_));
            if (version >= 0) archive(CEREAL_NVP(afterPaymentText_));
            if (version >= 0) archive(CEREAL_NVP(refusalText_));
            if (version >= 0) archive(CEREAL_NVP(paidStamp_));
            if (version >= 0) archive(CEREAL_NVP(markActiveBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(markInactiveBlendRate_));
            if (version >= 0) archive(CEREAL_NVP(stampDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(stampStartScale_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ShopReceipt, 0)
