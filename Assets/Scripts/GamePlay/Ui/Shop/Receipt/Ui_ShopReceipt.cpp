#include "Ui_ShopReceipt.h"

#include <algorithm>
#include <string>

#include "../../Format/Ui_MoneyFormat.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        // 判は押した直後の少しの間だけ大きく、そのあと残って最後に薄れる
        constexpr float SHOP_STAMP_PRESS_RATE = 0.2f;
        constexpr float SHOP_STAMP_FADE_RATE  = 0.7f;

        std::string ShopRefusalText(const ShopRefusal refusal)
        {
            switch (refusal)
            {
            case ShopRefusal::PouchFull:      return "これ以上持てない";
            case ShopRefusal::NotEnoughMoney: return "お金が足りない";
            case ShopRefusal::None:           return "";
            }
            return "";
        }
    }

    void ShopReceipt::Show(const ShopReceiptContent& content) const
    {
        const auto& item = content.item;
        if (const auto root = contentRoot_.get())
            root->SetEnable(item != nullptr);
        if (!item)
            return;

        iconRenderer_->SetSprite(std::weak_ptr<Asset::SpriteFile>(item->IconSprite()));
        nameText_->SetText(item->DisplayName());
        ownedText_->SetText("手持ち " + std::to_string(content.owned) + " / " + std::to_string(item->MaxStack()));

        const auto& lines = item->DescriptionLines();
        for (size_t i = 0; i < descriptionLines_.size(); ++i)
        {
            const auto text = descriptionLines_[i].get();
            if (!text)
                continue;

            const bool hasLine = i < lines.size();
            text->SetEnable(hasLine);
            if (hasLine)
                text->SetText(lines[i]);
        }

        unitPriceText_->SetText(FormatMoney(content.price));
        quantityText_->SetText(std::to_string(content.quantity));
        decreaseMark_->SetBlendRate(content.quantity > 1 ? markActiveBlendRate_ : markInactiveBlendRate_);
        increaseMark_->SetBlendRate(content.quantity < content.maxQuantity ? markActiveBlendRate_ : markInactiveBlendRate_);

        const int total = content.price * content.quantity;
        totalText_->SetText(FormatMoney(total));

        const bool isRefused = content.refusal != ShopRefusal::None;
        afterPaymentText_->SetEnable(!isRefused);
        refusalText_->SetEnable(isRefused);
        if (isRefused)
            refusalText_->SetText(ShopRefusalText(content.refusal));
        else
            afterPaymentText_->SetText("支払い後 " + FormatMoney(content.balance - total));
    }

    void ShopReceipt::PlayPaidStamp()
    {
        const auto stamp = paidStamp_.get();
        if (!stamp)
            return;

        if (!stampAlpha_.IsPlaying())
            stampBaseScale_ = stamp->Transform().GetLocalScale();
        stamp->SetEnable(true);

        // NOTE: 区間ごとに ms へ丸めると2本の合計がずれるので、境目の時刻から引き算で区間長を出す
        const float    duration   = std::max(stampDuration_secs_, 0.01f);
        const uint16_t totalMs    = LibCore::Tween::Ms(duration);
        const uint16_t pressEndMs = LibCore::Tween::Ms(duration * SHOP_STAMP_PRESS_RATE);
        const uint16_t fadeFromMs = LibCore::Tween::Ms(duration * SHOP_STAMP_FADE_RATE);
        stampScale_.Play(tweeny::from(stampStartScale_)
            .to(1.0f).during(pressEndMs)
            .to(1.0f).during(static_cast<uint16_t>(totalMs - pressEndMs)));
        stampAlpha_.Play(tweeny::from(0.0f)
            .to(1.0f).during(pressEndMs)
            .to(1.0f).during(static_cast<uint16_t>(fadeFromMs - pressEndMs))
            .to(0.0f).during(static_cast<uint16_t>(totalMs - fadeFromMs)));
    }

    void ShopReceipt::OnUpdate()
    {
        const auto stamp = paidStamp_.get();
        if (!stamp || !stampAlpha_.IsPlaying())
            return;

        const float deltaTime = Time::DeltaTime();
        stampScale_.Tick(deltaTime);
        const bool finished = stampAlpha_.Tick(deltaTime);
        stamp->Transform().SetLocalScale(stampBaseScale_ * stampScale_.Value());
        stamp->SetBlendRate(static_cast<int>(255.0f * stampAlpha_.Value()));

        if (finished)
        {
            stamp->Transform().SetLocalScale(stampBaseScale_);
            stamp->SetEnable(false);
        }
    }

    void ShopReceipt::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("contentRoot_", contentRoot_);
        ImGuiHelper::OnDrawInputField("iconRenderer_", iconRenderer_);
        ImGuiHelper::OnDrawInputField("nameText_", nameText_);
        ImGuiHelper::OnDrawInputField("ownedText_", ownedText_);
        ImGuiHelper::OnDrawInputField("descriptionLines_", descriptionLines_, [this]
        {
            if (ImGui::Button("Add Line"))
            {
                descriptionLines_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("unitPriceText_", unitPriceText_);
        ImGuiHelper::OnDrawInputField("quantityText_", quantityText_);
        ImGuiHelper::OnDrawInputField("decreaseMark_", decreaseMark_);
        ImGuiHelper::OnDrawInputField("increaseMark_", increaseMark_);
        ImGuiHelper::OnDrawInputField("totalText_", totalText_);
        ImGuiHelper::OnDrawInputField("afterPaymentText_", afterPaymentText_);
        ImGuiHelper::OnDrawInputField("refusalText_", refusalText_);
        ImGuiHelper::OnDrawInputField("paidStamp_", paidStamp_);
        ImGuiHelper::OnDrawInputField("markActiveBlendRate_", markActiveBlendRate_);
        ImGuiHelper::OnDrawInputField("markInactiveBlendRate_", markInactiveBlendRate_);
        ImGuiHelper::OnDrawInputField("stampDuration_secs_", stampDuration_secs_);
        ImGuiHelper::OnDrawInputField("stampStartScale_", stampStartScale_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ShopReceipt);
#pragma endregion
