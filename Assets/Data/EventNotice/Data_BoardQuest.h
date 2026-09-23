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
#include "../Stage/Data_StageData.h"
#include "Data_EventNotice.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto BOARD_QUEST_EXTENSION_LABEL = ".boardQuest";
    constexpr int  BOARD_QUEST_MAX_RANK        = 5;

    /**
     * @brief 掲示板に貼る依頼書1枚。受けたときに始まる中身は quest_ が持ち、ここは貼り紙の文言と場所だけを持つ。
     * event_ を指すと、そのイベントの開催中だけ貼り出す。
     */
    class BoardQuest final : public ScriptableObject
    {
    public:
        explicit BoardQuest(const std::string& contentPath = "");

        [[nodiscard]] const std::string&              Title           () const { return title_;            }
        [[nodiscard]] const std::string&              ClientName      () const { return clientName_;       }
        [[nodiscard]] int                             Rank            () const { return rank_;             }
        [[nodiscard]] const std::string&              GoalText        () const { return goalText_;         }
        [[nodiscard]] const std::vector<std::string>& DescriptionLines() const { return descriptionLines_; }
        /** @brief 場所の名前と写真はステージから借りる。拠点の依頼などでは空 */
        [[nodiscard]] std::shared_ptr<StageData>      Stage           () const { return stage_.get();      }
        [[nodiscard]] std::shared_ptr<EventNotice>    Event           () const { return event_.get();      }
        /** @brief 受注のときはこれを複製して渡す。空なら「準備中」 */
        [[nodiscard]] const std::shared_ptr<GameCore::PlayerAvatar::Quest::ITakeableQuest>& Quest() const { return quest_; }

    private:
        [[serialize(0)]] std::string              title_;
        [[serialize(0)]] std::string              clientName_;
        [[serialize(0)]] int                      rank_ = 1;
        [[serialize(0)]] std::string              goalText_;
        [[serialize(0)]] std::vector<std::string> descriptionLines_;
        [[serialize(0)]] FIELD(StageData)         stage_;
        [[serialize(0)]] FIELD(EventNotice)       event_;
        [[serialize(0)]] std::shared_ptr<GameCore::PlayerAvatar::Quest::ITakeableQuest> quest_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(title_));
            archive(CEREAL_NVP(clientName_));
            archive(CEREAL_NVP(rank_));
            archive(CEREAL_NVP(goalText_));
            archive(CEREAL_NVP(descriptionLines_));
            archive(CEREAL_NVP(stage_));
            archive(CEREAL_NVP(event_));
            archive(CEREAL_NVP(quest_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(title_));
            if (version >= 0) archive(CEREAL_NVP(clientName_));
            if (version >= 0) archive(CEREAL_NVP(rank_));
            if (version >= 0) archive(CEREAL_NVP(goalText_));
            if (version >= 0) archive(CEREAL_NVP(descriptionLines_));
            if (version >= 0) archive(CEREAL_NVP(stage_));
            if (version >= 0) archive(CEREAL_NVP(event_));
            if (version >= 0) archive(CEREAL_NVP(quest_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::BoardQuest, 0);
#pragma endregion
