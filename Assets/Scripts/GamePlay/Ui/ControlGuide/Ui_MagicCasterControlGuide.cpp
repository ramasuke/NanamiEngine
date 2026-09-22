#include "Ui_MagicCasterControlGuide.h"

#include <optional>

#include "../../PlayerAvatar/MagicCaster/MagicCasterAvatar.h"

namespace GamePlay::Ui
{
    namespace
    {
        using GameCore::PlayerAvatar::PlayerAvatarInputDevice;
        using GameCore::PlayerAvatar::PlayerAvatarControlAcceptance;
        using GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarInput;
        using GameCore::PlayerAvatar::PlayerAvatarInputPhase;
        using GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStateAction;
        using GameCore::PlayerAvatar::MagicCaster::MagicCasterAvatarStateType;
    }

    class MagicCasterControlGuide::RequestCollector final : public GameCore::PlayerAvatar::MagicCaster::IMagicCasterAvatarTransitionVisitor
    {
    public:
        [[nodiscard]] const RowRequests& Requests() const { return requests_; }

        void OnInput(const MagicCasterAvatarStateType to, const MagicCasterAvatarInput input, const PlayerAvatarInputPhase phase, const bool isUsable, bool) override
        {
            if (const auto label = TransitionLabel(to, phase))
                Offer(InputGlyph(input), *label, isUsable);
        }

        void Cast(const bool isBasicSpellUsable) override
        {
            Offer(Glyph::Cast, Label::Cast, isBasicSpellUsable);
        }

        void Action(const MagicCasterAvatarStateAction action, const bool isUsable) override
        {
            switch (action)
            {
            case MagicCasterAvatarStateAction::Move:          Offer(Glyph::Move,   Label::Move,          isUsable); return;
            case MagicCasterAvatarStateAction::LockOn:        Offer(Glyph::LockOn, Label::LockOn,        isUsable); return;
            case MagicCasterAvatarStateAction::LockOnRelease: Offer(Glyph::LockOn, Label::LockOnRelease, isUsable); return;
            // アイテムの切替/使用は専用のアイテム欄が出すので、操作ガイドには行を持たない
            case MagicCasterAvatarStateAction::CycleItem:
            case MagicCasterAvatarStateAction::UseItem:       return;
            }
        }

    private:
        [[nodiscard]] static Row RowOf(const Label label)
        {
            switch (label)
            {
            case Label::Move:          return Row::Move;
            case Label::Cast:          return Row::Cast;
            case Label::Run:           return Row::Run;
            case Label::Jump:          return Row::Jump;
            case Label::LockOn:
            case Label::LockOnRelease: return Row::LockOn;
            case Label::Chat:          return Row::Interact;
            }
            return Row::Move;
        }

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

        [[nodiscard]] static std::optional<Label> TransitionLabel(const MagicCasterAvatarStateType to, const PlayerAvatarInputPhase phase)
        {
            switch (to)
            {
            case MagicCasterAvatarStateType::Walk:
                return phase == PlayerAvatarInputPhase::Holding ? std::optional(Label::Move) : std::nullopt;
            case MagicCasterAvatarStateType::Run:
                return phase == PlayerAvatarInputPhase::Holding ? std::optional(Label::Run) : std::nullopt;
            case MagicCasterAvatarStateType::Jump:     return Label::Jump;
            case MagicCasterAvatarStateType::Chatting: return Label::Chat;
            default:                                   return std::nullopt;
            }
        }

        [[nodiscard]] static Glyph InputGlyph(const MagicCasterAvatarInput input)
        {
            switch (input)
            {
            case MagicCasterAvatarInput::Move: return Glyph::Move;
            case MagicCasterAvatarInput::Run:  return Glyph::Run;
            case MagicCasterAvatarInput::Jump: return Glyph::Jump;
            case MagicCasterAvatarInput::Chat: return Glyph::Interact;
            }
            return Glyph::Move;
        }

        // 条件が揃ったときにだけ現れる操作
        [[nodiscard]] static bool IsHiddenWhenUnusable(const Label label)
        {
            return label == Label::Chat || label == Label::LockOn || label == Label::LockOnRelease;
        }

        RowRequests requests_{};
    };

