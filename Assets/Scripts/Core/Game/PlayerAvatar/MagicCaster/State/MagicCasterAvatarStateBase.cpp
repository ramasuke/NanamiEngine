#include "MagicCasterAvatarStateBase.h"

#include <utility>

#include "Engine/Module/Component/BoneSync/BoneSync.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../Data/PlayerAvatar/Resource/Data_MagicCasterAvatarResource.h"
#include "../../Input/PlayerAvatarInput_void.h"
#include "../../LockOnTarget/ILockOnTarget.h"
#include "../../../../../GamePlay/Item/GamePlay_ItemUseCue.h"
#include "../../../../../GamePlay/Sound/SoundPlayer.h"
#include "../Spell/MagicCasterSpellSlot.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    MagicCasterAvatarStateBase::MagicCasterAvatarStateBase(const MagicCasterAvatarStateArgs& args)
        : PlayerAvatarStateBase(args)
    {
    }

    void MagicCasterAvatarStateBase::TryEmitFootstep(FootstepLatch& latch) const
    {
        if (!Resources().HasFootstepParticlePrefab())
            return;

        const auto boneSync = Player().Components().Catch<Component::BoneSync>().lock();
        if (!boneSync)
            return;

        const auto& boneNames = Resources().FootstepBoneNames();
        if (latch.bones.size() != boneNames.size())
            latch.bones.assign(boneNames.size(), {});

        const glm::vec3 featStepPos   = Context().PlayerAvatarFeatStepPos();
        const float     contactHeight = Resources().FootstepContactHeight();

        for (size_t boneIndex = 0; boneIndex < boneNames.size(); ++boneIndex)
        {
            const auto bonePose = boneSync->GetBoneWorldPose(boneSync->FindBoneIndex(boneNames[boneIndex]));
            if (!bonePose)
                continue;

            auto& bone = latch.bones[boneIndex];
            const float height = bonePose->Position().y - featStepPos.y;
            const std::optional<float> prevHeight = std::exchange(bone.prevHeight, height);
            if (height > contactHeight)
            {
                bone.armed = true;
                continue;
            }
            // NOTE: 閾値を跨いだ瞬間ではなく、浮いてから降りてきて下降が止まったフレーム(最下点)で出す
            if (!bone.armed || !prevHeight || height < *prevHeight)
                continue;

            bone.armed = false;

            const glm::vec3 stepPos(bonePose->Position().x, featStepPos.y, bonePose->Position().z);
            Scene::GameObject::Instantiate(Resources().FootstepParticlePrefab(), stepPos);
        }
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
                case MagicCasterAvatarInput::AvoidRolling: return IsInputInPhase(input_.AvoidRolling(), phase);
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
        glm::vec3 toAim = ILockOnTarget::PositionOf(*target) - Transform().GetWorldPos();
        toAim.y = 0.0f;
        Actions().RotateTowards(toAim, Status().GetAimRotateSpeed());
    }

    bool MagicCasterAvatarStateBase::UpdateItemPouchInput() const
    {
        auto& pouch = Status().Pouch();

        if (Input().CycleItemNext().IsPressed())
            pouch.Cycle(1);
        if (Input().CycleItemPrev().IsPressed())
            pouch.Cycle(-1);
        return Input().UseItem().IsPressed() && UseSelectedPouchItem();
    }

    bool MagicCasterAvatarStateBase::UseSelectedPouchItem() const
    {
        auto& pouch = Status().Pouch();
        const auto item = pouch.SelectedUsableItem();
        if (!item)
            return false;

        switch (item->UseMotion())
        {
        case Item::ItemUseMotion::Drink: pouch.SetPendingUse(item); OnChangeState(MagicCasterAvatarStateType::UseItemDrink); return true;
        case Item::ItemUseMotion::Eat:   pouch.SetPendingUse(item); OnChangeState(MagicCasterAvatarStateType::UseItemEat);   return true;
        case Item::ItemUseMotion::Place: pouch.SetPendingUse(item); OnChangeState(MagicCasterAvatarStateType::UseItemPlace); return true;
        case Item::ItemUseMotion::Instant:
            break;
        }

        const auto user = Context().PlayerAvatarObject();
        if (const auto used = pouch.UseSelected(Status(), user))
            GamePlay::Item::PlayItemUseCue(*used, user);
        return false;
    }
}
