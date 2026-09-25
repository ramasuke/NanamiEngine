#pragma once
#include <mutex>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "../../../Libs/cereal/include/cereal/types/polymorphic.hpp"

// cereal の多相登録 (NANAMI_REGISTER_TYPE / NANAMI_REGISTER_POLYMORPHIC_RELATION) の記録。
// cereal 自身は「どの型を、どのモジュール (exe / dll) が登録したか」を覚えないので、ここに残しておく。
// ゲーム DLL をアンロードするときは、この記録からそのモジュールの分を引いて cereal の表から消す (docs/HotReload.md §3.2)。
namespace NanamiEngine::Module::Serialization
{
    struct SerializationTypeRecord
    {
        /** 登録した型 */
        std::type_index type;
        /** その基底 (CEREAL_REGISTER_POLYMORPHIC_RELATION の Base) */
        std::type_index base;
        /** 保存ファイルに入る polymorphic_name (cereal::detail::binding_name<T>::name())。関係だけの登録では空 */
        std::string name;
        /** 登録元のモジュール (HMODULE)。ヘッダに Windows.h を出さないため void* で持つ */
        void* module;
    };

    class SerializationTypeRegistry final
    {
    public:
        // WARNING: SingletonBase を使うと DLL ごとに実体ができるので、.cpp に定義した関数ローカル static を返す
        static SerializationTypeRegistry& Instance();

        /** @param addressInModule 登録元モジュールに置かれた変数のアドレス。そこからモジュールを求める */
        void Record(std::type_index type, std::type_index base, std::string name, const void* addressInModule);

        [[nodiscard]] std::vector<SerializationTypeRecord> Records() const;
        [[nodiscard]] std::vector<SerializationTypeRecord> RecordsOfModule(const void* module) const;
        /** @brief そのモジュールの記録を消す (cereal の表からの削除は呼び出し側が行う) */
        void RemoveModule(const void* module);

        /** @brief アドレスが属するモジュール (HMODULE)。見つからなければ nullptr */
        [[nodiscard]] static void* ModuleOf(const void* address);

    private:
        SerializationTypeRegistry() = default;

        mutable std::mutex                   mutex_;
        std::vector<SerializationTypeRecord> records_;
    };

    namespace Detail
    {
        /** NANAMI_REGISTER_* マクロから呼ばれる。呼び出し元のモジュールでこの関数が実体化されるので、
         *  中の static のアドレスから登録元モジュールが分かる */
        template <typename T, typename Base, bool IsType>
        bool RecordPolymorphicRegistration()
        {
            static const int moduleAnchor = 0;
            const char* name = "";
            if constexpr (IsType)
                name = ::cereal::detail::binding_name<T>::name();
            SerializationTypeRegistry::Instance().Record(typeid(T), typeid(Base), name, &moduleAnchor);
            return true;
        }
    }
}
