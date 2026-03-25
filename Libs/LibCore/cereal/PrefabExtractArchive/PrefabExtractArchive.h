#pragma once
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/details/helpers.hpp>
#include <cereal/types/polymorphic.hpp>

namespace LibCore {

    class PrefabExtractArchive
        : public cereal::OutputArchive<PrefabExtractArchive>
    {
    public:
        using ArchiveType = PrefabExtractArchive;
        static constexpr bool is_saving = true;
        static constexpr bool is_loading = false;

        // ========= コンストラクタ =========
        PrefabExtractArchive()
            : cereal::OutputArchive<PrefabExtractArchive>(this)
        {
        }

        // ========= cereal 必須 API =========
        // OutputArchive を継承する場合、saveBinary は絶対に必要
        void saveBinary(const void* data, size_t size)
        {
            // 今回は抽出専用なので何もしなくて良い
            // （ポリモーフィック情報の読み取りには saveBinary が存在することが重要）
        }

        // ========= cereal から呼ばれる入口 =========
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

        // ========= メイン処理 =========
        template<class T>
        ArchiveType& process(T& value)
        {
            cereal::prologue(*this, value);

            // save() or serialize() を呼ぶ
            // cereal::access::member_serialize(*this, value);

            classify(value);

            cereal::epilogue(*this, value);
            return *this;
        }


        // ========= classify（自由） =========
        template<class T>
        void classify(T&) {}

    };

} // namespace LibCore
