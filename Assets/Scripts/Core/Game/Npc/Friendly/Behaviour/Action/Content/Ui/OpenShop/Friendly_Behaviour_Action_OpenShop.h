#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "../../../../../../../../../GamePlay/Prop/MerchantStall/Prop_MerchantStall.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    /**
     * @brief 店の画面を出し、その画面に露店を渡す
     */
    class OpenShop final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) prefab_;
        [[serialize(0)]] FIELD(GamePlay::Prop::MerchantStall) stall_;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(prefab_));
            archive(CEREAL_NVP(stall_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(prefab_));
            if (version >= 0) archive(CEREAL_NVP(stall_));
        }
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(OpenShop, "Ui::OpenShop")
}
