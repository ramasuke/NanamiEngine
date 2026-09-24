#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "../../../Core/Game/PlayerAvatar/InputAction/PlayerAvatarInputDevice.h"
#include "../../../Core/Game/PlayerAvatar/Item/ItemPouch.h"
#include "../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    class IItemBarSource;
    class ItemSlot;

    // 画面右下のアイテム欄
    class ItemBar final : public Component::ComponentBase,
                          public LifeCycleCallback::IUpdatable
    {
    public:
        void Initialize(const std::shared_ptr<IItemBarSource>& source);

    private:
        void OnUpdate() override;

        /// 見せる枠数に足りない分だけ枠を生成し、帯の位置を合わせ直す
        void SpawnSlots(const GameCore::PlayerAvatar::ItemPouch& pouch);
        /// 枠の中身を作り直す。
        void RefreshContent(const GameCore::PlayerAvatar::ItemPouch& pouch);
        void PresentSlots(const GameCore::PlayerAvatar::ItemPouch& pouch) const;
        void FadeOutSlots() const;
        void ApplyStripSlide() const;
        /// 前の選択から今の選択まで、回り込みを含めて近い向きに何枠動いたか
        [[nodiscard]] int SelectionStep(const GameCore::PlayerAvatar::ItemPouch& pouch) const;
        void ApplyDeviceGlyphs() const;
        /// 左から i 番目の枠が映すポーチの添字。選択中が中央に来るように回す
        [[nodiscard]] std::size_t PouchIndexOf(const GameCore::PlayerAvatar::ItemPouch& pouch, std::size_t slotIndex) const;
        [[nodiscard]] std::size_t CenterSlotIndex() const { return visibleCount_ / 2; }

        [[serialize(0)]] FIELD(GameObject::IGameObject) slots_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) slotPrefab_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) namePlate_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) cycleGlyph_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) cycleLabel_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) useGlyph_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) useLabel_;

        [[serialize(0)]] FIELD(Asset::SpriteFile) padCycleSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyCycleSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padUseSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyUseSprite_;

        [[serialize(0)]] int   maxVisibleSlots_ = 5;
        /// 枠の間隔。HorizontalLayoutGroup の cellSize_.x と揃えること(帯の右端を固定するのに使う)
        [[serialize(0)]] float slotPitch_px_ = 76.0f;
        [[serialize(0)]] float selectedScale_ = 1.0f;
        [[serialize(0)]] float unselectedScale_ = 0.74f;
        [[serialize(0)]] int   dimAlpha_ = 140;
        /// 使い切った枠の薄さ
        [[serialize(0)]] float emptyAlphaRate_ = 0.4f;
        /// 使えない State のときの薄さ
        [[serialize(0)]] float unusableAlphaRate_ = 0.6f;
        [[serialize(0)]] float fadeDuration_secs_ = 0.25f;
        [[serialize(0)]] float selectPulseDuration_secs_ = 0.3f;
        [[serialize(0)]] int   selectGlowMaxAlpha_ = 210;
        [[serialize(1)]] float slideDuration_secs_ = 0.18f;
        [[serialize(1)]] float selectPopRate_ = 0.12f;
        [[serialize(2)]] FIELD(Asset::UiSoundBankData) uiSounds_;

        std::shared_ptr<IItemBarSource> source_;
        std::vector<std::weak_ptr<ItemSlot>> slotViews_;
        std::size_t   visibleCount_ = 0;
        LibCore::Tween::TweenPlayer<float> barFade_;
        LibCore::Tween::TweenPlayer<float> selectPulse_;

        LibCore::Tween::TweenPlayer<float> slideTween_;
        float         stripBaseX_ = 0.0f;
        std::uint32_t lastRevision_ = 0;
        std::size_t   lastSelectedIndex_ = 0;
        bool          isContentDirty_ = true;
        bool          isShownDeclared_ = false;
        bool          isUsableDeclared_ = false;
        bool          isDeviceDirty_ = true;
        GameCore::PlayerAvatar::PlayerAvatarInputDevice device_ = GameCore::PlayerAvatar::PlayerAvatarInputDevice::KeyboardMouse;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(slots_));
            archive(CEREAL_NVP(slotPrefab_));
            archive(CEREAL_NVP(namePlate_));
            archive(CEREAL_NVP(nameText_));
            archive(CEREAL_NVP(cycleGlyph_));
            archive(CEREAL_NVP(cycleLabel_));
            archive(CEREAL_NVP(useGlyph_));
            archive(CEREAL_NVP(useLabel_));
            archive(CEREAL_NVP(padCycleSprite_));
            archive(CEREAL_NVP(keyCycleSprite_));
            archive(CEREAL_NVP(padUseSprite_));
            archive(CEREAL_NVP(keyUseSprite_));
            archive(CEREAL_NVP(maxVisibleSlots_));
            archive(CEREAL_NVP(slotPitch_px_));
            archive(CEREAL_NVP(selectedScale_));
            archive(CEREAL_NVP(unselectedScale_));
            archive(CEREAL_NVP(dimAlpha_));
            archive(CEREAL_NVP(emptyAlphaRate_));
            archive(CEREAL_NVP(unusableAlphaRate_));
            archive(CEREAL_NVP(fadeDuration_secs_));
            archive(CEREAL_NVP(selectPulseDuration_secs_));
            archive(CEREAL_NVP(selectGlowMaxAlpha_));
            archive(CEREAL_NVP(slideDuration_secs_));
            archive(CEREAL_NVP(selectPopRate_));
            archive(CEREAL_NVP(uiSounds_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(slots_));
            if (version >= 0) archive(CEREAL_NVP(slotPrefab_));
            if (version >= 0) archive(CEREAL_NVP(namePlate_));
            if (version >= 0) archive(CEREAL_NVP(nameText_));
            if (version >= 0) archive(CEREAL_NVP(cycleGlyph_));
            if (version >= 0) archive(CEREAL_NVP(cycleLabel_));
            if (version >= 0) archive(CEREAL_NVP(useGlyph_));
            if (version >= 0) archive(CEREAL_NVP(useLabel_));
            if (version >= 0) archive(CEREAL_NVP(padCycleSprite_));
            if (version >= 0) archive(CEREAL_NVP(keyCycleSprite_));
            if (version >= 0) archive(CEREAL_NVP(padUseSprite_));
            if (version >= 0) archive(CEREAL_NVP(keyUseSprite_));
            if (version >= 0) archive(CEREAL_NVP(maxVisibleSlots_));
            if (version >= 0) archive(CEREAL_NVP(slotPitch_px_));
            if (version >= 0) archive(CEREAL_NVP(selectedScale_));
            if (version >= 0) archive(CEREAL_NVP(unselectedScale_));
            if (version >= 0) archive(CEREAL_NVP(dimAlpha_));
            if (version >= 0) archive(CEREAL_NVP(emptyAlphaRate_));
            if (version >= 0) archive(CEREAL_NVP(unusableAlphaRate_));
            if (version >= 0) archive(CEREAL_NVP(fadeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(selectPulseDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(selectGlowMaxAlpha_));
            if (version >= 1) archive(CEREAL_NVP(slideDuration_secs_));
            if (version >= 1) archive(CEREAL_NVP(selectPopRate_));
            if (version >= 2) archive(CEREAL_NVP(uiSounds_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::ItemBar, 2);
