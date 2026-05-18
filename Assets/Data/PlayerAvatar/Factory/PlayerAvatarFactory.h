#pragma once
#include <memory>

#include "vec3.hpp"
#include "../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/CameraGroup/AllPlayerCameraGroup.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/RequireType/RequireType.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/Status/PlayerAvatarStatus.h"
#include "../../../Scripts/Core/Game/PlayerAvatar/Type/PlayerAvatarType.h"

namespace GameCore::PlayerAvatar::SwordMan
{
    class StatusPresenter;
}

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GameCore::PlayerAvatar
{
    enum class PlayerAvatarType;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Asset
{
    constexpr auto PLAYER_AVATAR_FACTORY_EXTENSION_LABEL = ".playerAvatarFactory";
    
    class PlayerAvatarFactory final : public ScriptableObject
    {
    public:
        explicit PlayerAvatarFactory(const std::string& contentPath = "");
        [[nodiscard]] std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> SummonSwordManAvatar(
              const glm::vec3& summonPosition
            , const std::shared_ptr<GameObject::IGameObject>& parent);
        
        [[nodiscard]] std::shared_ptr<GameCore::IPlayerAvatar> LoadInitedPlayerAvatar(
            const GameCore::PlayerAvatar::PlayerAvatarType& type,
            const glm::vec3& summonPosition,
            const std::shared_ptr<GameObject::IGameObject>& parent,
            GameCore::PlayerAvatar::AllPlayerCameraGroup allCameraGroup);

        template <typename AvatarT, typename TraitsT>
        [[nodiscard]] std::shared_ptr<AvatarT> LoadInitedPlayerAvatarImpl(
              const std::shared_ptr<PrefabGameObjectFile>& prefabFile
            , const glm::vec3& summonPosition
            , const std::shared_ptr<GameObject::IGameObject>& parent
            , std::shared_ptr<GameCore::PlayerAvatar::RequireType::CameraGroup<TraitsT>> cameraGroup);

        
    private:
        [[serialize(0)]] FIELD(PrefabGameObjectFile) swordManPrefab_;
        // [[serialize(0)]] std::weak_ptr<GameCore::PlayerAvatar::SwordMan::StatusPresenter> statusPresenter_; 
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(swordManPrefab_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(swordManPrefab_));
        }
#pragma endregion
    };

    template <typename AvatarT, typename TraitsT>
    std::shared_ptr<AvatarT> PlayerAvatarFactory::LoadInitedPlayerAvatarImpl(
        const std::shared_ptr<PrefabGameObjectFile>& prefabFile,
        const glm::vec3& summonPosition,
        const std::shared_ptr<GameObject::IGameObject>& parent,
        std::shared_ptr<GameCore::PlayerAvatar::RequireType::CameraGroup<TraitsT>> cameraGroup)
    {
        using namespace GameCore::PlayerAvatar;
        using Status = RequireType::Status<TraitsT>;
        using Input  = RequireType::InputAction<TraitsT>;
        
        //Create
        auto playerAvatarObject = Scene::GameObject::Instantiate(prefabFile, summonPosition).lock();
        auto playerAvatar = playerAvatarObject->Components().Catch<AvatarT>().lock();
        
        //Init
        auto inputAction  = std::make_shared<Input>();
        auto status       = GameCore::PlayerAvatar::LoadStatus<Status, TraitsT>();
        auto stateMachine = TraitsT::CreateStateMachine(status, inputAction, playerAvatar, cameraGroup);
        
        playerAvatar->Init(
            status,
            std::move(stateMachine),
            inputAction,
            cameraGroup);

        playerAvatarObject->Transform().SetWorldPos(summonPosition);
        playerAvatarObject->Transform().SetParent(parent);
        return playerAvatar;
    }
}

REGISTER_SCRIPTABLE_OBJECT(PlayerAvatarFactory, PLAYER_AVATAR_FACTORY_EXTENSION_LABEL)
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::PlayerAvatarFactory, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::PlayerAvatarFactory);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::ScriptableObject, NanamiEngine::Module::Asset::PlayerAvatarFactory);
#pragma endregion
