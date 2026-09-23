#pragma once
#include <array>
#include <memory>

#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/CircleGaugeRenderer/CircleGaugeRenderer.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "../../../Core/Game/PlayerAvatar/InputAction/PlayerAvatarInputDevice.h"
#include "../../../Core/Game/PlayerAvatar/MagicCaster/Spell/MagicCasterSpellSlot.h"

namespace GamePlay::PlayerAvatar::MagicCaster
{
    class MagicCasterAvatar;
}

namespace GamePlay::Ui
{
    class SpellSlot;

    // 画面左下の魔法陣。1ページ目を上下左右、2ページ目を斜めに置き、LT+RB（右クリック）で入れ替える。
    // 外周の弧が MP。置き場所は子の Anchor オブジェクト、枠は slotPrefab_ から slotsRoot_ の子へ生成する。
    // 画像と配置の数値は tools/art/spell_palette.py / spell_palette_prefab.py が作る
    class SpellPalette final : public Component::ComponentBase,
                               public LifeCycleCallback::IUpdatable
    {
    public:
        void Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar>& avatar);

    private:
        void OnUpdate() override;

        void SpawnSlots();
        /// 枠のアイコンと消費 MP を作り直す。装備が変わったときだけ呼ぶ
        void RefreshSpells();
        void ApplyDeviceGlyphs();
        void PresentSlots(float groupAlpha);
        void PresentNames(float groupAlpha) const;
        void PresentMana(float groupAlpha) const;
        void FadeOut();
        /// 向き（0=上 1=右 2=下 3=左）ごとの手前の置き場所と奥の置き場所
        [[nodiscard]] glm::vec3 FrontAnchorPos(int direction) const;
        [[nodiscard]] glm::vec3 BackAnchorPos (int direction) const;
        [[nodiscard]] int FrontPage() const { return pageSwap_.Value() >= 0.5f ? 1 : 0; }

