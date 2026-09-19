#pragma once
#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "cereal/types/vector.hpp"
#include "../../../Engine/Module/ScriptableObject/ScriptableObject.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto ANNOUNCEMENT_EXTENSION_LABEL = ".announcement";

    /** @brief お知らせの種類。掲示板の札と角印の絵はこの順で並べる */
    enum class AnnouncementKind : int
    {
        Important = 0,
        Update,
        Bug,
        Event,
        Guide,
    };

    constexpr std::string_view ToString(const AnnouncementKind kind)
    {
        switch (kind)
        {
        case AnnouncementKind::Important: return "Important";
        case AnnouncementKind::Update:    return "Update";
        case AnnouncementKind::Bug:       return "Bug";
        case AnnouncementKind::Event:     return "Event";
        case AnnouncementKind::Guide:     return "Guide";
        }
        return "UnknownAnnouncementKind";
    }

    constexpr std::array ANNOUNCEMENT_KINDS{
        AnnouncementKind::Important,
        AnnouncementKind::Update,
        AnnouncementKind::Bug,
        AnnouncementKind::Event,
        AnnouncementKind::Guide,
    };

    /**
     * @brief 掲示板の「お知らせ」1件。掲載時刻は "YYYY-MM-DD HH:MM"(日本時間) で書き、それより前は出さない。
     */
    class Announcement final : public ScriptableObject
    {
    public:
        explicit Announcement(const std::string& contentPath = "");

        [[nodiscard]] AnnouncementKind                Kind     () const { return kind_;      }
        [[nodiscard]] const std::string&              Title    () const { return title_;     }
        [[nodiscard]] const std::vector<std::string>& BodyLines() const { return bodyLines_; }
        /** @return 書式が崩れていれば nullopt */
        [[nodiscard]] std::optional<std::chrono::sys_seconds> PostedTime() const;

    private:
        [[serialize(0)]] AnnouncementKind         kind_ = AnnouncementKind::Update;
        [[serialize(0)]] std::string              title_;
        [[serialize(0)]] std::string              postedAt_;
        [[serialize(0)]] std::vector<std::string> bodyLines_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(kind_));
            archive(CEREAL_NVP(title_));
            archive(CEREAL_NVP(postedAt_));
            archive(CEREAL_NVP(bodyLines_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(kind_));
            if (version >= 0) archive(CEREAL_NVP(title_));
            if (version >= 0) archive(CEREAL_NVP(postedAt_));
            if (version >= 0) archive(CEREAL_NVP(bodyLines_));
        }
#pragma endregion
    };
}

REGISTER_SCRIPTABLE_OBJECT(Announcement, ANNOUNCEMENT_EXTENSION_LABEL, "EventBoard")
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::Announcement, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::Announcement);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::Announcement);
#pragma endregion
