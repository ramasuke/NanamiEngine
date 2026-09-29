#pragma once
#include <cstdint>
#include <set>
#include <string>

#include <cereal/cereal.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/string.hpp>

#include "Libs/Singleton/LibCore_SingletonBase.h"
#include "Packages/R4/R4.h"

class Guid;

namespace GameCore::Decoration
{
    constexpr auto DECORATION_SAVE_FILE_PATH = "GameProgression/";
    constexpr auto DECORATION_SAVE_FILE_KEY  = "Decorations";

    struct DecorationSaveData
    {
        /** DecorationData の guid */
        std::set<std::string> owned;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(owned));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(owned));
        }
    };

    /**
     * @brief 持っている島の飾り。StoryProgress と同じく手元の PC にだけ保存し、マルチプレイでは共有しない
     */
    class DecorationCollection final : public SingletonBase<DecorationCollection>
    {
    public:
        DecorationCollection();

        void Reload();

        [[nodiscard]] bool IsOwned(const Guid& decoration) const;
        /** @return 初めて手に入れたなら true */
        bool Add(const Guid& decoration);
        /** @return 持っていて手放したなら true */
        bool Remove(const Guid& decoration);
        [[nodiscard]] const std::set<std::string>& Owned() const { return data_.owned; }

        /** @brief 増えたか減ったときに流れる */
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnChanged() const { return onChanged_.AsObservable(); }

    private:
        void SaveAndNotify();

        DecorationSaveData data_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit> onChanged_;
    };
}

CEREAL_CLASS_VERSION(GameCore::Decoration::DecorationSaveData, 0);
