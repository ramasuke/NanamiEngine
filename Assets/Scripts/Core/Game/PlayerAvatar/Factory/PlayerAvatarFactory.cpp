#include "PlayerAvatarFactory.h"
#include "../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../GamePlay/PlayerAvatar/PlayerAvatarBase.h"
#include "../../../../GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"

namespace GameCore::PlayerAvatar
{
    std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> Factory::SummonSwordManAvatar(
          const std::shared_ptr<Asset::PrefabGameObjectFile>& playerAvatarPrefab
        , const glm::vec3& summonPosition
        , const std::shared_ptr<GameObject::IGameObject>& parent)
    {
        const auto playerAvatarObject = Scene::GameObject::Instantiate(*playerAvatarPrefab, summonPosition).lock();
        auto playerAvatar= playerAvatarObject->Components().Catch<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>().lock();
        
        playerAvatarObject->Transform().SetParent(parent);
        return playerAvatar;
    }
}
