#pragma once
#include <vector>

#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "Data_Announcement.h"
#include "Data_BoardQuest.h"
#include "Data_EventNotice.h"
#include "../Restoration/Data_RestorationFacility.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto EVENT_BOARD_EXTENSION_LABEL = ".eventBoard";

    /**
     * @brief 掲示板に貼るもの(催し・依頼・お知らせ・復興)の一覧。期間外のものは表示側で弾く
     */
    class EventBoardData final : public ScriptableObject
    {
    public:
        explicit EventBoardData(const std::string& contentPath = "");

        [[nodiscard]] std::vector<std::shared_ptr<EventNotice>>  Notices      () const;
        [[nodiscard]] std::vector<std::shared_ptr<BoardQuest>>   Quests       () const;
        [[nodiscard]] std::vector<std::shared_ptr<Announcement>> Announcements() const;
        [[nodiscard]] std::vector<std::shared_ptr<RestorationFacility>> Facilities() const;

    private:
        [[serialize(0)]] std::vector<FIELD(EventNotice)>  notices_;
        [[serialize(1)]] std::vector<FIELD(BoardQuest)>   quests_;
        [[serialize(1)]] std::vector<FIELD(Announcement)> announcements_;
        [[serialize(2)]] std::vector<FIELD(RestorationFacility)> facilities_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(notices_));
            archive(CEREAL_NVP(quests_));
            archive(CEREAL_NVP(announcements_));
            archive(CEREAL_NVP(facilities_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(notices_));
            if (version >= 1) archive(CEREAL_NVP(quests_));
            if (version >= 1) archive(CEREAL_NVP(announcements_));
            if (version >= 2) archive(CEREAL_NVP(facilities_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::EventBoardData, 2);
#pragma endregion
