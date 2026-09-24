#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/MV1/MV1File.h"
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /**
     * target_ の ModelRenderer のモデルを model_ に差し替える(島破壊で壊れた建物にする等)。
     * 差し替えは model_ の読み込みを待って裏で行うので、Tick はすぐ Success を返す。
     */
    class SwapModel final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] FIELD(GameObject::IGameObject) target_;
        [[serialize(0)]] FIELD(NanamiEngine::Module::Asset::Mv1File) model_;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(target_));
            archive(CEREAL_NVP(model_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(target_));
            if (version >= 0) archive(CEREAL_NVP(model_));
        }
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(SwapModel, "GameObject::SwapModel")
}
