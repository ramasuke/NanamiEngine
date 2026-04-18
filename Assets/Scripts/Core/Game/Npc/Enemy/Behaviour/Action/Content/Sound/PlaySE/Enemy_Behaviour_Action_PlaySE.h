#pragma once
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    class PlaySE final : public ActionBase
    {
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;


        [[serialize(0)]] FIELD(Asset::SoundFile) sound_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(sound_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(sound_));
        }
#pragma endregion
    };
    REGISTER_ENEMY_ACTION_WITH_NAME(PlaySE, "Sound::PlaySE")
}
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::PlaySE, 1)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::PlaySE)
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::PlaySE)