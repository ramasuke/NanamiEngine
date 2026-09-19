#include "Ui_ShopRow.h"

#include <string>

#include "../../Format/Ui_MoneyFormat.h"

namespace GamePlay::Ui
{
    void ShopRow::Bind(const ShopRowContent& content)
    {
        const auto& item = content.item;
        if (const auto icon = iconRenderer_.get())
            icon->SetSprite(std::weak_ptr<Asset::SpriteFile>(item ? item->IconSprite() : nullptr));
        if (const auto name = nameText_.get())
            name->SetText(item ? item->DisplayName() : std::string());
        if (const auto owned = ownedText_.get())
            owned->SetText(item ? "手持ち " + std::to_string(content.owned) + "/" + std::to_string(item->MaxStack()) : std::string());
        if (const auto price = priceText_.get())
        {
            price->SetText(FormatMoney(content.price));
            price->SetTextColor(content.isAffordable ? priceColor_ : unaffordablePriceColor_);
        }
    }

    void ShopRow::SubscribeOnClick(std::function<void()> onClick)
    {
        const auto button = selectButton_.get();
        if (!button)
            return;

        button->OnClick().subscribe(DestroyCancellationToken(), [onClick](NanamiUi::MouseState)
        {
            onClick();
        });
    }

    void ShopRow::SetHighlighted(const bool isHighlighted) const
    {
        if (const auto mark = selectMark_.get())
            mark->SetEnable(isHighlighted);
    }

    void ShopRow::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("iconRenderer_", iconRenderer_);
        ImGuiHelper::OnDrawInputField("nameText_", nameText_);
        ImGuiHelper::OnDrawInputField("ownedText_", ownedText_);
        ImGuiHelper::OnDrawInputField("priceText_", priceText_);
        ImGuiHelper::OnDrawInputField("selectMark_", selectMark_);
        ImGuiHelper::OnDrawInputField("selectButton_", selectButton_);
        ImGuiHelper::OnDrawInputField("priceColor_", priceColor_);
        ImGuiHelper::OnDrawInputField("unaffordablePriceColor_", unaffordablePriceColor_);
    }
}
