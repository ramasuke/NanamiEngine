#pragma once
#include <vector>

#include "cereal/types/vector.hpp"
#include "../../../Engine/Core/Object/Field/Field.h"
#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "Data_Announcement.h"
#include "Data_BoardQuest.h"
#include "Data_EventNotice.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto EVENT_BOARD_EXTENSION_LABEL = ".eventBoard";

    /**
     * @brief 掲示板に貼るものの一覧。催し(.eventNotice)・依頼(.boardQuest)・お知らせ(.announcement)を
     * このアセットに足すだけで貼り出せ、シーンもプレハブも触らない。
     * 終わった催しや期間外の依頼は表示側で弾くので、ここから外さなくてもよい。
     */
    class EventBoardData final : public ScriptableObject
    {
    public:
        explicit EventBoardData(const std::string& contentPath = "");

        [[nodiscard]] std::vector<std::shared_ptr<EventNotice>>  Notices      () const;
        [[nodiscard]] std::vector<std::shared_ptr<BoardQuest>>   Quests       () const;
        [[nodiscard]] std::vector<std::shared_ptr<Announcement>> Announcements() const;

    private:
        [[serialize(0)]] std::vector<FIELD(EventNotice)>  notices_;
        [[serialize(1)]] std::vector<FIELD(BoardQuest)>   quests_;
        [[serialize(1)]] std::vector<FIELD(Announcement)> announcements_;

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
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(notices_));
            if (version >= 1) archive(CEREAL_NVP(quests_));
            if (version >= 1) archive(CEREAL_NVP(announcements_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(EventBoardData, EVENT_BOARD_EXTENSION_LABEL, "EventBoard")
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::EventBoardData, 1);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::EventBoardData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::EventBoardData);
#pragma endregion
