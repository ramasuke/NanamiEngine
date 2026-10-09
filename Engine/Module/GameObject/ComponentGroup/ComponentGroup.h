#pragma once
#include "Engine/Core/Api/NanamiApi.h"
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
    class NANAMI_API ComponentGroup final
    {
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const;
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version);
        
        void InitComponentGroup(const std::weak_ptr<IGameObject>& gameObject);
        [[nodiscard]] std::weak_ptr<IGameObject> Entity() const { return ownerGameObject_; }
        // WARNING: エンジン内部用。全 Component の GUID を振り直す
        void ResetGuid() const;

        template <class T, typename = std::enable_if_t<std::is_base_of_v<Component::ComponentBase, T>>>
        std::shared_ptr<T> Add();
        template <typename T>
        void Remove();
        template <typename T>
        requires std::is_base_of_v<Component::ComponentBase, T>
        std::shared_ptr<T> RequireComponent();
        template <typename T>
        std::weak_ptr<T> Catch();
        template <typename T>
        std::vector<std::weak_ptr<T>> Catches();
        void OnDrawGui();
        void SetEnable(bool enable) const;
        void OnDestroy();

    private:
        void MoveAdd(const std::shared_ptr<Component::ComponentBase>& move);
        // NOTE: Component を破棄する時の共通処理（OnDestroy / トークンのキャンセル / Registry からの登録解除）
        void DestroyComponent(const std::shared_ptr<Component::ComponentBase>& component);

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
    void ComponentGroup::Remove()
    {
        // NOTE: OnDestroy の中で components_ が変わっても安全なよう、先に対象を集める
        std::vector<std::shared_ptr<Component::ComponentBase>> targets;
        for (const auto& component : components_)
        {
            if (dynamic_cast<T*>(component.get()) != nullptr)
                targets.push_back(component);
        }

        for (const auto& component : targets)
        {
            // NOTE: OnDestroy の中で Components() を使えるよう、外すのはその後
            DestroyComponent(component);
            std::erase(components_, component);
        }
    }

    template <typename T>
    requires std::is_base_of_v<Component::ComponentBase, T>
    std::shared_ptr<T> ComponentGroup::RequireComponent()
    {
        auto component = Catch<T>();
        if (component.expired())
            return Add<T>();
        
        return component.lock();
    }
    
    template <typename T>
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

    template <typename T>
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
