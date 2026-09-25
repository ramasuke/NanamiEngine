#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine/Core/Api/NanamiModule.h"
#include <memory>
#include <unordered_map>
#include "../../../Module/Guid/Guid.h"

namespace NanamiEngine::Module::Object
{
    class IObject;
}

namespace NanamiEngine::Core::FileSystem
{
    class NANAMI_API ObjectRegistry final
    {
    public:
        void Add(const std::weak_ptr<Module::Object::IObject>& object);
        void Remove(const Guid& guid);
        void Unregister(const Guid& guid, const Module::Object::IObject& object);
        void RemoveIfExpired(const Guid& guid);
        /** @brief 期限切れの weak_ptr を全部捨てる。ゲーム DLL を外す前に呼ぶ (制御ブロックの解放が DLL のコードを呼ぶ) */
        std::size_t PurgeExpired();
        /** @brief 生きていて vtable が module にあるオブジェクトの数 (アンロード前の取り残し確認用) */
        [[nodiscard]] std::size_t CountAliveOfModule(ModuleHandle module) const;

        template <typename T>
        std::weak_ptr<T> Catch(const Guid& guid) const;

    private:
        std::unordered_map<Guid, std::weak_ptr<Module::Object::IObject>, GuidHash> assets_;
    };

    template <typename T>
    std::weak_ptr<T> ObjectRegistry::Catch(const Guid& guid) const
    {
        const auto it = assets_.find(guid);
        if (it == assets_.end())
            return {};

        if (const auto shared = it->second.lock())
        {
            return std::dynamic_pointer_cast<T>(shared);
        }

        return {};
    }
}