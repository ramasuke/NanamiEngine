#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <vector>
#include "../../ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../PlayerId/PlayerId.h"
#include "OwnerLeavePolicy.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Core::Network
{
    struct NANAMI_API OwnedEntry final
    {
        NetworkObjectId  id;
        OwnerLeavePolicy policy;
    };
    
    struct NANAMI_API ObjectOwner final
    {
        NetworkObjectId id;
        PlayerId        owner;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(id, owner);
        }
    };

    // NOTE: ネットワーク上のオブジェクトインスタンスの現在の所有者を管理する
    class NANAMI_API INetworkObjectInstanceRegistry
    {
    public:
        virtual ~INetworkObjectInstanceRegistry() = default;

        // NOTE: owner は初期所有者
        // NOTE: 所有者が設定済みなら上書きしない (後入りピアには所有権の通知が spawn 履歴より先に届く)
        virtual void RegisterWithId(
            NetworkObjectId id,
            const std::weak_ptr<Module::GameObject::IGameObject>& object,
            OwnerLeavePolicy policy,
            PlayerId owner) = 0;

        virtual void Unregister(NetworkObjectId id) = 0;
        // NOTE: 1つの GameObject が複数エントリを持つ場合も全て解除する
        virtual void UnregisterObject(const std::shared_ptr<Module::GameObject::IGameObject>& object) = 0;

        [[nodiscard]] virtual std::weak_ptr<Module::GameObject::IGameObject>
            Find(NetworkObjectId id) const = 0;

        // NOTE: 現在の所有者。未登録なら PlayerId::Invalid()
        [[nodiscard]] virtual PlayerId OwnerOf(NetworkObjectId id) const = 0;
        // NOTE: 所有者を上書きする。未登録の ID でも所有者だけのエントリを作る
        virtual void SetOwner(NetworkObjectId id, PlayerId owner) = 0;

        [[nodiscard]] virtual std::vector<OwnedEntry> CollectOwnedBy(PlayerId owner) const = 0;
        // NOTE: 生存インスタンスの id と所有者の一覧
        [[nodiscard]] virtual std::vector<ObjectOwner> CollectOwners() const = 0;
    };
}
