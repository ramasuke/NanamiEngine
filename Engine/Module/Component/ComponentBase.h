#pragma once
#include <memory>

#include "../LifeCycleCallback/Render/IRenderable.h"
#include "../cereal/include/cereal/types/polymorphic.hpp"
#include "../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../Core/Object/IObject.h"
#include "../GameObject/ComponentGroup/ComponentGroup.h"
#include "../GameObject/Interface/IGameObject.h"
#include "../Namespace/EngineNamespace.h"

namespace NanamiEngine::Core::Object
{
    class IInitializablePrefabObjectField;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Component
{
    class ComponentBase : public Object::IObject
    {
    public:
        ComponentBase();
        virtual ~ComponentBase() override;
        void InitComponent(const std::weak_ptr<GameObject::IGameObject>& ownerGameObject);

        [[nodiscard]] GameObject::ComponentGroup& Components() const { return Entity().lock()->Components(); }
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> Entity() const { return gameObjectRef_; }
        [[nodiscard]] GameObject::Transform& Transform() const { return gameObjectRef_.lock()->TransformRef(); }
        //TODO: 現在は行っていないがRemoveComponentでもOnDestroy()を呼ぶようにする必要がある。
        //NOTE: Componentが破棄されるタイミングで呼ばれる関数
        virtual void OnDestroy() { }
        virtual void BasedOnDrawgui() { }
        virtual void OnDrawGui() override;
        //NOTE: この関数を何かしらの方法でカプセル化した方が安全
        //WARNING: エンジン開発者以外使用しないでください。 
        void ResetGuid();
        void SetEnable(bool enable);
        [[nodiscard]] bool IsEnable() const;

    private:
        Guid guid_;
        bool isEnable_ = true;
        std::weak_ptr<GameObject::IGameObject> gameObjectRef_;
        std::vector<std::shared_ptr<Core::Object::IInitializablePrefabObjectField>> prefabObjectFields_;
        
    protected:
        template <typename T>
        requires std::is_base_of_v<ComponentBase, T>
        std::shared_ptr<T> RequireComponent();
        // template<typename T>
        // const std::shared_ptr<Core::Object::PrefabObjectField<T>>& CreatePrefabField();
        
#pragma region Serialization Function
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(guid_    ));
            archive(CEREAL_NVP(isEnable_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(guid_    ));
            archive(CEREAL_NVP(isEnable_));
        }
#pragma endregion
    };

    template <typename T> requires std::is_base_of_v<ComponentBase, T>
    std::shared_ptr<T> ComponentBase::RequireComponent()
    {
        auto component = Components().Catch<T>();
        if (component.expired())
            return Components().Add<T>();
        
        return component.lock();
    }

    // template <typename T>
    // const std::shared_ptr<Core::Object::PrefabObjectField<T>>& ComponentBase::CreatePrefabField()
    // {
    //     const auto field = std::make_shared<Core::Object::PrefabObjectField<T>>();
    //     prefabObjectFields_.push_back(field);
    //     return field;
    // }
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::ComponentBase, 0);