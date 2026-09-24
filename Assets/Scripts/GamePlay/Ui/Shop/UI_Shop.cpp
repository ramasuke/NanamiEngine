#include "UI_Shop.h"

#include "../Format/Ui_MoneyFormat.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    void ShopUi::BuildRows(const size_t count)
    {
        if (rows_.IsBuilt())
            return;

        rows_.Build(rowPrefab_, rowsRoot_, count, rowSpacing_px_);
        if (const auto restock = restockText_.get())
            restockOffsetY_ = restock->Transform().GetLocalPos().y;
    }

    void ShopUi::SubscribeOnClickRow(const std::function<void(size_t)>& onClick) const
    {
        rows_.ForEach([&onClick](ShopRow& row, const size_t i)
        {
            row.SubscribeOnClick([onClick, i]
            {
                onClick(i);
            });
        });
    }

    void ShopUi::SetTitle(const std::string& title) const
    {
        if (const auto text = titleText_.get())
            text->SetText(title);
    }

    void ShopUi::SetMoney(const int balance) const
    {
        if (const auto text = moneyText_.get())
            text->SetText(FormatMoney(balance));
    }

    void ShopUi::Bind(const ShopModel& model) const
    {
        const size_t shownRows = rows_.Bind(model.Entries(), model.Cursor(), moreAboveMark_, moreBelowMark_,
            [&model](ShopRow& row, const auto& entry)
            {
                row.Bind(ShopRowContent{
                    .item         = entry.item,
                    .price        = entry.price,
                    .owned        = model.Owned(entry),
                    .isAffordable = model.Refusal(entry) != ShopRefusal::NotEnoughMoney,
                });
            });

        // 品が窓より少ないときだけ、最後の品の下に「入荷待ち」を書き足す
        if (const auto restock = restockText_.get())
        {
            const bool hasRoom = shownRows < static_cast<size_t>(maxVisibleRows_);
            restock->SetEnable(hasRoom);
            if (hasRoom)
            {
                const glm::vec3 pos = restock->Transform().GetLocalPos();
                restock->Transform().SetLocalPos(glm::vec3(pos.x, restockOffsetY_ + static_cast<float>(shownRows) * rowSpacing_px_, pos.z));
            }
        }

        if (const auto receipt = receipt_.get())
        {
            const auto selected = model.Selected();
            receipt->Show(selected
                ? ShopReceiptContent{
                    .item        = selected->item,
                    .price       = selected->price,
                    .quantity    = model.Quantity(),
                    .maxQuantity = model.MaxQuantity(*selected),
                    .owned       = model.Owned(*selected),
                    .balance     = model.Balance(),
                    .refusal     = model.Refusal(*selected),
                }
                : ShopReceiptContent{});
        }
    }

    void ShopUi::PlayPaidStamp() const
    {
        if (const auto receipt = receipt_.get())
            receipt->PlayPaidStamp();
    }

    void ShopUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("rowsRoot_", rowsRoot_);
        ImGuiHelper::OnDrawInputField("rowSpacing_px_", rowSpacing_px_);
        ImGuiHelper::OnDrawInputField("maxVisibleRows_", maxVisibleRows_);
        ImGuiHelper::OnDrawInputField("moreAboveMark_", moreAboveMark_);
        ImGuiHelper::OnDrawInputField("moreBelowMark_", moreBelowMark_);
        ImGuiHelper::OnDrawInputField("titleText_", titleText_);
        ImGuiHelper::OnDrawInputField("restockText_", restockText_);
        ImGuiHelper::OnDrawInputField("moneyText_", moneyText_);
        ImGuiHelper::OnDrawInputField("receipt_", receipt_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ShopUi);
#pragma endregion
