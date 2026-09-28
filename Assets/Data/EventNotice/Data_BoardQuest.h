#pragma once
#include <memory>
#include <string>
#include <vector>

#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/PlayerAvatar/Quest/PlayerAvatar_ITakeableQuest.h"
#include "../../Scripts/Core/Game/Condition/Condition_ICondition.h"
#include "../Stage/Data_StageData.h"
#include "Data_EventNotice.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto BOARD_QUEST_EXTENSION_LABEL = ".boardQuest";

    class BoardQuest final : public ScriptableObject
    {
    public:
        explicit BoardQuest(const std::string& contentPath = "");

        [[nodiscard]] const std::string&              Title           () const { return title_;            }
        [[nodiscard]] const std::string&              ClientName      () const { return clientName_;       }
        [[nodiscard]] const std::string&              GoalText        () const { return goalText_;         }
        [[nodiscard]] const std::vector<std::string>& DescriptionLines() const { return descriptionLines_; }
        [[nodiscard]] std::shared_ptr<StageData>      Stage           () const { return stage_.get();      }
        [[nodiscard]] std::shared_ptr<EventNotice>    Event           () const { return event_.get();      }
        
        [[nodiscard]] const std::shared_ptr<GameCore::PlayerAvatar::Quest::ITakeableQuest>& Quest() const { return quest_; }
        [[nodiscard]] const GameCore::Condition::Conditions& UnlockConditions() const { return unlockConditions_; }
        [[nodiscard]] const std::string&              LockedText      () const { return lockedText_;       }
        [[nodiscard]] bool IsUnlocked(const GameCore::Condition::ConditionContext& context) const;

    private:
        [[serialize(0)]] std::string              title_;
        [[serialize(0)]] std::string              clientName_;
        [[serialize(0)]] std::string              goalText_;
        [[serialize(0)]] std::vector<std::string> descriptionLines_;
        [[serialize(0)]] FIELD(StageData)         stage_;
        [[serialize(0)]] FIELD(EventNotice)       event_;
        [[serialize(0)]] std::shared_ptr<GameCore::PlayerAvatar::Quest::ITakeableQuest> quest_;
        [[serialize(1)]] GameCore::Condition::Conditions unlockConditions_;
        [[serialize(1)]] std::string              lockedText_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(title_));
            archive(CEREAL_NVP(clientName_));
            archive(CEREAL_NVP(goalText_));
            archive(CEREAL_NVP(descriptionLines_));
            archive(CEREAL_NVP(stage_));
            archive(CEREAL_NVP(event_));
            archive(CEREAL_NVP(quest_));
            archive(CEREAL_NVP(unlockConditions_));
            archive(CEREAL_NVP(lockedText_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(title_));
            if (version >= 0) archive(CEREAL_NVP(clientName_));
            if (version < 2)
            {
                int legacyRank = 0;
                archive(cereal::make_nvp("rank_", legacyRank));
            }
            if (version >= 0) archive(CEREAL_NVP(goalText_));
            if (version >= 0) archive(CEREAL_NVP(descriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(stage_));
            if (version >= 0) archive(CEREAL_NVP(event_));
            if (version >= 0) archive(CEREAL_NVP(quest_));
            if (version >= 1) archive(CEREAL_NVP(unlockConditions_));
            if (version >= 1) archive(CEREAL_NVP(lockedText_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::BoardQuest, 2);
#pragma endregion
