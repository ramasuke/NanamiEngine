#pragma once
#include <memory>
#include <type_traits>

#include "../../../../Module/GameObject/Interface/IGameObject.h"
#include "../../../Application/Editor/EditorApplication.h"
#include "../../Registry/ObjectRegistry.h"
#include "../Interface/IFieldContext.h"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::Component
{
    class ComponentBase;
}

namespace NanamiEngine::Core::Object
{
    template <typename T>
    requires std::is_base_of_v<Module::Object::IObject, T>
    class FieldContext final : public IFieldContext
    {
    public:
        FieldContext() = default;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t) const
        {
            archive(guid_);
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t)
        {
            archive(guid_);
        }

        // Lifecycle
        void Init() override
        {
            if (content_.expired())
            {
                content_ = Application::ApplicationBase::ObjectRegistry().Catch<T>(guid_);
            }
        }

        explicit operator bool() const { return !content_.expired(); }
        std::shared_ptr<T> operator->() const { return content_.lock(); }
        std::shared_ptr<T> get() const { return content_.lock(); }
        void set(const std::shared_ptr<T>& content)
        {
            if (!content)
                return;

            guid_ = content->GetGuid();
            content_ = content;
        }

        [[nodiscard]] const Guid& Guid() const { return guid_; }

        void OnDrawGui()
        {
            DrawCurrentObjectInfo();
            HandleDragDrop();
        }

    private:
        void DrawCurrentObjectInfo()
        {
            if (auto content = content_.lock())
            {
                if constexpr (std::is_base_of_v<Asset::AssetBase, T>)
                {
                    ImGui::Text("Asset: %s", content->GetContentPath().c_str());
                }
                else if constexpr (std::is_base_of_v<GameObject::IGameObject, T>)
                {
                    ImGui::Text("GameObject: %s", content->Name());
                }
                else if constexpr (std::is_base_of_v<Component::ComponentBase, T>)
                {
                    ImGui::Text("%s Component: %s", typeid(T).name(), content->Entity().lock()->Name());
                }
                else
                {
                    ImGui::Text("Object: %s", typeid(T).name());
                }
            }
            else
            {
                ImGui::Text("No object assigned");
            }
        }

        void HandleDragDrop()
        {
            if (!ImGui::BeginDragDropTarget())
                return;

            if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload(FileSystem::EDITOR_DRAGGING_ITEM_PAYLOAD_TYPE))
            {
                if (const auto draggingGuid =
                    Application::EditorApplication::FileDraggingHand()
                        .TakeDraggingItemGuid())
                {
                    AssignFromDraggedObject(draggingGuid.value());
                }
            }

            ImGui::EndDragDropTarget();
        }

        template<typename U = T>
        requires std::derived_from<U, Module::Component::ComponentBase>
        void AssignFromDraggedObject(const class Guid guid)
        {
            if (const auto draggingGameObject =
                Application::ApplicationBase::ObjectRegistry()
                    .Catch<Module::GameObject::IGameObject>(guid)
                    .lock())
            {
                if (auto component = draggingGameObject->Components().Catch<U>().lock())
                {
                    content_ = component;
                    guid_ = component->GetGuid();
                }
            }
        }

        template<typename U = T>
        requires (!std::derived_from<U, Component::ComponentBase>)
        void AssignFromDraggedObject(const class Guid guid)
        {
            content_ = Application::ApplicationBase::ObjectRegistry().Catch<U>(guid);
            guid_ = guid;
        }

    private:
        ::Guid guid_;
        std::weak_ptr<T> content_;
    };
}