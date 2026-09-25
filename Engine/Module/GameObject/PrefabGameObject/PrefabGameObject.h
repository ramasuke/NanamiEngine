#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../ComponentGroup/ComponentGroup.h"
#include "../Interface/IGameObject.h"
#include "../Transform/Transform.h"

namespace NanamiEngine::Scene
{
    class CopiedPrefabGameObject;
}

namespace NanamiEngine::Module::GameObject
{
    class NANAMI_API PrefabGameObject final : public IGameObject
    {
    public:
        explicit PrefabGameObject(const std::string& filePath = "");
        
        void InitGameObject(const std::weak_ptr<IGameObject>& parent, const std::shared_ptr<IGameObject>& ownPtr) override;
        void InitForCopied(const std::shared_ptr<IGameObject>& ownPtr, bool isActive, std::string name, GameObjectMark mark, ComponentGroup components, GameObject::Transform transform) override;
        void InvokeInitAwakeCallbacks() override;
        void InvokeInitStartCallbacks() override;
        void InitPrefab(const std::string& filePath);
        [[nodiscard]] const Guid& GetGuid() const   override { return guid_;         }
        ComponentGroup& Components()                override { return components_;   }
        [[nodiscard]] GameObject::Transform& Transform() override { return transform_;    }

        void ImplementDestroy() override;
        void OnDrawGui()        override;
        void OnDrawTreeGui(bool drawChildren = true) override;
        void OnSave();
        void OnReplaceCopiedObjects();
        void CopiedInit(const std::string& contentPath);

        ///PrefabCopyに使用している関数群
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const;
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version);
        std::shared_ptr<IGameObject> CopyForEditor();
        [[nodiscard]] std::shared_ptr<IGameObject> CopyForInstantiate() override;
        [[nodiscard]] std::shared_ptr<PrefabGameObject> CreateWorkingCopy() const;
        [[nodiscard]] const std::string& Name() const override { return name_; }
        void SetName(std::string name) override { name_ = std::move(name); }
        [[nodiscard]] GameObjectMark Mark() const override { return mark_; }
        void SetMark(const GameObjectMark mark) override { mark_ = mark; }
        bool IsEnable() override { return isActive_; }
        void SetEnable(bool enable) override;

    private:
        bool isActive_ = false;
        std::string filePath_;
        std::string name_ = "Empty";
        GameObjectMark mark_ = GameObjectMark::None;
        Guid guid_;
        ComponentGroup components_;
        GameObject::Transform transform_;
        std::shared_ptr<IGameObject> ownPtr_;
        std::vector<Guid> copiedObjectGuidList_;
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::GameObject::PrefabGameObject, 2);
