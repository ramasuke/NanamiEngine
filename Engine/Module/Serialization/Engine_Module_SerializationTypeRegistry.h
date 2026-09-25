#pragma once
#include <mutex>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "../../../Libs/cereal/include/cereal/types/polymorphic.hpp"
#include "../../Core/Api/NanamiApi.h"
#include "../../Core/Api/NanamiModule.h"

namespace NanamiEngine::Module::Serialization
{
    struct NANAMI_API SerializationTypeRecord
    {
        /** 登録した型 */
        std::type_index type;
        /** その基底 (CEREAL_REGISTER_POLYMORPHIC_RELATION の Base) */
        std::type_index base;
        /** 保存ファイルに入る polymorphic_name (cereal::detail::binding_name<T>::name())。関係だけの登録では空 */
        std::string name;
        /** 登録元のモジュール */
        Core::ModuleHandle module;
    };

    //Serializeの多層登録を保存するRegistry
    class NANAMI_API SerializationTypeRegistry final
    {
    public:
        // WARNING: SingletonBase を使うと DLL ごとに実体ができるので、.cpp に定義した関数ローカル static を返す
        static SerializationTypeRegistry& Instance();

        /** @param addressInModule 登録元モジュールに置かれた変数のアドレス。そこからモジュールを求める */
        void Record(std::type_index type, std::type_index base, std::string name, const void* addressInModule);

        [[nodiscard]] std::vector<SerializationTypeRecord> Records() const;
        [[nodiscard]] std::vector<SerializationTypeRecord> RecordsOfModule(Core::ModuleHandle module) const;
        /** @brief そのモジュールの記録を消す */
        void RemoveModule(Core::ModuleHandle module);


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
