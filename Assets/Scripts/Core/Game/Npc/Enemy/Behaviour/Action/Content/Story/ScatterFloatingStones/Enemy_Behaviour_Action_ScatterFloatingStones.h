#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /**
     * @brief 序章で島の心臓が砕けたとき、地面に埋めた3つの浮遊石 (stonesRoot_ の子) をせり上がらせて三方へ飛ばす。
     *        動かすのはコルーチンなので、始めたらすぐ Success を返す。OnceExecute で包むこと
     * @note riseHeight_ を負にすると下へ抜ける。落ちる島の底から石が抜かれる所にも使う
     */
    class ScatterFloatingStones final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        [[serialize(0)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) stonesRoot_;
        [[serialize(0)]] float riseHeight_ = 130.0f;
        [[serialize(0)]] float riseSeconds_ = 1.6f;
        [[serialize(0)]] float hoverSeconds_ = 0.9f;
        [[serialize(0)]] float flySeconds_ = 3.2f;
        [[serialize(0)]] float flyDistance_ = 2600.0f;
        [[serialize(0)]] float flyRise_ = 700.0f;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(stonesRoot_));
            archive(CEREAL_NVP(riseHeight_));
            archive(CEREAL_NVP(riseSeconds_));
            archive(CEREAL_NVP(hoverSeconds_));
            archive(CEREAL_NVP(flySeconds_));
            archive(CEREAL_NVP(flyDistance_));
            archive(CEREAL_NVP(flyRise_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(stonesRoot_));
            if (version >= 0) archive(CEREAL_NVP(riseHeight_));
            if (version >= 0) archive(CEREAL_NVP(riseSeconds_));
            if (version >= 0) archive(CEREAL_NVP(hoverSeconds_));
            if (version >= 0) archive(CEREAL_NVP(flySeconds_));
            if (version >= 0) archive(CEREAL_NVP(flyDistance_));
            if (version >= 0) archive(CEREAL_NVP(flyRise_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ScatterFloatingStones, "Story::ScatterFloatingStones")
}
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ScatterFloatingStones, 0)
