#pragma once
#include <string>
#include <vector>

#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** @brief 重み付きで選んだ値を 1 つブラックボードに書いて Success（RandomSelector[WriteBlackBoard...] の代わり） */
    class RandomWriteBlackBoard final : public ActionBase
    {
    public:
        struct Choice
        {
            int value_  = 0;
            int weight_ = 100;

            void OnDrawGui();

            template<class Archive>
            void serialize(Archive& archive)
            {
                archive(CEREAL_NVP(value_));
                archive(CEREAL_NVP(weight_));
            }
        };

    private:
        TickStatus DoTick(const TickContext& context) override;
        void DoDrawGui() override;

        [[serialize(0)]] std::string keyName_;
        [[serialize(0)]] std::vector<Choice> choices_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(keyName_));
            archive(CEREAL_NVP(choices_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(keyName_));
            if (version >= 0) archive(CEREAL_NVP(choices_));
        }
#pragma endregion
    };

    REGISTER_ENEMY_ACTION_WITH_NAME(RandomWriteBlackBoard, "Other::RandomWriteBlackBoard<Int>")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::RandomWriteBlackBoard, 0)
