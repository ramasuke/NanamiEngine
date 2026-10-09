#pragma once
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "MagicCasterAvatarStateType.h"
#include "../../State/PlayerAvatarStateBase.h"
#include "../Animation/MagicCasterAvatarAnimation.h"
#include "../InputAction/MagicCasterAvatarInputAction.h"
#include "../Status/MagicCasterAvatarStatus.h"
#include "Context/MagicCasterAvatarStateContext.h"
#include "Transition/MagicCasterAvatarStateTransition.h"

namespace GameCore::PlayerAvatar::MagicCaster
{
    using MagicCasterAvatarStateArgs = PlayerAvatarStateArgs<MagicCasterAvatarStateContext, MagicCasterAvatarStateType>;

    class MagicCasterAvatarStateBase : public PlayerAvatarStateBase<MagicCasterAvatarStateContext,
                                                                    MagicCasterAvatarStateType,
                                                                    MagicCaster::AnimationType,
                                                                    IMagicCasterAvatarTransitionVisitor>
    {
    public:
        explicit MagicCasterAvatarStateBase(const MagicCasterAvatarStateArgs& args);

        virtual ~MagicCasterAvatarStateBase() override = default;

    protected:
        struct FootstepLatch
        {
            struct Bone
            {
                bool                 armed = false;
                std::optional<float> prevHeight;
            };
            std::vector<Bone> bones;
        };

        // NOTE: 以下サンドボックスパターン
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject>     CastPoint       () const { return Context().CastPoint(); }
        [[nodiscard]] Magic::IMagicCaster&                       Caster          () const { return Context().Caster(); }
        [[nodiscard]] int                                        PendingSpellSlot() const { return Context().PendingSpellSlot(); }
        [[nodiscard]] std::shared_ptr<const Magic::IMagicSpell>  PendingSpell    () const { return Context().PendingSpell(); }
        // NOTE: 枠番号（MagicCasterSpellSlot.h）の魔法。空の枠なら nullptr
        [[nodiscard]] std::shared_ptr<const Magic::IMagicSpell>  SpellAt(int slot) const;

        // NOTE: 足ボーンが接地した瞬間に足元へ土煙を出す。毎フレーム呼ぶ前提
        void TryEmitFootstep(FootstepLatch& latch) const;

        // NOTE: 基本魔法か持ち込み枠の入力を見て、撃てるなら枠を決めて Cast へ移る。移ったら true
        bool TryBeginCast() const;
        // NOTE: カウンター受付中は常に true
        [[nodiscard]] bool CanCastBasicSpell() const;
        [[nodiscard]] bool CanCounterCast() const;
        // NOTE: VisitTransitions の宣言どおりに遷移し、最初に成立した遷移で止まる。遷移したら true
        bool UpdateTransitions() const;
        // NOTE: ロックオン中ならその対象へ向きを合わせる
        void FaceAimTarget() const;
        // NOTE: アイテム欄の入力を見て、使うモーションのステートへ移ったら true。そのフレームは他の遷移を見ない
        bool UpdateItemPouchInput() const;
        // NOTE: 使うモーションのステートへ移ったら true
        bool UseSelectedPouchItem() const;
    };
}
