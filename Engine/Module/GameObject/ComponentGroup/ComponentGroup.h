#pragma once
#include <memory>
#include <vector>

#include "../../../Core/Application/ApplicationBase.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::Component
{
    class ComponentBase;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::GameObject
{
    class ComponentGroup final
    {
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const;
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version);
        
        void InitComponentGroup(const std::weak_ptr<IGameObject>& gameObject);
        [[nodiscard]] std::weak_ptr<IGameObject> Entity() const { return ownerGameObject_; }
        //NOTE: この関数を何かしらの方法でカプセル化した方が安全
        //WARNING: エンジン開発者以外使用しないでください。
        void ResetGuid() const;

        template <class T, typename = std::enable_if_t<std::is_base_of_v<Component::ComponentBase, T>>>
        std::shared_ptr<T> Add();
        template <typename T>
        void OnRemove();
        template <class T>
        std::weak_ptr<T> Catch();
        template <class T>
        std::vector<std::weak_ptr<T>> Catches();
        void OnDrawGui();
        void SetEnable(bool enable) const;
        //Entityが破棄される時に呼ばれる。
        void OnDestroy();

    private:
        void MoveAdd(const std::shared_ptr<Component::ComponentBase>& move);
        
        std::vector<std::shared_ptr<Component::ComponentBase>> components_;
        std::weak_ptr<IGameObject> ownerGameObject_;
    };

    template <class T, typename>
    std::shared_ptr<T> ComponentGroup::Add()
    {
        auto component = std::make_shared<T>();

        Core::Application::ApplicationBase::GetMainWindow()->LifeCycle()
            .DynamicAddCallback<T>(std::weak_ptr<T>(component));

        component->InitComponent(ownerGameObject_);
        components_.push_back(component);

        return component;
    }

    template <typename T>
    void ComponentGroup::OnRemove()
    {
        auto it = std::remove_if(components_.begin(), components_.end(),
                                 [](const std::unique_ptr<Component::ComponentBase>& component)
                                 {
                                     return dynamic_cast<T*>(component.get()) != nullptr;
                                 });

        components_.erase(it, components_.end());
    }

    template <class T>
    std::weak_ptr<T> ComponentGroup::Catch()
    {
        for (const auto& component : components_)
        {
            if (auto castedComponent = std::dynamic_pointer_cast<T>(component); castedComponent)
            {
                return castedComponent;
            }
        }
        return std::weak_ptr<T>{};
    }

    template <class T>
    std::vector<std::weak_ptr<T>> ComponentGroup::Catches()
    {
        std::vector<std::weak_ptr<T>> result;
        for (const auto& component : components_)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(component); casted)
            {
                result.push_back(casted);
            }
        }
        return result;
    }
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::GameObject::ComponentGroup, 0);
