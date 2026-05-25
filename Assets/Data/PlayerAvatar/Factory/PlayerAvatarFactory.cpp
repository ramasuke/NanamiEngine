#include "PlayerAvatarFactory.h"

#include "../../../Scripts/Core/Game/PlayerAvatar/PlayerAvatar.h"
#include "../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/CameraGroup/AllPlayerCameraGroup.h"
#include "../../../Scripts/GamePlay/PlayerAvatar/PlayerAvatarBase.h"
#include "../../../Scripts/GamePlay/PlayerAvatar/SwordMan/SwordManAvatar.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/Status/PlayerAvatarStatus.h"

namespace NanamiEngine::Module::Asset
{
    PlayerAvatarFactory::PlayerAvatarFactory(const std::string& contentPath)
        : ScriptableObject(contentPath)
    {
        
    }

    std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> PlayerAvatarFactory::SummonSwordManAvatar(
        const glm::vec3& summonPosition,
        const std::shared_ptr<GameObject::IGameObject>& parent)
    {
        const auto playerAvatarObject = Scene::GameObject::Instantiate(swordManPrefab_.get(), summonPosition).lock();
        auto playerAvatar= playerAvatarObject->Components().Catch<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>().lock();
        
        playerAvatarObject->Transform().SetParent(parent);
        return playerAvatar;
    }

    std::shared_ptr<GameCore::IPlayerAvatar> PlayerAvatarFactory::LoadInitedPlayerAvatar(
        const GameCore::PlayerAvatar::PlayerAvatarType& type,
        const glm::vec3& summonPosition,
        const std::shared_ptr<GameObject::IGameObject>& parent,
        const GameCore::PlayerAvatar::AllPlayerCameraGroup& allCameraGroup)
    {
        std::shared_ptr<GameCore::IPlayerAvatar> playerAvatar;

        switch (type)
        {
        case GameCore::PlayerAvatar::PlayerAvatarType::SwordMan:
            {
                playerAvatar = LoadInitedPlayerAvatarImpl<
                    GamePlay::PlayerAvatar::SwordMan::SwordManAvatar,
                    GameCore::PlayerAvatar::SwordMan::SwordManAvatarTraits>(
                    swordManPrefab_.get(),
                    summonPosition,
                    parent,
                    allCameraGroup.Swordman());
                break;
            }

        case GameCore::PlayerAvatar::PlayerAvatarType::Gunner:
            {
                break;
            }

        default:
            {
                break;
            }
        }
        return playerAvatar;
    }

    void PlayerAvatarFactory::OnDrawGui()
    {
        ScriptableObject::OnDrawGui();
        ImGuiHelper::OnDrawInputField("swordManPrefab_", swordManPrefab_);
    }
}
