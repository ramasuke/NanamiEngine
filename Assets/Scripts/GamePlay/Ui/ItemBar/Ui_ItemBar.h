#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "../../../../../Libs/LibCore/cereal/glm/GlmHelper.h"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../Core/Game/PlayerAvatar/InputAction/PlayerAvatarInputDevice.h"
#include "../../../Core/Game/PlayerAvatar/Item/ItemPouch.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GamePlay::Ui
{
    class ItemSlot;

    // 画面右下のアイテム欄。選択中を中央に置いたまま左右へ回る帯で、枠は slotPrefab_ から slots_ の子へ生成する。
    // 出入りは State が宣言する CycleItem / UseItem を見て決めるので、
    // 宣言しない State(大砲に乗っている間など)では自動的に引っ込む
    class ItemBar final : public Component::ComponentBase,
                          public LifeCycleCallback::IUpdatable
    {
    public:
        void Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar);

    private:
        /// State が宣言する操作から、アイテム欄を出すか・使えるかだけを拾う
        class ActionCollector;

        void OnUpdate() override;

        /// 見せる枠数(ポーチの枠数と maxVisibleSlots_ の小さい方)に足りない分だけ枠を生成し、帯の位置を合わせ直す
        void SpawnSlots(const GameCore::PlayerAvatar::ItemPouch& pouch);
        /// 枠の中身(アイコン・個数・名前)を作り直す。ポーチが変わったときだけ呼ぶ
        void RefreshContent(const GameCore::PlayerAvatar::ItemPouch& pouch);
        void PresentSlots(const GameCore::PlayerAvatar::ItemPouch& pouch) const;
        void FadeOutSlots() const;
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

        std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> swordManAvatar_;
        std::vector<std::weak_ptr<ItemSlot>> slotViews_;
        std::size_t   visibleCount_ = 0;
        float         barAlpha_ = 0.0f;
        float         selectPulse_secs_ = 1000.0f;
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
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::ItemBar, 0)
