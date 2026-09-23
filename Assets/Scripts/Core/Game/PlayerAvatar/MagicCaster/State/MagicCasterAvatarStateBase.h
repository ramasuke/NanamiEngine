#pragma once
#include <functional>
#include <memory>
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
            std::vector<bool> boneAirborne;
        };

        /** ---- 以下サンドボックスパターン ---- */
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject>     CastPoint       () const { return Context().CastPoint(); }
        [[nodiscard]] Magic::IMagicCaster&                       Caster          () const { return Context().Caster(); }
        [[nodiscard]] int                                        PendingSpellSlot() const { return Context().PendingSpellSlot(); }
        [[nodiscard]] std::shared_ptr<const Magic::IMagicSpell>  PendingSpell    () const { return Context().PendingSpell(); }
        /** @brief 枠番号（MagicCasterSpellSlot.h）の魔法。空の枠なら nullptr */
        [[nodiscard]] std::shared_ptr<const Magic::IMagicSpell>  SpellAt(int slot) const;

        /** @brief 足ボーンが接地した瞬間に足元へ土煙を出す。Walk / Run が毎フレーム呼ぶ */
        void TryEmitFootstep(FootstepLatch& latch) const;

        /**
         * @brief 基本魔法（RT / 左クリック）か LT+ボタン（1〜4）の入力を見て、撃てるなら枠を決めて Cast へ移る
         * @return Cast へ移ったら true
         */
        bool TryBeginCast() const;
        /** @brief 基本魔法（RT / 左クリック）を今撃てるか */
        [[nodiscard]] bool CanCastBasicSpell() const;
        /**
         * @brief VisitTransitions の宣言どおりに遷移する。最初に成立した遷移で止まる
         * @return 遷移したか
         */
        bool UpdateTransitions() const;
        /** @brief ロックオン中ならその対象へ向きを合わせる */
        void FaceAimTarget() const;
        // VisitTransitions で CycleItem / UseItem を宣言したStateだけが呼ぶ（アイテム欄の表示がその宣言を見ている）
        /** @return 使うモーションのステートへ移ったら true。そのフレームは呼び出し元の遷移を見ない */
        bool UpdateItemPouchInput() const;
        /** @return 使うモーションのステートへ移ったら true */
        bool UseSelectedPouchItem() const;
    };
}