        [[serialize(0)]] FIELD(GameObject::IGameObject) slotsRoot_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) slotPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) anchorTop_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) anchorRight_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) anchorBottom_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) anchorLeft_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) anchorTopRight_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) anchorBottomRight_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) anchorBottomLeft_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) anchorTopLeft_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameTop_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameRight_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameBottom_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameLeft_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) halo_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) circle_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) manaTrack_;
        [[serialize(0)]] FIELD(NanamiUi::CircleGaugeRenderer) manaFill_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) manaTip_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) manaLabel_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) manaValue_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) paletteGlyph_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) pageGlyph_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) pageText_;

        [[serialize(0)]] FIELD(Asset::SpriteFile) padGlyphTop_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padGlyphRight_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padGlyphBottom_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padGlyphLeft_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyGlyphTop_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyGlyphRight_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyGlyphBottom_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyGlyphLeft_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padPaletteSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyPaletteSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) padPageSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) keyPageSprite_;

        /// MP の弧。manaFill_ の startPercent_ / spanPercent_ と同じ弧を角度で持つ（先端の光を置くのに使う）
        [[serialize(0)]] float manaArcRadius_ = 138.0f;
        [[serialize(0)]] float manaArcStartDeg_ = 210.0f;
        [[serialize(0)]] float manaArcSpanDeg_ = 300.0f;
        /// LT を離している間の濃さ
        [[serialize(0)]] float idleAlphaRate_ = 0.75f;
        /// 奥のページ（斜め）の大きさと濃さ
        [[serialize(0)]] float backScale_ = 0.56f;
        [[serialize(0)]] float backAlphaRate_ = 0.6f;
        /// MP が足りない魔法のアイコンの濃さ
        [[serialize(0)]] float manaLackIconRate_ = 0.45f;
        [[serialize(0)]] float fadeDuration_secs_ = 0.2f;
        [[serialize(0)]] float pageSwapDuration_secs_ = 0.15f;
        [[serialize(0)]] Color32 manaValueColor_ = Color32(248, 246, 255);

        std::weak_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar> avatar_;
        std::array<std::weak_ptr<SpellSlot>, GameCore::PlayerAvatar::MagicCaster::SPELL_LOADOUT_SLOT_COUNT> slotViews_;
        std::array<bool, GameCore::PlayerAvatar::MagicCaster::SPELL_LOADOUT_SLOT_COUNT> shownManaLack_ {};
        LibCore::Tween::TweenPlayer<float> visibleFade_;
        LibCore::Tween::TweenPlayer<float> openFade_;
        LibCore::Tween::TweenPlayer<float> pageSwap_;
        bool  isSpawned_      = false;
        bool  isSpellsDirty_  = true;
        bool  isDeviceDirty_  = true;
        GameCore::PlayerAvatar::PlayerAvatarInputDevice device_ = GameCore::PlayerAvatar::PlayerAvatarInputDevice::KeyboardMouse;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(slotsRoot_));
            archive(CEREAL_NVP(slotPrefab_));
            archive(CEREAL_NVP(anchorTop_));
            archive(CEREAL_NVP(anchorRight_));
            archive(CEREAL_NVP(anchorBottom_));
            archive(CEREAL_NVP(anchorLeft_));
            archive(CEREAL_NVP(anchorTopRight_));
            archive(CEREAL_NVP(anchorBottomRight_));
            archive(CEREAL_NVP(anchorBottomLeft_));
            archive(CEREAL_NVP(anchorTopLeft_));
            archive(CEREAL_NVP(nameTop_));
            archive(CEREAL_NVP(nameRight_));
            archive(CEREAL_NVP(nameBottom_));
            archive(CEREAL_NVP(nameLeft_));
            archive(CEREAL_NVP(halo_));
            archive(CEREAL_NVP(circle_));
            archive(CEREAL_NVP(manaTrack_));
            archive(CEREAL_NVP(manaFill_));
            archive(CEREAL_NVP(manaTip_));
            archive(CEREAL_NVP(manaLabel_));
            archive(CEREAL_NVP(manaValue_));
            archive(CEREAL_NVP(paletteGlyph_));
            archive(CEREAL_NVP(pageGlyph_));
            archive(CEREAL_NVP(pageText_));
            archive(CEREAL_NVP(padGlyphTop_));
            archive(CEREAL_NVP(padGlyphRight_));
            archive(CEREAL_NVP(padGlyphBottom_));
            archive(CEREAL_NVP(padGlyphLeft_));
            archive(CEREAL_NVP(keyGlyphTop_));
            archive(CEREAL_NVP(keyGlyphRight_));
            archive(CEREAL_NVP(keyGlyphBottom_));
            archive(CEREAL_NVP(keyGlyphLeft_));
            archive(CEREAL_NVP(padPaletteSprite_));
            archive(CEREAL_NVP(keyPaletteSprite_));
            archive(CEREAL_NVP(padPageSprite_));
            archive(CEREAL_NVP(keyPageSprite_));
            archive(CEREAL_NVP(manaArcRadius_));
            archive(CEREAL_NVP(manaArcStartDeg_));
            archive(CEREAL_NVP(manaArcSpanDeg_));
            archive(CEREAL_NVP(idleAlphaRate_));
            archive(CEREAL_NVP(backScale_));
            archive(CEREAL_NVP(backAlphaRate_));
            archive(CEREAL_NVP(manaLackIconRate_));
            archive(CEREAL_NVP(fadeDuration_secs_));
            archive(CEREAL_NVP(pageSwapDuration_secs_));
            archive(CEREAL_NVP(manaValueColor_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(slotsRoot_));
            if (version >= 0) archive(CEREAL_NVP(slotPrefab_));
            if (version >= 0) archive(CEREAL_NVP(anchorTop_));
            if (version >= 0) archive(CEREAL_NVP(anchorRight_));
            if (version >= 0) archive(CEREAL_NVP(anchorBottom_));
            if (version >= 0) archive(CEREAL_NVP(anchorLeft_));
            if (version >= 0) archive(CEREAL_NVP(anchorTopRight_));
            if (version >= 0) archive(CEREAL_NVP(anchorBottomRight_));
            if (version >= 0) archive(CEREAL_NVP(anchorBottomLeft_));
            if (version >= 0) archive(CEREAL_NVP(anchorTopLeft_));
            if (version >= 0) archive(CEREAL_NVP(nameTop_));
            if (version >= 0) archive(CEREAL_NVP(nameRight_));
            if (version >= 0) archive(CEREAL_NVP(nameBottom_));
            if (version >= 0) archive(CEREAL_NVP(nameLeft_));
            if (version >= 0) archive(CEREAL_NVP(halo_));
            if (version >= 0) archive(CEREAL_NVP(circle_));
            if (version >= 0) archive(CEREAL_NVP(manaTrack_));
            if (version >= 0) archive(CEREAL_NVP(manaFill_));
            if (version >= 0) archive(CEREAL_NVP(manaTip_));
            if (version >= 0) archive(CEREAL_NVP(manaLabel_));
            if (version >= 0) archive(CEREAL_NVP(manaValue_));
            if (version >= 0) archive(CEREAL_NVP(paletteGlyph_));
            if (version >= 0) archive(CEREAL_NVP(pageGlyph_));
            if (version >= 0) archive(CEREAL_NVP(pageText_));
            if (version >= 0) archive(CEREAL_NVP(padGlyphTop_));
            if (version >= 0) archive(CEREAL_NVP(padGlyphRight_));
            if (version >= 0) archive(CEREAL_NVP(padGlyphBottom_));
            if (version >= 0) archive(CEREAL_NVP(padGlyphLeft_));
            if (version >= 0) archive(CEREAL_NVP(keyGlyphTop_));
            if (version >= 0) archive(CEREAL_NVP(keyGlyphRight_));
            if (version >= 0) archive(CEREAL_NVP(keyGlyphBottom_));
            if (version >= 0) archive(CEREAL_NVP(keyGlyphLeft_));
            if (version >= 0) archive(CEREAL_NVP(padPaletteSprite_));
            if (version >= 0) archive(CEREAL_NVP(keyPaletteSprite_));
            if (version >= 0) archive(CEREAL_NVP(padPageSprite_));
            if (version >= 0) archive(CEREAL_NVP(keyPageSprite_));
            if (version >= 0) archive(CEREAL_NVP(manaArcRadius_));
            if (version >= 0) archive(CEREAL_NVP(manaArcStartDeg_));
            if (version >= 0) archive(CEREAL_NVP(manaArcSpanDeg_));
            if (version >= 0) archive(CEREAL_NVP(idleAlphaRate_));
            if (version >= 0) archive(CEREAL_NVP(backScale_));
            if (version >= 0) archive(CEREAL_NVP(backAlphaRate_));
            if (version >= 0) archive(CEREAL_NVP(manaLackIconRate_));
            if (version >= 0) archive(CEREAL_NVP(fadeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(pageSwapDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(manaValueColor_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::SpellPalette, 0);