    void MagicCasterControlGuide::Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::MagicCaster::MagicCasterAvatar>& magicCasterAvatar)
    {
        magicCasterAvatar_ = magicCasterAvatar;
        if (controlGuide_)
            controlGuide_->SpawnRows(static_cast<std::size_t>(Row::Count));
    }

    void MagicCasterControlGuide::OnUpdate()
    {
        const auto magicCasterAvatar = magicCasterAvatar_.lock();
        const auto state = magicCasterAvatar ? magicCasterAvatar->GetStateMachine().CurrentStateValue() : nullptr;
        const auto acceptance = state ? state->ControlAcceptance() : PlayerAvatarControlAcceptance::None;
        if (magicCasterAvatar)
            device_ = magicCasterAvatar->GetInputAction().CurrentDevice();

        if (acceptance == PlayerAvatarControlAcceptance::Accept)
        {
            RequestCollector collector;
            state->VisitTransitions(collector);
            requests_ = collector.Requests();
        }

        if (!controlGuide_)
            return;

        std::array<ControlGuide::RowRequest, static_cast<std::size_t>(Row::Count)> rows{};
        for (std::size_t i = 0; i < rows.size(); ++i)
        {
            const RowRequest& request = requests_[i];
            rows[i] = ControlGuide::RowRequest{ request.isShown, request.isUsable, GlyphSprite(request.glyph), LabelText(request.label) };
        }
        controlGuide_->Present(acceptance != PlayerAvatarControlAcceptance::None, rows, std::nullopt, false);
    }

    std::shared_ptr<Asset::SpriteFile> MagicCasterControlGuide::GlyphSprite(const Glyph glyph) const
    {
        const bool isPad = device_ == PlayerAvatarInputDevice::Gamepad;
        switch (glyph)
        {
        case Glyph::Move:     return (isPad ? padMoveSprite_     : keyMoveSprite_    ).get();
        case Glyph::Cast:     return (isPad ? padCastSprite_     : keyCastSprite_    ).get();
        case Glyph::Run:      return (isPad ? padRunSprite_      : keyRunSprite_     ).get();
        case Glyph::Jump:     return (isPad ? padJumpSprite_     : keyJumpSprite_    ).get();
        case Glyph::LockOn:   return (isPad ? padLockOnSprite_   : keyLockOnSprite_  ).get();
        case Glyph::Interact: return (isPad ? padInteractSprite_ : keyInteractSprite_).get();
        }
        return nullptr;
    }

    const std::string& MagicCasterControlGuide::LabelText(const Label label) const
    {
        switch (label)
        {
        case Label::Move:          return moveLabel_;
        case Label::Cast:          return castLabel_;
        case Label::Run:           return runLabel_;
        case Label::Jump:          return jumpLabel_;
        case Label::LockOn:        return lockOnLabel_;
        case Label::LockOnRelease: return lockOnReleaseLabel_;
        case Label::Chat:          return chatLabel_;
        }
        return moveLabel_;
    }

    void MagicCasterControlGuide::OnDrawGui()
    {
        ImGui::Text("device_: %s", device_ == PlayerAvatarInputDevice::Gamepad ? "Gamepad" : "KeyboardMouse");

        ImGuiHelper::OnDrawInputField("controlGuide_", controlGuide_);
        ImGuiHelper::OnDrawInputField("keyMoveSprite_", keyMoveSprite_);
        ImGuiHelper::OnDrawInputField("keyCastSprite_", keyCastSprite_);
        ImGuiHelper::OnDrawInputField("keyRunSprite_", keyRunSprite_);
        ImGuiHelper::OnDrawInputField("keyJumpSprite_", keyJumpSprite_);
        ImGuiHelper::OnDrawInputField("keyLockOnSprite_", keyLockOnSprite_);
        ImGuiHelper::OnDrawInputField("keyInteractSprite_", keyInteractSprite_);
        ImGuiHelper::OnDrawInputField("padMoveSprite_", padMoveSprite_);
        ImGuiHelper::OnDrawInputField("padCastSprite_", padCastSprite_);
        ImGuiHelper::OnDrawInputField("padRunSprite_", padRunSprite_);
        ImGuiHelper::OnDrawInputField("padJumpSprite_", padJumpSprite_);
        ImGuiHelper::OnDrawInputField("padLockOnSprite_", padLockOnSprite_);
        ImGuiHelper::OnDrawInputField("padInteractSprite_", padInteractSprite_);

        ImGuiHelper::OnDrawInputField("moveLabel_", moveLabel_);
        ImGuiHelper::OnDrawInputField("castLabel_", castLabel_);
        ImGuiHelper::OnDrawInputField("runLabel_", runLabel_);
        ImGuiHelper::OnDrawInputField("jumpLabel_", jumpLabel_);
        ImGuiHelper::OnDrawInputField("lockOnLabel_", lockOnLabel_);
        ImGuiHelper::OnDrawInputField("lockOnReleaseLabel_", lockOnReleaseLabel_);
        ImGuiHelper::OnDrawInputField("chatLabel_", chatLabel_);
    }
}
