#pragma once
#include <array>
#include <memory>
#include <string>
#include "Ui_ControlGuide.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "../../../Core/Game/PlayerAvatar/InputAction/PlayerAvatarInputDevice.h"
#include "../../../Core/Game/PlayerAvatar/SwordMan/State/Transition/SwordManAvatarStateTransition.h"
#include "../../../Core/Game/PlayerAvatar/SwordMan/State/Transition/SwordManControlGuideFocus.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GamePlay::Ui
{
    // State が宣言する遷移と操作から、どの行に何を出すかを決める。見せ方は controlGuide_ が行う
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

        using RowRequests = std::array<RowRequest, static_cast<std::size_t>(Row::Count)>;

        void OnUpdate() override;

        /// チュートリアルが指した行は、State が出していなくても薄く出す
        void ApplyFocusRequest(GameCore::PlayerAvatar::SwordMan::SwordManControlGuideFocus target);
        [[nodiscard]] std::shared_ptr<Asset::SpriteFile> GlyphSprite(Glyph glyph) const;
        [[nodiscard]] const std::string& LabelText(Label label) const;
        [[nodiscard]] static Row FocusRow(GameCore::PlayerAvatar::SwordMan::SwordManControlGuideFocus target);
        /// 吹き出しを出す側が行の位置を知れるように、指している行の画面座標を返す
        void ReportFocusAnchor(const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar, Row focusedRow) const;

        [[serialize(3)]] FIELD(Ui::ControlGuide) controlGuide_;

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

        std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> swordManAvatar_;
        RowRequests requests_{};
        GameCore::PlayerAvatar::PlayerAvatarInputDevice device_ = GameCore::PlayerAvatar::PlayerAvatarInputDevice::KeyboardMouse;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(controlGuide_));
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
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            // v3 で行の生成と見せ方を ControlGuide へ移した
            if (version >= 3) archive(CEREAL_NVP(controlGuide_));
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
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::SwordManControlGuide, 3)
