#pragma once
#include <array>
#include <string_view>

namespace GameCore::Story
{
    // NOTE: セーブとシーンの RestorationGate に int で残るので、新しい値は必ず末尾に足す。一覧は docs/Story.md
    enum class Facility : int
    {
        // WARNING: Dock..Field は仮置きで、施設データ・建つ場所はもう無い(2026-09-24 に削除)。番号は再利用しない
        Dock = 0,
        GeneralStore,
        HunterLodge,
        Field,
        // 一族の家。噴水の島に草原の狩人の一族が住み、仲間(キャラ選択)を出してくれる
        ClanHouse,
    };

    constexpr std::string_view ToString(const Facility facility)
    {
        switch (facility)
        {
        case Facility::Dock:         return "Dock";
        case Facility::GeneralStore: return "GeneralStore";
        case Facility::HunterLodge:  return "HunterLodge";
        case Facility::Field:        return "Field";
        case Facility::ClanHouse:    return "ClanHouse";
        }
        return "UnknownFacility";
    }

    // NOTE: エディタで選べる施設。削除済みの番号は出さない
    constexpr std::array FACILITIES{
        Facility::ClanHouse,
    };
}
