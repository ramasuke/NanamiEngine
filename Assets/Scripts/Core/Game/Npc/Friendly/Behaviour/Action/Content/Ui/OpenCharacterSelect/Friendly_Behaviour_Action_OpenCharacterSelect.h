#pragma once
#include "../../../Friendly_Behaviour_ActionBase.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../../../../Editor/Npc/Friendly/Behaviour/Action/Friendly_Behaviour_ActionFactory.h"
#include "../../../../../../../../../GamePlay/Prop/CharacterPodium/Prop_CharacterPodium.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Friendly::Behaviour::Action
{
    /**
     * @brief キャラ選択の画面を出し、その画面に展示台を渡す
     */
    class OpenCharacterSelect final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) prefab_;
        [[serialize(0)]] FIELD(GamePlay::Prop::CharacterPodium) podium_;

    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(prefab_));
            archive(CEREAL_NVP(podium_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(prefab_));
            if (version >= 0) archive(CEREAL_NVP(podium_));
        }
    };

    REGISTER_FRIENDLY_ACTION_WITH_NAME(OpenCharacterSelect, "Ui::OpenCharacterSelect")
}

CEREAL_REGISTER_TYPE(GameCore::Npc::Friendly::Behaviour::Action::OpenCharacterSelect)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Friendly::Behaviour::ActionBase, GameCore::Npc::Friendly::Behaviour::Action::OpenCharacterSelect)
