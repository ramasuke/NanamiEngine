#pragma once
#include "../../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../GameObject/Interface/IGameObject.h"
#include "../../../GameObject/Transform/Transform.h"
#include "../cereal/include/cereal/types/memory.hpp"

namespace NanamiEngine::Scene
{
    class SceneGameObject final : public Module::GameObject::IGameObject
    {
    public:
        void InitGameObject(const std::weak_ptr<IGameObject>& parent, const std::shared_ptr<IGameObject>& ownPtr) override;
        void InitForCopied(const std::shared_ptr<IGameObject>& ownPtr, bool isActive, std::string name, Module::GameObject::ComponentGroup components, Module::GameObject::Transform transform) override;
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        [[nodiscard]] const std::string& Name() const override { return name_; }
        Module::GameObject::ComponentGroup& Components() override { return components_; }
        [[nodiscard]] Module::GameObject::Transform& TransformRef() override { return transform_; }
        bool IsEnable() override { return isActive_; }
        void SetEnable(bool enable) override;

        std::shared_ptr<IGameObject> CopyForInstantiate() override;
        void OnDestroy()        const override;
        void ImplementDestroy() override;
        void OnDrawGui()        override;
        void OnDrawTreeGui()    override;

    private:
        bool isActive_ = false;
        std::string name_ = "Empty";
        Guid guid_;
        Module::GameObject::ComponentGroup components_;
        Module::GameObject::Transform transform_;
        std::shared_ptr<IGameObject> ownPtr_;

    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IGameObject>(this));
            archive(CEREAL_NVP(isActive_));
            archive(CEREAL_NVP(name_));
            archive(CEREAL_NVP(guid_));
            archive(CEREAL_NVP(components_));
            archive(CEREAL_NVP(transform_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IGameObject>(this));
            archive(CEREAL_NVP(isActive_));
            archive(CEREAL_NVP(name_));
            archive(CEREAL_NVP(guid_));
            archive(CEREAL_NVP(components_));
            archive(CEREAL_NVP(transform_));
        }
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Scene::SceneGameObject, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Scene::SceneGameObject);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::GameObject::IGameObject, NanamiEngine::Scene::SceneGameObject);