#pragma once
#include <array>
#include <string_view>

namespace GameCore::Story
{
    // NOTE: セーブとシーンの RestorationGate に int で残るので、新しい値は必ず末尾に足す。一覧は docs/Story.md
    enum class Facility : int
    {
        // 船着き場。最初に直す施設(チュートリアル)
        Dock = 0,
        // 雑貨屋の修繕。商店の品揃えが増える
        GeneralStore,
        // 狩人小屋。草原クリア後、狩人の一族が移り住む
        HunterLodge,
        // 畑。狩人小屋の後
        Field,
    };

    constexpr std::string_view ToString(const Facility facility)
    {
        switch (facility)
        {
        case Facility::Dock:         return "Dock";
        case Facility::GeneralStore: return "GeneralStore";
        case Facility::HunterLodge:  return "HunterLodge";
        case Facility::Field:        return "Field";
        }
        return "UnknownFacility";
    }

    constexpr std::array FACILITIES{
        Facility::Dock,
        Facility::GeneralStore,
        Facility::HunterLodge,
        Facility::Field,
    };
}
