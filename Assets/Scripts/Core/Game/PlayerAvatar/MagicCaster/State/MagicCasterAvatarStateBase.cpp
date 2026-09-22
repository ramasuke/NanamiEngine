#include "MagicCasterAvatarStateBase.h"

#include "../../../../../../Data/PlayerAvatar/Resource/Data_MagicCasterAvatarResource.h"
#include "../../Input/PlayerAvatarInput_void.h"
#include "../../LockOnTarget/ILockOnTarget.h"
#include "../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../Spell/MagicCasterSpellSlot.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    MagicCasterAvatarStateBase::MagicCasterAvatarStateBase(const MagicCasterAvatarStateArgs& args)
        : PlayerAvatarStateBase(args)
    {
    }

    std::shared_ptr<const Magic::IMagicSpell> MagicCasterAvatarStateBase::SpellAt(const int slot) const
    {
        if (slot == SPELL_BASIC_SLOT)
            return Resources().BasicSpell();
        return Resources().LoadoutSpell(slot);
    }

    bool MagicCasterAvatarStateBase::TryBeginCast() const
    {
        std::optional<int> slot;
        if (Input().Cast().IsPressed())
            slot = SPELL_BASIC_SLOT;
        else
            slot = Input().PressedLoadoutSlot();

        if (!slot)
            return false;

        const auto spell = SpellAt(*slot);
        if (!spell || !Status().CanCast(*slot, *spell))
            return false;

        Context().SetPendingCast(*slot, spell);
        OnChangeState(MagicCasterAvatarStateType::Cast);
        return true;
    }

    bool MagicCasterAvatarStateBase::CanCastBasicSpell() const
    {
        const auto spell = SpellAt(SPELL_BASIC_SLOT);
        return spell && Status().CanCast(SPELL_BASIC_SLOT, *spell);
    }

    namespace
    {
        class MagicCasterTransitionExecutor final : public PlayerAvatarTransitionExecutorBase<IMagicCasterAvatarTransitionVisitor>
        {
        public:
            MagicCasterTransitionExecutor(
                const MagicCasterAvatarInputAction& input,
                const std::function<void(MagicCasterAvatarStateType)>& onChangeState,
                const std::function<bool()>& tryBeginCast)
                : PlayerAvatarTransitionExecutorBase(onChangeState)
                , input_(input)
                , tryBeginCast_(tryBeginCast)
            {
            }

            void Cast(bool) override
            {
                if (HasChanged() || !tryBeginCast_())
                    return;

                MarkChanged();
            }

        private:
            [[nodiscard]] bool IsTriggered(const MagicCasterAvatarInput input, const PlayerAvatarInputPhase phase) const override
            {
                switch (input)
                {
                case MagicCasterAvatarInput::Move: return IsInputInPhase(input_.Move(), phase);
                case MagicCasterAvatarInput::Run:  return IsInputInPhase(input_.Run(),  phase);
                case MagicCasterAvatarInput::Jump: return IsInputInPhase(input_.Jump(), phase);
                case MagicCasterAvatarInput::Chat: return IsInputInPhase(input_.Chat(), phase);
                }
                return false;
            }

            const MagicCasterAvatarInputAction& input_;
            const std::function<bool()>& tryBeginCast_;
        };
    }

    bool MagicCasterAvatarStateBase::UpdateTransitions() const
    {
        const std::function<bool()> tryBeginCast = [this] { return TryBeginCast(); };
        MagicCasterTransitionExecutor executor(Input(), OnChangeStateCallback(), tryBeginCast);
        VisitTransitions(executor);
        return executor.HasChanged();
    }

    void MagicCasterAvatarStateBase::FaceAimTarget() const
    {
        const auto target = Caster().AimTarget().lock();
        if (!target)
            return;

        // 部位は真上にあることもあるので、高さを消してから渡す(RotateTowards の長さ判定をすり抜けて水平成分 0 を正規化しないように)
        glm::vec3 toAim = LockOnPositionOf(*target) - Transform().GetWorldPos();
        toAim.y = 0.0f;
        Actions().RotateTowards(toAim, Status().GetMoveRotateSpeed());
    }

    void MagicCasterAvatarStateBase::UpdateItemPouchInput() const
    {
        auto& pouch = Status().Pouch();

        if (Input().CycleItemNext().IsPressed())
            pouch.Cycle(1);
        if (Input().CycleItemPrev().IsPressed())
            pouch.Cycle(-1);
        if (Input().UseItem().IsPressed())
            UseSelectedPouchItem();
    }

    void MagicCasterAvatarStateBase::UseSelectedPouchItem() const
    {
        const auto used = Status().Pouch().UseSelected(Status(), Context().PlayerAvatarObject());
        if (!used)
            return;

        if (const auto sound = used->UseSound())
            GamePlay::Sound::SoundPlayer::PlaySe(*sound, Transform().GetWorldPos());
    }
}
