#include "Ui_PauseMenu.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>
#include <string>

#include "Model/PauseMenuModel.h"
#include "../Format/Ui_MoneyFormat.h"
#include "../../../../../Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../Core/Game/PlayerAvatar/Item/ItemPouch.h"
#include "../../../Core/Game/PlayerAvatar/SwordMan/Status/SwordManAvatarStatus.h"

namespace GamePlay::Ui
{
    namespace
    {
        struct PauseMenuEntryText
        {
            const char* name;
            const char* description;
        };

        PauseMenuEntryText PauseMenuTextOf(const PauseMenuEntry entry)
        {
            switch (entry)
            {
            case PauseMenuEntry::Status:        return { "ステータス",   "体の具合と稼ぎを確かめる" };
            case PauseMenuEntry::Items:         return { "持ち物",       "袋の中身を並べ替える" };
            case PauseMenuEntry::Quests:        return { "クエスト",     "受けた頼みごとを読み返す" };
            case PauseMenuEntry::Controls:      return { "操作方法",     "手綱の握り方を思い出す" };
            case PauseMenuEntry::ReturnToTitle: return { "タイトルへ",   "冒険を切り上げる" };
            case PauseMenuEntry::Resume:        return { "ゲームへ戻る", "そのまま続ける" };
            }
            return { "", "" };
        }

        std::string PauseMenuFormatGauge(const int current, const int max)
        {
            return std::to_string(current) + " / " + std::to_string(max);
        }
    }

    void PauseMenuUi::BuildRows()
    {
        if (!rows_.empty() || !rowPrefab_ || !rowsRoot_)
            return;

        const auto rowsObject = rowsRoot_.get();
        for (std::size_t i = 0; i < PAUSE_MENU_ENTRIES.size(); ++i)
        {
            const auto rowObject = Scene::GameObject::Instantiate(*rowPrefab_.get(), rowsObject).lock();
            if (!rowObject)
                continue;

            const auto row = rowObject->Components().Catch<PauseMenuRow>();
            if (const auto locked = row.lock())
            {
                const auto text = PauseMenuTextOf(PAUSE_MENU_ENTRIES[i]);
                locked->SetContent(text.name, text.description, static_cast<int>(i) + 1);
            }
            rows_.push_back(row);
        }
    }

    void PauseMenuUi::ShowCharacter() const
    {
        const auto character = character_.get();
        if (!character)
            return;

        if (nameText_)    nameText_   ->SetText(character->DisplayName());
        if (readingText_) readingText_->SetText(character->Reading());
        if (taglineText_) taglineText_->SetText(character->Tagline());

        const int pips[] = { character->PowerPips(), character->ToughnessPips(), character->AgilityPips() };
        for (std::size_t i = 0; i < statPips_.size(); ++i)
        {
            if (const auto strip = statPips_[i].get())
                strip->SetDifficulty(i < std::size(pips) ? pips[i] : 0);
        }
    }

    void PauseMenuUi::SetVisible(const bool isVisible) const
    {
        if (book_)
            book_->SetEnable(isVisible);
    }

    void PauseMenuUi::HighlightRow(const std::size_t index) const
    {
        for (std::size_t i = 0; i < rows_.size(); ++i)
        {
            if (const auto row = rows_[i].lock())
                row->SetHighlighted(i == index);
        }
    }

    void PauseMenuUi::Present(const GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus& status)
    {
        const auto health    = status.Health().Value();
        const auto maxHealth = status.MaxHealth().Value();
        if (healthBar_)  healthBar_ ->SetValue(maxHealth > 0 ? static_cast<float>(health) / static_cast<float>(maxHealth) : 0.0f);
        if (healthText_) healthText_->SetText(PauseMenuFormatGauge(health, maxHealth));

        const float stamina    = status.Stamina().Value().Value();
        const float maxStamina = status.MaxStamina().Value();
        if (staminaBar_)  staminaBar_ ->SetValue(maxStamina > 0.0f ? stamina / maxStamina : 0.0f);
        if (staminaText_) staminaText_->SetText(PauseMenuFormatGauge(static_cast<int>(std::lround(stamina)),
                                                                     static_cast<int>(std::lround(maxStamina))));

        if (moneyText_)
            moneyText_->SetText(FormatMoney(status.Wallet().Balance().Value()));

        PresentItems(status.Pouch());
    }

    void PauseMenuUi::PresentItems(const GameCore::PlayerAvatar::ItemPouch& pouch)
    {
        if (hasPresentedItems_ && pouch.Revision() == presentedPouchRevision_)
            return;
        hasPresentedItems_ = true;
        presentedPouchRevision_ = pouch.Revision();

        const auto& slots = pouch.Slots();
        // 拾ったアイテムでポーチの枠が増えることがあるので、足りない分だけ足す
        if (itemCellPrefab_ && itemsRoot_)
        {
            const auto itemsObject = itemsRoot_.get();
            const auto count = std::min(slots.size(), static_cast<std::size_t>(std::max(maxItemCells_, 0)));
            while (itemCells_.size() < count)
            {
                const auto cellObject = Scene::GameObject::Instantiate(*itemCellPrefab_.get(), itemsObject).lock();
                itemCells_.push_back(cellObject ? cellObject->Components().Catch<PauseMenuItemCell>() : std::weak_ptr<PauseMenuItemCell>{});
            }
        }

        for (std::size_t i = 0; i < itemCells_.size() && i < slots.size(); ++i)
        {
            const auto cell = itemCells_[i].lock();
            if (!cell)
                continue;

            const auto& slot = slots[i];
            const auto icon = slot.item ? slot.item->IconSprite() : nullptr;
            cell->SetContent(std::weak_ptr<Asset::SpriteFile>(icon), slot.count);
        }
    }

    void PauseMenuUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("book_", book_);
        ImGuiHelper::OnDrawInputField("rowPrefab_", rowPrefab_);
        ImGuiHelper::OnDrawInputField("rowsRoot_", rowsRoot_);
        ImGuiHelper::OnDrawInputField("character_", character_);
        ImGuiHelper::OnDrawInputField("nameText_", nameText_);
        ImGuiHelper::OnDrawInputField("readingText_", readingText_);
        ImGuiHelper::OnDrawInputField("taglineText_", taglineText_);
        ImGuiHelper::OnDrawInputField("statPips_", statPips_, [this]
        {
            if (ImGui::Button("Add Stat"))
            {
                statPips_.emplace_back();
            }
        });
        ImGuiHelper::OnDrawInputField("healthBar_", healthBar_);
        ImGuiHelper::OnDrawInputField("healthText_", healthText_);
        ImGuiHelper::OnDrawInputField("staminaBar_", staminaBar_);
        ImGuiHelper::OnDrawInputField("staminaText_", staminaText_);
        ImGuiHelper::OnDrawInputField("moneyText_", moneyText_);
        ImGuiHelper::OnDrawInputField("itemCellPrefab_", itemCellPrefab_);
        ImGuiHelper::OnDrawInputField("itemsRoot_", itemsRoot_);
        ImGuiHelper::OnDrawInputField("maxItemCells_", maxItemCells_);
    }
}
