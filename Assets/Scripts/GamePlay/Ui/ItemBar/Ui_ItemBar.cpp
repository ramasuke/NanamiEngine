#include "Ui_ItemBar.h"

#include <algorithm>
#include <string>

#include "Ui_ItemBarSource.h"
#include "Ui_ItemSlot.h"
#include "Engine/Core/Application/Time/Time.h"
#include "Engine/Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"

namespace GamePlay::Ui
{
    namespace
    {
        using GameCore::PlayerAvatar::PlayerAvatarInputDevice;
        using GameCore::PlayerAvatar::PlayerAvatarControlAcceptance;

        float ItemBarMoveTowards(const float current, const float target, const float maxDelta)
        {
            if (current < target)
                return std::min(current + maxDelta, target);
            return std::max(current - maxDelta, target);
        }

        float ItemBarStepRate(const float deltaTime, const float duration_secs)
        {
            return duration_secs > 0.0f ? deltaTime / duration_secs : 1.0f;
        }

        int ItemBarToBlendRate(const float alpha)
        {
            return std::clamp(static_cast<int>(alpha), 0, 255);
        }
    }

    void ItemBar::Initialize(const std::shared_ptr<IItemBarSource>& source)
    {
        source_ = source;

        if (const auto* pouch = source_ ? source_->Pouch() : nullptr)
            SpawnSlots(*pouch);
    }

    void ItemBar::SpawnSlots(const GameCore::PlayerAvatar::ItemPouch& pouch)
    {
        if (!slotPrefab_ || !slots_)
            return;

        const std::size_t limit = static_cast<std::size_t>(std::max(maxVisibleSlots_, 0));
        const std::size_t wantedCount = std::min(limit, pouch.Slots().size());
        if (wantedCount <= visibleCount_)
            return;

        const auto slotsObject = slots_.get();
        while (slotViews_.size() < wantedCount)
        {
            const auto slotObject = Scene::GameObject::Instantiate(*slotPrefab_.get(), slotsObject).lock();
            slotViews_.push_back(slotObject ? slotObject->Components().Catch<ItemSlot>() : std::weak_ptr<ItemSlot>{});
        }
        visibleCount_ = wantedCount;

        // HorizontalLayoutGroup は原点から右へ並べるので、枠の数だけ帯を左へずらして右端を固定する。
        // ルートの位置が「一番右の枠の中心」になる
        const float stripOffsetX = -static_cast<float>(visibleCount_ - 1) * slotPitch_px_;
        const glm::vec3 slotsPos = slotsObject->Transform().GetLocalPos();
        slotsObject->Transform().SetLocalPos(glm::vec3(stripOffsetX, slotsPos.y, slotsPos.z));

        // 名前は選択中の枠(帯の中央)の真上に出す
        const float centreX = (static_cast<float>(CenterSlotIndex()) - static_cast<float>(visibleCount_ - 1)) * slotPitch_px_;
        if (namePlate_)
        {
            const glm::vec3 platePos = namePlate_->Transform().GetLocalPos();
            namePlate_->Transform().SetLocalPos(glm::vec3(centreX, platePos.y, platePos.z));
        }
        if (nameText_)
        {
            const glm::vec3 textPos = nameText_->Transform().GetLocalPos();
            nameText_->Transform().SetLocalPos(glm::vec3(centreX, textPos.y, textPos.z));
        }
    }

    std::size_t ItemBar::PouchIndexOf(const GameCore::PlayerAvatar::ItemPouch& pouch, const std::size_t slotIndex) const
    {
        const auto size = static_cast<long long>(pouch.Slots().size());
        if (size <= 0)
            return 0;

        const long long offset = static_cast<long long>(slotIndex) - static_cast<long long>(CenterSlotIndex());
        const long long raw = static_cast<long long>(pouch.SelectedIndex()) + offset;
        return static_cast<std::size_t>((raw % size + size) % size);
    }

