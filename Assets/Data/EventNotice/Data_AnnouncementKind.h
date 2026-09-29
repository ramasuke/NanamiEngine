#pragma once
#include <array>
#include <string_view>

namespace NanamiEngine::Module::Asset
{
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
}
