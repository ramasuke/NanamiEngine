#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

namespace LibCore {

    class NANAMI_API PrefabExtractArchive
        : public cereal::OutputArchive<PrefabExtractArchive>
    {
    public:
        using ArchiveType = PrefabExtractArchive;
        static constexpr bool is_saving = true;
        static constexpr bool is_loading = false;

        PrefabExtractArchive()
            : cereal::OutputArchive<PrefabExtractArchive>(this)
        {
        }

        // NOTE: OutputArchive の派生に必須。抽出専用なので何も書かないが、多相情報を辿るには存在すること自体が要る
        void saveBinary(const void* data, size_t size)
        {
        }

        template<class T>
        ArchiveType& operator&(T&& value)
        {
            return process(value);
        }

        template<class T>
        ArchiveType& operator()(T&& value)
        {
            return process(value);
        }

        template<class T>
        ArchiveType& process(T& value)
        {
            cereal::prologue(*this, value);

            classify(value);

            cereal::epilogue(*this, value);
            return *this;
        }

        template<class T>
        void classify(T&) {}

    };

} // namespace LibCore
