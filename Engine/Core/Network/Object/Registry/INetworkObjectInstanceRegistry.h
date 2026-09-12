#pragma once
#include <memory>
#include <vector>
#include "../../ObjectId/Engine_Network_NetworkObjectId.h"
#include "../../PlayerId/PlayerId.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Core::Network
{
    /** 所有者が離脱したときにそのオブジェクトをどう扱うか(RegisterWithId 時に決める) */
    enum class OwnerLeavePolicy : uint8_t
    {
        Transfer = 0, // 所有権をホストへ移す(敵・ワールドオブジェクト)
        Destroy  = 1, // 破棄する(プレイヤーアバター)
    };

    struct OwnedEntry final
    {
        NetworkObjectId  id;
        OwnerLeavePolicy policy;
    };

    /** 「Spawn したピア ≠ 現在の所有者」になっている ID(後入りへ同期する) */
    struct OwnerOverride final
    {
        NetworkObjectId id;
        PlayerId        owner;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(id, owner);
        }
    };

    /**
     * ネットワーク上のオブジェクトインスタンスと、その現在の所有者を管理する。
     * NetworkObjectId の上位バイトは「Spawn したピア」でしかないので、所有者の判定は必ず OwnerOf() を使うこと。
     */
    class INetworkObjectInstanceRegistry
    {
    public:
        virtual ~INetworkObjectInstanceRegistry() = default;

        /** 登録。所有者が未設定なら id.SpawnerId() を初期所有者にする(SetOwner が先に呼ばれていればそれを保持する) */
        virtual void RegisterWithId(
            NetworkObjectId id,
            const std::weak_ptr<Module::GameObject::IGameObject>& object,
            OwnerLeavePolicy policy) = 0;

        virtual void Unregister(NetworkObjectId id) = 0;
        // 1つの GameObject が複数エントリを持つ場合も全て解除する
        virtual void UnregisterObject(const std::shared_ptr<Module::GameObject::IGameObject>& object) = 0;

        [[nodiscard]] virtual std::weak_ptr<Module::GameObject::IGameObject>
            Find(NetworkObjectId id) const = 0;

        /** 現在の所有者。未登録なら PlayerId::Invalid() */
        [[nodiscard]] virtual PlayerId OwnerOf(NetworkObjectId id) const = 0;
        /** 所有者を上書きする。未登録の ID でも所有者だけのエントリを作る(Spawn が後から届いてもよい) */
        virtual void SetOwner(NetworkObjectId id, PlayerId owner) = 0;

        [[nodiscard]] virtual std::vector<OwnedEntry> CollectOwnedBy(PlayerId owner) const = 0;
        /** 生存インスタンスのうち owner != id.SpawnerId() のもの */
        [[nodiscard]] virtual std::vector<OwnerOverride> CollectOwnerOverrides() const = 0;
    };
}
