#pragma once
#include <set>
#include <string>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Scene/SceneFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Libs/glm/gtc/quaternion.hpp"
#include "cereal/types/vector.hpp"
#include "../../../../../../Data/HeightGridMap/Data_HeightGridMap.h"
#include "../../../../../../Data/PlayerAvatar/Factory/PlayerAvatarFactory.h"
#include "../../../Navigation/Navigation_INavigationSceneSource.h"
#include "../../../Navigation/Navigation_NavigationTargetEntry.h"
#include "../../../PlayerAvatar/SwordMan/CameraGroup/SwordManAvatarCameraGroup.h"

namespace GameCore::Scene
{
    class SceneContextBase : public Component::ComponentBase,
                             public Navigation::INavigationSceneSource
    {
    public:
        virtual void Init();
        ~SceneContextBase() override = default;
        [[nodiscard]] std::shared_ptr<NanamiEngine::Module::Asset::HeightGridMap> NavigationGrid() const override { return navigationGrid_.get(); }
        [[nodiscard]] std::optional<Navigation::NavigationTargetRef> FindNavigationTarget(std::string_view id) const override;
        [[nodiscard]] bool HasNavigationObjective(std::string_view id) const override { return navigationObjectives_.contains(id); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnNavigationObjectivesChanged() const override { return onNavigationObjectivesChanged_.AsObservable(); }
        void SetNavigationObjective(const std::string& id, bool active);
        [[nodiscard]] std::shared_ptr<Asset::SceneFile> LoadSceneFile() const { return loadSceneFile_.get(); }
        [[nodiscard]] glm::vec3 PlayerSpawnPoint() const;
        // NOTE: スポーン地点のマーカーの向き。プレイヤーはこの -Z を向いて出てくる
        [[nodiscard]] glm::quat PlayerSpawnRotation() const;
        [[nodiscard]] Asset::PlayerAvatarFactory&        PlayerAvatarFactory() const { return *playerAvatarFactory_.get(); }

    private:
        [[serialize(1)]] FIELD(NanamiEngine::Module::Asset     ::SceneFile  ) loadSceneFile_;
        [[serialize(1)]] FIELD(NanamiEngine::Module::GameObject::IGameObject) playerSpawnPoint_;
        [[serialize(3)]] FIELD(Asset::PlayerAvatarFactory)                    playerAvatarFactory_;
        [[serialize(5)]] FIELD(NanamiEngine::Module::Asset::HeightGridMap)    navigationGrid_;
        [[serialize(5)]] std::vector<Navigation::NavigationTargetEntry>       navigationTargets_;

        std::set<std::string, std::less<>>                navigationObjectives_;
        NanamiEngine::R4::Subject<NanamiEngine::R4::Unit> onNavigationObjectivesChanged_;
        
#pragma region Serialization Function
public:
void BasedOnDrawgui() override;

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    archive(CEREAL_NVP(loadSceneFile_));
    archive(CEREAL_NVP(playerSpawnPoint_));
    [[serialize(3)]] FIELD(PlayerAvatar::SwordMan::SwordManAvatarCameraGroup) swordmanCameraGroup_;
    if (version == 3) archive(CEREAL_NVP(swordmanCameraGroup_));
    archive(CEREAL_NVP(playerAvatarFactory_));
    archive(CEREAL_NVP(navigationGrid_));
    archive(CEREAL_NVP(navigationTargets_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
    if (version >= 1) archive(CEREAL_NVP(loadSceneFile_));
    if (version >= 1) archive(CEREAL_NVP(playerSpawnPoint_));
    [[serialize(3)]] FIELD(PlayerAvatar::SwordMan::SwordManAvatarCameraGroup) swordmanCameraGroup_;
    if (version == 3) archive(CEREAL_NVP(swordmanCameraGroup_));
    if (version >= 3) archive(CEREAL_NVP(playerAvatarFactory_));
    if (version >= 5) archive(CEREAL_NVP(navigationGrid_));
    if (version >= 5) archive(CEREAL_NVP(navigationTargets_));
}
#pragma endregion
};
}
CEREAL_CLASS_VERSION(GameCore::Scene::SceneContextBase, 5);
