#include "Engine_Module_SharedStaticObject.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace
{
    struct Slot
    {
        void*                            object;
        void                           (*destroy)(void*);
        NanamiEngine::Core::ModuleHandle owner; // 実体を作ったモジュール。create 関数のアドレスから求める
    };

    struct Table
    {
        std::mutex                             mutex;
        std::unordered_map<std::string, Slot>  slots;
    };

    // NOTE: 静的初期化の途中 (DLL の DllMain 中) から呼ばれるので、関数ローカル static で遅延生成する
    Table& GetTable()
    {
        static Table table;
        return table;
    }
}

// static_object.hpp のパッチが呼ぶ。エンジン DLL が export する唯一の入口
namespace cereal::detail
{
    void* nanami_shared_static_object(char const* key, void* (*create)(), void (*destroy)(void*))
    {
        Table& table = GetTable();
        {
            std::lock_guard lock(table.mutex);
            if (const auto it = table.slots.find(key); it != table.slots.end())
                return it->second.object;
        }
        // WARNING: create() は別の StaticObject を触って再入してくる (OutputBindingCreator のコンストラクタが
        //          OutputBindingMap を取る等) ので、ロックの外で作る
        const NanamiEngine::Core::ModuleHandle owner = NanamiEngine::Core::ModuleOf(reinterpret_cast<const void*>(create));
        void* const object = create();
        std::lock_guard lock(table.mutex);
        const auto [it, inserted] = table.slots.try_emplace(key, Slot{ object, destroy, owner });
        if (!inserted)
        {
            // 別スレッドが先に作っていた
            destroy(object);
            return it->second.object;
        }
        return object;
    }
}

namespace NanamiEngine::Module::Serialization
{
    std::size_t SharedStaticObjects::ReleaseOwnedBy(const Core::ModuleHandle module)
    {
        Table& table = GetTable();
        std::lock_guard lock(table.mutex);
        std::size_t released = 0;
        for (auto it = table.slots.begin(); it != table.slots.end();)
        {
            if (it->second.owner == module)
            {
                // WARNING: destroy は所有モジュール内のコード。FreeLibrary の前に呼ぶこと
                it->second.destroy(it->second.object);
                it = table.slots.erase(it);
                ++released;
            }
            else
            {
                ++it;
            }
        }
        return released;
    }

    std::size_t SharedStaticObjects::CountOwnedBy(const Core::ModuleHandle module)
    {
        Table& table = GetTable();
        std::lock_guard lock(table.mutex);
        std::size_t count = 0;
        for (const auto& slot : table.slots)
        {
            if (slot.second.owner == module)
                ++count;
        }
        return count;
    }

    std::size_t SharedStaticObjects::Count()
    {
        Table& table = GetTable();
        std::lock_guard lock(table.mutex);
        return table.slots.size();
    }
}
