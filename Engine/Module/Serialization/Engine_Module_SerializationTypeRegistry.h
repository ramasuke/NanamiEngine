#pragma once
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <../cereal/include/cereal/types/polymorphic.hpp>

// cereal の多相登録（NANAMI_REGISTER_TYPE / NANAMI_REGISTER_POLYMORPHIC_RELATION）を、どのモジュールが行ったかと一緒に記録する。
// cereal 自身は登録を自分の static な表に入れるだけで、エンジンからは一覧も出所も分からない。
// ゲームコードを DLL にして差し替える場合（docs/HotReload.md）、外す DLL が登録した型をここから引いて cereal の表から消す。
// マクロは Engine_Module_SerializationRegistration.h にある。
namespace NanamiEngine::Module::Serialization
{
    struct SerializationTypeRecord
    {
        std::type_index type;
        std::type_index base;
        /** cereal に登録した型名（保存ファイルの polymorphic_name）。関係だけの登録では空 */
        std::string     name;
        /** 登録したモジュール（HMODULE）。exe 1 つの構成では全部同じ値 */
        const void*     module;
    };

    class SerializationTypeRegistry final
    {
    public:
        static SerializationTypeRegistry& Instance();

        /** @param moduleAnchor 登録したモジュールの中にあるアドレス。ここからモジュールを割り出す */
        void Register(const std::type_info& type, const std::type_info& base, const char* name, const void* moduleAnchor);

        [[nodiscard]] const std::vector<SerializationTypeRecord>& Records() const { return records_; }

        template <class T, class Base, bool IsType>
        static bool Record();

    private:
        SerializationTypeRegistry() = default;

        std::vector<SerializationTypeRecord> records_;
    };

    template <class T, class Base, bool IsType>
    bool SerializationTypeRegistry::Record()
    {
        // テンプレートなので、登録を書いたモジュールの中に実体化される。この変数のアドレスでモジュールが分かる
        static constexpr char moduleAnchor = 0;

        const char* name = nullptr;
        if constexpr (IsType)
        {
            name = cereal::detail::binding_name<T>::name();
        }
        Instance().Register(typeid(T), typeid(Base), name, &moduleAnchor);
        return true;
    }
}
