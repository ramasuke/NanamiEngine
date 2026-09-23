#pragma once
#include <cstdint>
#include <set>

#include <cereal/cereal.hpp>
#include <cereal/types/set.hpp>

#include "Story_Facility.h"
#include "Story_StoryFlag.h"
#include "Libs/Singleton/LibCore_SingletonBase.h"
#include "Packages/R4/R4.h"

namespace GameCore::Story
{
    constexpr auto STORY_PROGRESS_SAVE_FILE_PATH = "GameProgression/";
    constexpr auto STORY_PROGRESS_SAVE_FILE_KEY  = "StoryProgress";

    struct StorySaveData
    {
        std::set<StoryFlag> flags;
        std::set<Facility>  restoredFacilities;

        void OnDrawGui();

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(flags));
            archive(CEREAL_NVP(restoredFacilities));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(flags));
            if (version >= 0) archive(CEREAL_NVP(restoredFacilities));
        }
    };

    /**
     * @brief 物語の進み具合と島の復興状況
     */
    class StoryProgress final : public SingletonBase<StoryProgress>
    {
    public:
        StoryProgress();

        void Reload();

        [[nodiscard]] bool IsSet(StoryFlag flag) const;
        /** @return 初めて立てたなら true */
        bool Set(StoryFlag flag);

        [[nodiscard]] bool IsRestored(Facility facility) const;
        /** @return 初めて直したなら true */
        bool Restore(Facility facility);

        /** @brief フラグか施設が変わったときに流れる */
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnChanged() const { return onChanged_.AsObservable(); }

        void OnDrawGui();

    private:
        void SaveAndNotify();

        StorySaveData data_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit> onChanged_;
    };
}

CEREAL_CLASS_VERSION(GameCore::Story::StorySaveData, 0);
