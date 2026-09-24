#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "Engine/Core/Coroutine/Task/Task.h"
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
     */
    class ScatterFloatingStones final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        struct ScatterShot
        {
            float riseHeight;
            float rise_secs;
            float hover_secs;
            float fly_secs;
            float flyDistance; ///< 水平に飛ぶ距離
            float flyRise;     ///< 飛ぶあいだに上がる高さ
        };

        /**
         * @brief stonesRoot の子(地面に埋めた浮遊石)をせり上がらせ、少し浮かせてから、根元から見た向きへそれぞれ飛ばす。
         *        石の子の ParticleSystem (光の尾。PlayMode は Manual にしておく) は飛び立つときに出す。飛び終えた石は隠す
         * @note BT より長生きしうるので static。値は DoTick で写して渡す
         */
        static Coroutine::Task<void> PlayScatterAsync(std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> stonesRoot, ScatterShot shot);

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