    void ItemBar::RefreshContent(const GameCore::PlayerAvatar::ItemPouch& pouch)
    {
        for (std::size_t i = 0; i < visibleCount_ && i < slotViews_.size(); ++i)
        {
            const auto view = slotViews_[i].lock();
            if (!view)
                continue;

            const auto& slot = pouch.Slots()[PouchIndexOf(pouch, i)];
            const auto icon = slot.item ? slot.item->IconSprite() : nullptr;
            view->SetContent(std::weak_ptr<Asset::SpriteFile>(icon), std::to_string(slot.count));
            view->SetSelected(i == CenterSlotIndex());
        }

        if (nameText_)
        {
            const auto* selected = pouch.Selected();
            nameText_->SetText(selected != nullptr && selected->item ? selected->item->DisplayName() : std::string());
        }
    }

    void ItemBar::PresentSlots(const GameCore::PlayerAvatar::ItemPouch& pouch) const
    {
        const float usableRate = isUsableDeclared_ ? 1.0f : unusableAlphaRate_;
        const float pulse = selectPulseDuration_secs_ > 0.0f
            ? std::max(0.0f, 1.0f - selectPulse_secs_ / selectPulseDuration_secs_)
            : 0.0f;
        const float groupAlpha = 255.0f * barAlpha_ * usableRate;

        for (std::size_t i = 0; i < visibleCount_ && i < slotViews_.size(); ++i)
        {
            const auto view = slotViews_[i].lock();
            if (!view)
                continue;

            const bool isSelected = (i == CenterSlotIndex());
            const auto& slot = pouch.Slots()[PouchIndexOf(pouch, i)];
            const float emptyRate = slot.count > 0 ? 1.0f : emptyAlphaRate_;
            const float bodyAlpha = groupAlpha * (isSelected ? 1.0f : static_cast<float>(dimAlpha_) / 255.0f);

            view->Apply(ItemSlot::Appearance{
                .scale              = isSelected ? selectedScale_ : unselectedScale_,
                .bodyAlpha          = ItemBarToBlendRate(bodyAlpha),
                .iconAlpha          = ItemBarToBlendRate(bodyAlpha * emptyRate),
                .frameAlpha         = ItemBarToBlendRate(isSelected ? 0.0f : bodyAlpha),
                .selectedFrameAlpha = ItemBarToBlendRate(isSelected ? bodyAlpha : 0.0f),
                .selectGlowAlpha    = ItemBarToBlendRate(isSelected ? static_cast<float>(selectGlowMaxAlpha_) * pulse * barAlpha_ : 0.0f),
                .countAlpha         = ItemBarToBlendRate(bodyAlpha * emptyRate),
            });
        }

        const int groupBlendRate = ItemBarToBlendRate(groupAlpha);
        if (namePlate_)  namePlate_ ->SetBlendRate(groupBlendRate);
        if (nameText_)   nameText_  ->SetBlendRate(groupBlendRate);
        if (cycleGlyph_) cycleGlyph_->SetBlendRate(groupBlendRate);
        if (cycleLabel_) cycleLabel_->SetBlendRate(groupBlendRate);
        if (useGlyph_)   useGlyph_  ->SetBlendRate(groupBlendRate);
        if (useLabel_)   useLabel_  ->SetBlendRate(groupBlendRate);
    }

    void ItemBar::FadeOutSlots() const
    {
        for (const auto& weakView : slotViews_)
        {
            if (const auto view = weakView.lock())
                view->Apply(ItemSlot::Appearance{});
        }

        if (namePlate_)  namePlate_ ->SetBlendRate(0);
        if (nameText_)   nameText_  ->SetBlendRate(0);
        if (cycleGlyph_) cycleGlyph_->SetBlendRate(0);
        if (cycleLabel_) cycleLabel_->SetBlendRate(0);
        if (useGlyph_)   useGlyph_  ->SetBlendRate(0);
        if (useLabel_)   useLabel_  ->SetBlendRate(0);
    }

    void ItemBar::ApplyDeviceGlyphs() const
    {
        const bool isPad = device_ == PlayerAvatarInputDevice::Gamepad;

        if (cycleGlyph_) cycleGlyph_->SetSprite((isPad ? padCycleSprite_ : keyCycleSprite_).get());
        if (useGlyph_)   useGlyph_  ->SetSprite((isPad ? padUseSprite_   : keyUseSprite_  ).get());
    }

