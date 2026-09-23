#include "GamePlay_ItemUseCue.h"

#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Network/Engine_Network_NetworkRunner.h"
#include "Engine/Module/Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../../Data/Item/Data_ItemData.h"
#include "../../Core/Network/Rpc/Custom_RpcType.h"
#include "../Sound/SoundPlayer.h"
#include "../Spawn/GamePlay_PrefabSpawner.h"

namespace GamePlay::Item
{
    namespace
    {
        /** @return オンラインでなければ Invalid */
        Core::Network::NetworkObjectId NetworkObjectIdOf(GameObject::IGameObject& user)
        {
            if (!Module::Network::NetworkRunnerBase::TryGetInstance())
                return Core::Network::NetworkObjectId::Invalid();

            const auto networkGameObject = user.Components().Catch<Module::Network::NetworkGameObject>().lock();
            return networkGameObject ? networkGameObject->GetNetworkObjectId() : Core::Network::NetworkObjectId::Invalid();
        }
    }

    void PlayItemUseCue(const Asset::ItemData& item, const std::shared_ptr<GameObject::IGameObject>& user)
    {
        if (!user)
            return;

        const glm::vec3 position = user->Transform().GetWorldPos();
        const auto userId = NetworkObjectIdOf(*user);
        const bool isOnline = userId != Core::Network::NetworkObjectId::Invalid();

        if (const auto sound = item.UseSound())
        {
            Sound::SoundPlayer::PlaySe(*sound, position);
            if (isOnline)
                GameCore::Network::PlaySeRpc::Send(userId, Core::Network::DeliveryMode::Reliable, sound->GetGuid(), position);
        }

        if (const auto particle = item.UseParticle())
        {
            Spawn::SpawnFollowingPrefab(*particle, user);
            if (isOnline)
                GameCore::Network::SpawnFollowingPrefabRpc::Send(userId, Core::Network::DeliveryMode::Reliable, particle->GetGuid());
        }
    }
}
