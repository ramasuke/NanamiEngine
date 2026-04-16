#pragma once
#include "../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../Engine/Module/Asset/Scene/SceneFile.h"
#include "../../../../../../../Engine/Module/Component/ComponentBase.h"

namespace GameCore::Scene::Sub
{
    class SceneContextBase : public Component::ComponentBase
    {
    public:
        virtual ~SceneContextBase() override = default;
        void Initialize();
        [[nodiscard]] const Asset::SceneFile& GetSceneFile() const { return *sceneFile_.get(); }

    private:
        [[serialize(1)]] FIELD(Asset::SceneFile) sceneFile_;
        
    protected:
        virtual void DoInitialize() = 0;
        
#pragma region Serialization Function
    public:
        void BasedOnDrawgui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
            archive(CEREAL_NVP(sceneFile_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<NanamiEngine::Module::Component::ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(sceneFile_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::Sub::SceneContextBase, 1);
CEREAL_REGISTER_TYPE(GameCore::Scene::Sub::SceneContextBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GameCore::Scene::Sub::SceneContextBase);
#pragma endregion