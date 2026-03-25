#pragma once
#include "../ComponentGroup/ComponentGroup.h"
#include "../Interface/IGameObject.h"
#include "../Transform/Transform.h"

namespace NanamiEngine::Scene
{
    class CopiedPrefabGameObject;
}

namespace NanamiEngine::Module::GameObject
{
    class PrefabGameObject final : public IGameObject
    {
    public:
        explicit PrefabGameObject(const std::string& filePath = "");
        
        void InitGameObject(const std::weak_ptr<IGameObject>& parent, const std::shared_ptr<IGameObject>& ownPtr) override;
        void InitForCopied(const std::shared_ptr<IGameObject>& ownPtr, bool isActive, std::string name,
            ComponentGroup components, Transform transform) override;
        void InitPrefab(const std::string& filePath);
        [[nodiscard]] const Guid& GetGuid() const   override { return guid_;         }
        ComponentGroup& Components()                override { return components_;   }
        [[nodiscard]] Transform& TransformRef()     override { return transform_;    }

        void ImplementDestroy() override;
        void OnDrawGui()        override;
        void OnDrawTreeGui()    override;
        void OnSave();
        void OnReplaceCopiedObjects();
        
        ///=== PrefabCopyに使用している関数群 ===
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const;
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version);
        std::shared_ptr<IGameObject> CopyForEditor();
        [[nodiscard]] std::shared_ptr<IGameObject> CopyForInstantiate() override;
        [[nodiscard]] const std::string& Name() const override { return name_; }
        void SetEnable(bool enable) override;

    private:
        bool isActive_ = false;
        std::string filePath_;
        std::string name_ = "Empty";
        Guid guid_;
        ComponentGroup components_;
        Transform transform_;
        std::shared_ptr<IGameObject> ownPtr_;
        std::vector<Guid> copiedObjectGuidList_;
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::GameObject::PrefabGameObject, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::GameObject::PrefabGameObject);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::GameObject::IGameObject, NanamiEngine::Module::GameObject::PrefabGameObject);
