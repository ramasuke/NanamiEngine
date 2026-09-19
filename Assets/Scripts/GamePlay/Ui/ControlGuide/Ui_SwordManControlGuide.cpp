#include "Ui_SwordManControlGuide.h"

#include <optional>

#include "../../../Core/Game/PlayerAvatar/SwordMan/Status/ControlGuideFocus/SwordMan_IControlGuideFocusPresentation.h"
#include "../../PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GamePlay::Ui
{
    namespace
    {
        using GameCore::PlayerAvatar::PlayerAvatarInputDevice;
        using GameCore::PlayerAvatar::PlayerAvatarControlAcceptance;
        using GameCore::PlayerAvatar::SwordMan::SwordManAvatarInput;
        using GameCore::PlayerAvatar::PlayerAvatarInputPhase;
        using GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateAction;
        using GameCore::PlayerAvatar::SwordMan::SwordManAvatarStateType;
        using GameCore::PlayerAvatar::SwordMan::SwordManControlGuideFocus;
        using GameCore::PlayerAvatar::SwordMan::SwordManControlGuideFocusState;
    }

    class SwordManControlGuide::RequestCollector final : public GameCore::PlayerAvatar::SwordMan::ISwordManAvatarTransitionVisitor
    {
    public:
        [[nodiscard]] const RowRequests& Requests() const { return requests_; }

        bool Automatic(SwordManAvatarStateType, bool) override { return false; }

        bool OnInput(const SwordManAvatarStateType to, const SwordManAvatarInput input, const PlayerAvatarInputPhase phase, const bool isUsable) override
        {
            if (const auto label = TransitionLabel(to, phase))
                Offer(InputGlyph(input), *label, isUsable);
            return false;
        }

        bool OnInputWhenReady(const SwordManAvatarStateType to, const SwordManAvatarInput input, const PlayerAvatarInputPhase phase, const bool isUsable, bool) override
        {
            return OnInput(to, input, phase, isUsable);
        }

        void Action(const SwordManAvatarStateAction action, const bool isUsable) override
        {
            switch (action)
            {
            case SwordManAvatarStateAction::Move:          Offer(Glyph::Move,           Label::Move,          isUsable); return;
            case SwordManAvatarStateAction::ComboAttack:   Offer(Glyph::Attack,         Label::Attack,        isUsable); return;
            case SwordManAvatarStateAction::LockOn:        Offer(Glyph::LockOn,         Label::LockOn,        isUsable); return;
            case SwordManAvatarStateAction::LockOnRelease: Offer(Glyph::LockOn,         Label::LockOnRelease, isUsable); return;
            case SwordManAvatarStateAction::CannonTurn:    Offer(Glyph::MoveHorizontal, Label::CannonTurn,    isUsable); return;
            case SwordManAvatarStateAction::CannonFire:    Offer(Glyph::Attack,         Label::CannonFire,    isUsable); return;
            // アイテムの切替/使用は専用のアイテム欄が出すので、操作ガイドには行を持たない
            case SwordManAvatarStateAction::CycleItem:
            case SwordManAvatarStateAction::UseItem:
            case SwordManAvatarStateAction::OpenMenu:      return;
            }
        }

        [[nodiscard]] static Row RowOf(const Label label)
        {
            switch (label)
            {
            case Label::Move:
            case Label::CannonTurn:          return Row::Move;
            case Label::Attack:
            case Label::DashAttack:
            case Label::JumpAttack:
            case Label::CannonFire:          return Row::Attack;
            case Label::ChargeAttackHold:
            case Label::ChargeAttackRelease: return Row::ChargeAttack;
            case Label::Run:                 return Row::Run;
            case Label::Jump:                return Row::Jump;
            case Label::AvoidRolling:        return Row::AvoidRolling;
            case Label::LockOn:
            case Label::LockOnRelease:       return Row::LockOn;
            case Label::Chat:
            case Label::WakeUp:              return Row::Interact;
            }
            return Row::Move;
        }

    private:
        // 同じ行に複数届いたら、先に届いた使える方を出す
        void Offer(const Glyph glyph, const Label label, const bool isUsable)
        {
            if (!isUsable && IsHiddenWhenUnusable(label))
                return;

            auto& request = requests_[static_cast<std::size_t>(RowOf(label))];
            if (request.isShown && (request.isUsable || !isUsable))
                return;
            request = RowRequest{ true, isUsable, glyph, label };
        }

        [[nodiscard]] static std::optional<Label> TransitionLabel(const SwordManAvatarStateType to, const PlayerAvatarInputPhase phase)
        {
            switch (to)
            {
            case SwordManAvatarStateType::Walk:
            case SwordManAvatarStateType::InjuredWalk:
                return phase == PlayerAvatarInputPhase::Holding ? std::optional(Label::Move) : std::nullopt;
            case SwordManAvatarStateType::Run:
            case SwordManAvatarStateType::InjuredRun:
                return phase == PlayerAvatarInputPhase::Holding ? std::optional(Label::Run) : std::nullopt;
            case SwordManAvatarStateType::Jump:                 return Label::Jump;
            case SwordManAvatarStateType::AvoidRolling:         return Label::AvoidRolling;
            case SwordManAvatarStateType::NormalAttack:
                return phase == PlayerAvatarInputPhase::Pressed ? std::optional(Label::Attack) : std::nullopt;
            case SwordManAvatarStateType::JumpAttackAir:
                return phase == PlayerAvatarInputPhase::Pressed ? std::optional(Label::JumpAttack) : std::nullopt;
            case SwordManAvatarStateType::DashAttack:           return Label::DashAttack;
            case SwordManAvatarStateType::ChargeAttackCharging: return Label::ChargeAttackHold;
            case SwordManAvatarStateType::ChargeAttackRelease:  return Label::ChargeAttackRelease;
            case SwordManAvatarStateType::Chatting:             return Label::Chat;
            case SwordManAvatarStateType::WakeUp:               return Label::WakeUp;
            default:                                            return std::nullopt;
            }
        }

        [[nodiscard]] static Glyph InputGlyph(const SwordManAvatarInput input)
        {
            switch (input)
            {
            case SwordManAvatarInput::Move:         return Glyph::Move;
            case SwordManAvatarInput::Run:          return Glyph::Run;
            case SwordManAvatarInput::Jump:         return Glyph::Jump;
            case SwordManAvatarInput::AvoidRolling: return Glyph::AvoidRolling;
            case SwordManAvatarInput::NormalAttack:
            case SwordManAvatarInput::DashAttack:
            case SwordManAvatarInput::CannonAttack: return Glyph::Attack;
            case SwordManAvatarInput::Chat:         return Glyph::Interact;
            case SwordManAvatarInput::LockOn:       return Glyph::LockOn;
            }
            return Glyph::Move;
        }

        // 条件が揃ったときにだけ現れる操作
        [[nodiscard]] static bool IsHiddenWhenUnusable(const Label label)
        {
            return label == Label::Chat || label == Label::WakeUp || label == Label::LockOn || label == Label::LockOnRelease;
        }

        RowRequests requests_{};
    };

    void SwordManControlGuide::Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar)
    {
        swordManAvatar_ = swordManAvatar;
        if (controlGuide_)
            controlGuide_->SpawnRows(static_cast<std::size_t>(Row::Count));
    }

    SwordManControlGuide::Row SwordManControlGuide::FocusRow(const SwordManControlGuideFocus target)
    {
        switch (target)
        {
        case SwordManControlGuideFocus::Move:         return Row::Move;
        case SwordManControlGuideFocus::Attack:       return Row::Attack;
        case SwordManControlGuideFocus::ChargeAttack: return Row::ChargeAttack;
        case SwordManControlGuideFocus::Run:          return Row::Run;
        case SwordManControlGuideFocus::Jump:         return Row::Jump;
        case SwordManControlGuideFocus::AvoidRolling: return Row::AvoidRolling;
        case SwordManControlGuideFocus::LockOn:       return Row::LockOn;
        case SwordManControlGuideFocus::Interact:     return Row::Interact;
        case SwordManControlGuideFocus::None:         break;
        }
        return Row::Count;
    }

    void SwordManControlGuide::ApplyFocusRequest(const SwordManControlGuideFocus target)
    {
        const Row row = FocusRow(target);
        if (row == Row::Count)
            return;

        auto& request = requests_[static_cast<std::size_t>(row)];
        if (request.isShown)
            return;

        // State が出していない操作でも、指された行は「まだ使えない」姿で見せる
        switch (row)
        {
        case Row::Move:         request = RowRequest{ true, false, Glyph::Move,         Label::Move             }; return;
        case Row::Attack:       request = RowRequest{ true, false, Glyph::Attack,       Label::Attack           }; return;
        case Row::ChargeAttack: request = RowRequest{ true, false, Glyph::Attack,       Label::ChargeAttackHold }; return;
        case Row::Run:          request = RowRequest{ true, false, Glyph::Run,          Label::Run              }; return;
        case Row::Jump:         request = RowRequest{ true, false, Glyph::Jump,         Label::Jump             }; return;
        case Row::AvoidRolling: request = RowRequest{ true, false, Glyph::AvoidRolling, Label::AvoidRolling     }; return;
        case Row::LockOn:       request = RowRequest{ true, false, Glyph::LockOn,       Label::LockOn           }; return;
        case Row::Interact:     request = RowRequest{ true, false, Glyph::Interact,     Label::Chat             }; return;
        case Row::Count:        return;
        }
    }

    void SwordManControlGuide::OnUpdate()
    {
        const auto swordManAvatar = swordManAvatar_.lock();
        const auto state = swordManAvatar ? swordManAvatar->GetStateMachine().CurrentStateValue() : nullptr;
        const auto acceptance = state ? state->ControlAcceptance() : PlayerAvatarControlAcceptance::None;
        if (swordManAvatar)
            device_ = swordManAvatar->GetInputAction().CurrentDevice();

        if (acceptance == PlayerAvatarControlAcceptance::Accept)
        {
            RequestCollector collector;
            state->VisitTransitions(collector);
            requests_ = collector.Requests();
        }

        const SwordManControlGuideFocusState focus =
            swordManAvatar ? swordManAvatar->PlayerStatus().GuideFocusPresentation().Current() : SwordManControlGuideFocusState{};
        const Row focusedRow = FocusRow(focus.target);
        if (focusedRow != Row::Count)
            ApplyFocusRequest(focus.target);

        if (controlGuide_)
        {
            std::array<ControlGuide::RowRequest, static_cast<std::size_t>(Row::Count)> rows{};
            for (std::size_t i = 0; i < rows.size(); ++i)
            {
                const RowRequest& request = requests_[i];
                rows[i] = ControlGuide::RowRequest{ request.isShown, request.isUsable, GlyphSprite(request.glyph), LabelText(request.label) };
            }
            controlGuide_->Present(
                acceptance != PlayerAvatarControlAcceptance::None,
                rows,
                focusedRow == Row::Count ? std::nullopt : std::optional(static_cast<std::size_t>(focusedRow)),
                focus.isCleared);
        }

        ReportFocusAnchor(swordManAvatar, focusedRow);
    }

    void SwordManControlGuide::ReportFocusAnchor(
        const std::shared_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar, const Row focusedRow) const
    {
        if (!swordManAvatar)
            return;

        auto& presentation = swordManAvatar->PlayerStatus().GuideFocusPresentation();
        if (focusedRow == Row::Count || !controlGuide_)
        {
            presentation.ReportFocusAnchor(std::nullopt);
            return;
        }
        presentation.ReportFocusAnchor(controlGuide_->RowAnchor(static_cast<std::size_t>(focusedRow)));
    }

    std::shared_ptr<Asset::SpriteFile> SwordManControlGuide::GlyphSprite(const Glyph glyph) const
    {
        const bool isPad = device_ == PlayerAvatarInputDevice::Gamepad;
        switch (glyph)
        {
        case Glyph::Move:           return (isPad ? padMoveSprite_           : keyMoveSprite_          ).get();
        case Glyph::MoveHorizontal: return (isPad ? padMoveHorizontalSprite_ : keyMoveHorizontalSprite_).get();
        case Glyph::Attack:         return (isPad ? padAttackSprite_         : keyAttackSprite_        ).get();
        case Glyph::Run:            return (isPad ? padRunSprite_            : keyRunSprite_           ).get();
        case Glyph::Jump:           return (isPad ? padJumpSprite_           : keyJumpSprite_          ).get();
        case Glyph::AvoidRolling:   return (isPad ? padAvoidRollingSprite_   : keyAvoidRollingSprite_  ).get();
        case Glyph::LockOn:         return (isPad ? padLockOnSprite_         : keyLockOnSprite_        ).get();
        case Glyph::Interact:       return (isPad ? padInteractSprite_       : keyInteractSprite_      ).get();
        }
        return nullptr;
    }

    const std::string& SwordManControlGuide::LabelText(const Label label) const
    {
        switch (label)
        {
        case Label::Move:                return moveLabel_;
        case Label::Attack:              return attackLabel_;
        case Label::DashAttack:          return dashAttackLabel_;
        case Label::JumpAttack:          return jumpAttackLabel_;
        case Label::ChargeAttackHold:    return chargeAttackHoldLabel_;
        case Label::ChargeAttackRelease: return chargeAttackReleaseLabel_;
        case Label::Run:                 return runLabel_;
        case Label::Jump:                return jumpLabel_;
        case Label::AvoidRolling:        return avoidRollingLabel_;
        case Label::LockOn:              return lockOnLabel_;
        case Label::LockOnRelease:       return lockOnReleaseLabel_;
        case Label::Chat:                return chatLabel_;
        case Label::WakeUp:              return wakeUpLabel_;
        case Label::CannonTurn:          return cannonTurnLabel_;
        case Label::CannonFire:          return cannonFireLabel_;
        }
        return moveLabel_;
    }

    void SwordManControlGuide::OnDrawGui()
    {
        ImGui::Text("device_: %s", device_ == PlayerAvatarInputDevice::Gamepad ? "Gamepad" : "KeyboardMouse");

        ImGuiHelper::OnDrawInputField("controlGuide_", controlGuide_);
        ImGuiHelper::OnDrawInputField("keyMoveSprite_", keyMoveSprite_);
        ImGuiHelper::OnDrawInputField("keyMoveHorizontalSprite_", keyMoveHorizontalSprite_);
        ImGuiHelper::OnDrawInputField("keyAttackSprite_", keyAttackSprite_);
        ImGuiHelper::OnDrawInputField("keyRunSprite_", keyRunSprite_);
        ImGuiHelper::OnDrawInputField("keyJumpSprite_", keyJumpSprite_);
        ImGuiHelper::OnDrawInputField("keyAvoidRollingSprite_", keyAvoidRollingSprite_);
        ImGuiHelper::OnDrawInputField("keyLockOnSprite_", keyLockOnSprite_);
        ImGuiHelper::OnDrawInputField("keyInteractSprite_", keyInteractSprite_);
        ImGuiHelper::OnDrawInputField("padMoveSprite_", padMoveSprite_);
        ImGuiHelper::OnDrawInputField("padMoveHorizontalSprite_", padMoveHorizontalSprite_);
        ImGuiHelper::OnDrawInputField("padAttackSprite_", padAttackSprite_);
        ImGuiHelper::OnDrawInputField("padRunSprite_", padRunSprite_);
        ImGuiHelper::OnDrawInputField("padJumpSprite_", padJumpSprite_);
        ImGuiHelper::OnDrawInputField("padAvoidRollingSprite_", padAvoidRollingSprite_);
        ImGuiHelper::OnDrawInputField("padLockOnSprite_", padLockOnSprite_);
        ImGuiHelper::OnDrawInputField("padInteractSprite_", padInteractSprite_);

        ImGuiHelper::OnDrawInputField("moveLabel_", moveLabel_);
        ImGuiHelper::OnDrawInputField("attackLabel_", attackLabel_);
        ImGuiHelper::OnDrawInputField("dashAttackLabel_", dashAttackLabel_);
        ImGuiHelper::OnDrawInputField("jumpAttackLabel_", jumpAttackLabel_);
        ImGuiHelper::OnDrawInputField("chargeAttackHoldLabel_", chargeAttackHoldLabel_);
        ImGuiHelper::OnDrawInputField("chargeAttackReleaseLabel_", chargeAttackReleaseLabel_);
        ImGuiHelper::OnDrawInputField("runLabel_", runLabel_);
        ImGuiHelper::OnDrawInputField("jumpLabel_", jumpLabel_);
        ImGuiHelper::OnDrawInputField("avoidRollingLabel_", avoidRollingLabel_);
        ImGuiHelper::OnDrawInputField("lockOnLabel_", lockOnLabel_);
        ImGuiHelper::OnDrawInputField("lockOnReleaseLabel_", lockOnReleaseLabel_);
        ImGuiHelper::OnDrawInputField("chatLabel_", chatLabel_);
        ImGuiHelper::OnDrawInputField("wakeUpLabel_", wakeUpLabel_);
        ImGuiHelper::OnDrawInputField("cannonTurnLabel_", cannonTurnLabel_);
        ImGuiHelper::OnDrawInputField("cannonFireLabel_", cannonFireLabel_);
    }
}
