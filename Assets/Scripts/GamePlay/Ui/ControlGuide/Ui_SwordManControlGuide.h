#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../Core/Game/PlayerAvatar/InputAction/PlayerAvatarInputDevice.h"
#include "../../../Core/Game/PlayerAvatar/SwordMan/State/Transition/SwordManAvatarStateTransition.h"
#include "../../../Core/Game/PlayerAvatar/SwordMan/State/Transition/SwordManControlGuideFocus.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GamePlay::Ui
{
    class SwordManControlGuideRow;

    // State が宣言する遷移と操作から、どの行に何を出すかを決める。
    // 描画は rowPrefab_ から rows_ の子へ Row の順に生成した SwordManControlGuideRow が行い、行の積み上げは VerticalLayoutGroup が行う
    class SwordManControlGuide final : public Component::ComponentBase,
                                       public LifeCycleCallback::IUpdatable
    {
    public:
        void Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar);

    private:
        /// State が宣言する遷移と操作を、ガイドの行へ振り分ける
        class RequestCollector;

        // 押す操作そのもの。実際に出す絵は接続中の入力機器で選ぶ
        enum class Glyph : std::uint8_t
        {
            Move,
            MoveHorizontal,
            Attack,
            Run,
            Jump,
            AvoidRolling,
            LockOn,
            Interact,
        };

        enum class Label : std::uint8_t
        {
            Move,
            Attack,
            DashAttack,
            JumpAttack,
            ChargeAttackHold,
            ChargeAttackRelease,
            Run,
            Jump,
            AvoidRolling,
            LockOn,
            LockOnRelease,
            Chat,
            WakeUp,
            CannonTurn,
            CannonFire,
        };

        // 下から並ぶ順。行はこの順に生成する
        enum class Row : std::uint8_t
        {
            Move,
            Attack,
            ChargeAttack,
            Run,
            Jump,
            AvoidRolling,
            LockOn,
            Interact,
            Count,
        };

        struct RowRequest
        {
            bool  isShown  = false;
            bool  isUsable = false;
            Glyph glyph    = Glyph::Move;
            Label label    = Label::Move;
        };

        struct RowState
        {
            Glyph glyph             = Glyph::Move;
            Label label             = Label::Move;
            float visibility        = 0.0f;
            float usableRate        = 0.0f;
            float focusRate         = 0.0f;
            float pulseElapsed_secs = 0.0f;
            bool  isActive          = false;
            bool  isContentDirty    = true;
            bool  isFocused         = false;
            bool  isFocusDirty      = true;
        };

        using RowRequests = std::array<RowRequest, static_cast<std::size_t>(Row::Count)>;

        void OnUpdate() override;

        void SpawnRows();
        /// チュートリアルが指した行は、State が出していなくても薄く出す
        void ApplyFocusRequest(GameCore::PlayerAvatar::SwordMan::SwordManControlGuideFocus target);
        void AnimateRow(RowState& row, const RowRequest& request, bool isFocused, float deltaTime) const;
        void PresentRow(SwordManControlGuideRow& view, RowState& row, const RowRequest& request, bool isCleared) const;
        [[nodiscard]] std::shared_ptr<Asset::SpriteFile> GlyphSprite(Glyph glyph) const;
        [[nodiscard]] const std::string& LabelText(Label label) const;
        [[nodiscard]] static Row FocusRow(GameCore::PlayerAvatar::SwordMan::SwordManControlGuideFocus target);
        /// 吹き出しを出す側が行の位置を知れるように、指している行の画面座標を返す
        void ReportFocusAnchor(const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar, Row focusedRow) const;

        [[serialize(1)]] FIELD(GameObject::IGameObject) rows_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;

        [[serialize(2)]] FIELD(Asset::SpriteFile) keyMoveSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) keyMoveHorizontalSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) keyAttackSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) keyRunSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) keyJumpSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) keyAvoidRollingSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) keyLockOnSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) keyInteractSprite_;

        [[serialize(2)]] FIELD(Asset::SpriteFile) padMoveSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) padMoveHorizontalSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) padAttackSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) padRunSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) padJumpSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) padAvoidRollingSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) padLockOnSprite_;
        [[serialize(2)]] FIELD(Asset::SpriteFile) padInteractSprite_;

        [[serialize(0)]] std::string moveLabel_;
        [[serialize(0)]] std::string attackLabel_;
        [[serialize(0)]] std::string dashAttackLabel_;
        [[serialize(2)]] std::string jumpAttackLabel_;
        [[serialize(0)]] std::string chargeAttackHoldLabel_;
        [[serialize(0)]] std::string chargeAttackReleaseLabel_;
        [[serialize(0)]] std::string runLabel_;
        [[serialize(0)]] std::string jumpLabel_;
        [[serialize(0)]] std::string avoidRollingLabel_;
        [[serialize(0)]] std::string lockOnLabel_;
        [[serialize(0)]] std::string lockOnReleaseLabel_;
        [[serialize(0)]] std::string chatLabel_;
        [[serialize(0)]] std::string wakeUpLabel_;
        [[serialize(0)]] std::string cannonTurnLabel_;
        [[serialize(0)]] std::string cannonFireLabel_;

        [[serialize(0)]] float slideDistance_px_ = 14.0f;
        [[serialize(0)]] float guideFadeDuration_secs_ = 0.3f;
        [[serialize(0)]] float rowFadeDuration_secs_ = 0.15f;
        [[serialize(0)]] float pulseDuration_secs_ = 0.45f;
        [[serialize(0)]] int accentGlowMaxAlpha_ = 230;
        [[serialize(0)]] int glyphFlashMaxAlpha_ = 115;
        [[serialize(0)]] int dimAlpha_ = 110;
        [[serialize(0)]] float labelShadowAlphaRate_ = 0.7f;

        [[serialize(2)]] float focusFadeDuration_secs_ = 0.18f;
        [[serialize(2)]] float focusPulsePeriod_secs_ = 0.9f;
        [[serialize(2)]] float focusArrowSwing_px_ = 5.0f;
        [[serialize(2)]] int focusGlyphFlashMaxAlpha_ = 90;
        /// フォーカス中、指していない行をさらに沈める割合
        [[serialize(2)]] float unfocusedDimRate_ = 0.3f;

        std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> swordManAvatar_;
        std::vector<std::weak_ptr<SwordManControlGuideRow>> rowViews_;
        RowRequests requests_{};
        std::array<RowState, static_cast<std::size_t>(Row::Count)> rowStates_{};
        float guideAlpha_ = 0.0f;
        float focusElapsed_secs_ = 0.0f;
        float anyFocusRate_ = 0.0f;
        GameCore::PlayerAvatar::PlayerAvatarInputDevice device_ = GameCore::PlayerAvatar::PlayerAvatarInputDevice::KeyboardMouse;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(rows_));
            archive(CEREAL_NVP(rowPrefab_));
            archive(CEREAL_NVP(keyMoveSprite_));
            archive(CEREAL_NVP(keyMoveHorizontalSprite_));
            archive(CEREAL_NVP(keyAttackSprite_));
            archive(CEREAL_NVP(keyRunSprite_));
            archive(CEREAL_NVP(keyJumpSprite_));
            archive(CEREAL_NVP(keyAvoidRollingSprite_));
            archive(CEREAL_NVP(keyLockOnSprite_));
            archive(CEREAL_NVP(keyInteractSprite_));
            archive(CEREAL_NVP(padMoveSprite_));
            archive(CEREAL_NVP(padMoveHorizontalSprite_));
            archive(CEREAL_NVP(padAttackSprite_));
            archive(CEREAL_NVP(padRunSprite_));
            archive(CEREAL_NVP(padJumpSprite_));
            archive(CEREAL_NVP(padAvoidRollingSprite_));
            archive(CEREAL_NVP(padLockOnSprite_));
            archive(CEREAL_NVP(padInteractSprite_));
            archive(CEREAL_NVP(moveLabel_));
            archive(CEREAL_NVP(attackLabel_));
            archive(CEREAL_NVP(dashAttackLabel_));
            archive(CEREAL_NVP(jumpAttackLabel_));
            archive(CEREAL_NVP(chargeAttackHoldLabel_));
            archive(CEREAL_NVP(chargeAttackReleaseLabel_));
            archive(CEREAL_NVP(runLabel_));
            archive(CEREAL_NVP(jumpLabel_));
            archive(CEREAL_NVP(avoidRollingLabel_));
            archive(CEREAL_NVP(lockOnLabel_));
            archive(CEREAL_NVP(lockOnReleaseLabel_));
            archive(CEREAL_NVP(chatLabel_));
            archive(CEREAL_NVP(wakeUpLabel_));
            archive(CEREAL_NVP(cannonTurnLabel_));
            archive(CEREAL_NVP(cannonFireLabel_));
            archive(CEREAL_NVP(slideDistance_px_));
            archive(CEREAL_NVP(guideFadeDuration_secs_));
            archive(CEREAL_NVP(rowFadeDuration_secs_));
            archive(CEREAL_NVP(pulseDuration_secs_));
            archive(CEREAL_NVP(accentGlowMaxAlpha_));
            archive(CEREAL_NVP(glyphFlashMaxAlpha_));
            archive(CEREAL_NVP(dimAlpha_));
            archive(CEREAL_NVP(labelShadowAlphaRate_));
            archive(CEREAL_NVP(focusFadeDuration_secs_));
            archive(CEREAL_NVP(focusPulsePeriod_secs_));
            archive(CEREAL_NVP(focusArrowSwing_px_));
            archive(CEREAL_NVP(focusGlyphFlashMaxAlpha_));
            archive(CEREAL_NVP(unfocusedDimRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            // v1 で Rows の名前検索を FIELD に置き換えた
            std::string rowsName_;
            if (version < 1) archive(CEREAL_NVP(rowsName_));
            if (version >= 1) archive(CEREAL_NVP(rows_));
            if (version >= 0) archive(CEREAL_NVP(rowPrefab_));
            // v2 でグリフをキー名から操作名に付け替え、ゲームパッド用の絵を足した
            if (version < 2)
            {
                archive(cereal::make_nvp("wasdSprite_", keyMoveSprite_));
                archive(cereal::make_nvp("adSprite_", keyMoveHorizontalSprite_));
                archive(cereal::make_nvp("keyQSprite_", keyLockOnSprite_));
                archive(cereal::make_nvp("keyESprite_", keyInteractSprite_));
                archive(cereal::make_nvp("keyShiftSprite_", keyRunSprite_));
                archive(cereal::make_nvp("keyCtrlSprite_", keyAvoidRollingSprite_));
                archive(cereal::make_nvp("keySpaceSprite_", keyJumpSprite_));
                archive(cereal::make_nvp("mouseLeftSprite_", keyAttackSprite_));
            }
            else
            {
                archive(CEREAL_NVP(keyMoveSprite_));
                archive(CEREAL_NVP(keyMoveHorizontalSprite_));
                archive(CEREAL_NVP(keyAttackSprite_));
                archive(CEREAL_NVP(keyRunSprite_));
                archive(CEREAL_NVP(keyJumpSprite_));
                archive(CEREAL_NVP(keyAvoidRollingSprite_));
                archive(CEREAL_NVP(keyLockOnSprite_));
                archive(CEREAL_NVP(keyInteractSprite_));
                archive(CEREAL_NVP(padMoveSprite_));
                archive(CEREAL_NVP(padMoveHorizontalSprite_));
                archive(CEREAL_NVP(padAttackSprite_));
                archive(CEREAL_NVP(padRunSprite_));
                archive(CEREAL_NVP(padJumpSprite_));
                archive(CEREAL_NVP(padAvoidRollingSprite_));
                archive(CEREAL_NVP(padLockOnSprite_));
                archive(CEREAL_NVP(padInteractSprite_));
            }
            if (version >= 0) archive(CEREAL_NVP(moveLabel_));
            if (version >= 0) archive(CEREAL_NVP(attackLabel_));
            if (version >= 0) archive(CEREAL_NVP(dashAttackLabel_));
            if (version >= 2) archive(CEREAL_NVP(jumpAttackLabel_));
            if (version >= 0) archive(CEREAL_NVP(chargeAttackHoldLabel_));
            if (version >= 0) archive(CEREAL_NVP(chargeAttackReleaseLabel_));
            if (version >= 0) archive(CEREAL_NVP(runLabel_));
            if (version >= 0) archive(CEREAL_NVP(jumpLabel_));
            if (version >= 0) archive(CEREAL_NVP(avoidRollingLabel_));
            if (version >= 0) archive(CEREAL_NVP(lockOnLabel_));
            if (version >= 0) archive(CEREAL_NVP(lockOnReleaseLabel_));
            if (version >= 0) archive(CEREAL_NVP(chatLabel_));
            if (version >= 0) archive(CEREAL_NVP(wakeUpLabel_));
            if (version >= 0) archive(CEREAL_NVP(cannonTurnLabel_));
            if (version >= 0) archive(CEREAL_NVP(cannonFireLabel_));
            if (version >= 0) archive(CEREAL_NVP(slideDistance_px_));
            if (version >= 0) archive(CEREAL_NVP(guideFadeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(rowFadeDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(pulseDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(accentGlowMaxAlpha_));
            if (version >= 0) archive(CEREAL_NVP(glyphFlashMaxAlpha_));
            if (version >= 0) archive(CEREAL_NVP(dimAlpha_));
            if (version >= 0) archive(CEREAL_NVP(labelShadowAlphaRate_));
            if (version >= 2) archive(CEREAL_NVP(focusFadeDuration_secs_));
            if (version >= 2) archive(CEREAL_NVP(focusPulsePeriod_secs_));
            if (version >= 2) archive(CEREAL_NVP(focusArrowSwing_px_));
            if (version >= 2) archive(CEREAL_NVP(focusGlyphFlashMaxAlpha_));
            if (version >= 2) archive(CEREAL_NVP(unfocusedDimRate_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SwordManControlGuide, 2)