    void ItemBar::OnUpdate()
    {
        const float deltaTime = Time::DeltaTime();
        auto* const pouchPtr = source_ ? source_->Pouch() : nullptr;
        if (!pouchPtr)
        {
            barAlpha_ = ItemBarMoveTowards(barAlpha_, 0.0f, ItemBarStepRate(deltaTime, fadeDuration_secs_));
            FadeOutSlots();
            return;
        }

        const auto device = source_->CurrentDevice();
        if (isDeviceDirty_ || device != device_)
        {
            device_ = device;
            isDeviceDirty_ = false;
            ApplyDeviceGlyphs();
        }

        const auto declaration = source_->Declaration();
        const auto acceptance = declaration.acceptance;
        // Momentary は一瞬で終わるので、直前に宣言された内容をそのまま引き継ぐ(帯が瞬かない)
        if (acceptance == PlayerAvatarControlAcceptance::Accept)
        {
            isShownDeclared_  = declaration.isShown;
            isUsableDeclared_ = declaration.isUsable;
        }

        auto& pouch = *pouchPtr;
        if (isContentDirty_ || pouch.Revision() != lastRevision_)
        {
            if (!isContentDirty_ && pouch.SelectedIndex() != lastSelectedIndex_)
                selectPulse_secs_ = 0.0f;

            lastRevision_ = pouch.Revision();
            lastSelectedIndex_ = pouch.SelectedIndex();
            isContentDirty_ = false;
            // 拾ったアイテムでポーチの枠が増えることがある
            SpawnSlots(pouch);
            RefreshContent(pouch);
        }

        const bool isShown = acceptance != PlayerAvatarControlAcceptance::None && isShownDeclared_ && visibleCount_ > 0;
        barAlpha_ = ItemBarMoveTowards(barAlpha_, isShown ? 1.0f : 0.0f, ItemBarStepRate(deltaTime, fadeDuration_secs_));
        selectPulse_secs_ += deltaTime;

        PresentSlots(pouch);
    }

    void ItemBar::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("slots_", slots_);
        ImGuiHelper::OnDrawInputField("slotPrefab_", slotPrefab_);
        ImGuiHelper::OnDrawInputField("namePlate_", namePlate_);
        ImGuiHelper::OnDrawInputField("nameText_", nameText_);
        ImGuiHelper::OnDrawInputField("cycleGlyph_", cycleGlyph_);
        ImGuiHelper::OnDrawInputField("cycleLabel_", cycleLabel_);
        ImGuiHelper::OnDrawInputField("useGlyph_", useGlyph_);
        ImGuiHelper::OnDrawInputField("useLabel_", useLabel_);
        ImGuiHelper::OnDrawInputField("padCycleSprite_", padCycleSprite_);
        ImGuiHelper::OnDrawInputField("keyCycleSprite_", keyCycleSprite_);
        ImGuiHelper::OnDrawInputField("padUseSprite_", padUseSprite_);
        ImGuiHelper::OnDrawInputField("keyUseSprite_", keyUseSprite_);
        ImGuiHelper::OnDrawInputField("maxVisibleSlots_", maxVisibleSlots_);
        ImGuiHelper::OnDrawInputField("slotPitch_px_", slotPitch_px_);
        ImGuiHelper::OnDrawInputField("selectedScale_", selectedScale_);
        ImGuiHelper::OnDrawInputField("unselectedScale_", unselectedScale_);
        ImGuiHelper::OnDrawInputField("dimAlpha_", dimAlpha_);
        ImGuiHelper::OnDrawInputField("emptyAlphaRate_", emptyAlphaRate_);
        ImGuiHelper::OnDrawInputField("unusableAlphaRate_", unusableAlphaRate_);
        ImGuiHelper::OnDrawInputField("fadeDuration_secs_", fadeDuration_secs_);
        ImGuiHelper::OnDrawInputField("selectPulseDuration_secs_", selectPulseDuration_secs_);
        ImGuiHelper::OnDrawInputField("selectGlowMaxAlpha_", selectGlowMaxAlpha_);
    }
}
