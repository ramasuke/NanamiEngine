#include "ComponentGroup.h"

#include "ImGuiHelper.h"
#include "../../../Core/Application/Editor/Camera/Free/Editor3DCamera.h"
#include "AddComponenet/AddComponent.h"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "ComponentHeaders.h"
#include "../../../../Libs/LibCore/cereal/PrefabExtractArchive/PrefabExtractArchive.h"
#include "../../../Core/Object/Registry/ObjectRegistry.h"

void GameObject::ComponentGroup::InitComponentGroup(const std::weak_ptr<IGameObject>& gameObject)
{
	ownerGameObject_ = gameObject;

    for(const auto& component : components_)
    {
        const std::weak_ptr weakComponent = component;
        Core::Application::ApplicationBase::GetMainWindow()->LifeCycle().DynamicAddCallback(weakComponent);
        component->InitComponent(gameObject);
        Core::Application::ApplicationBase::ObjectRegistry().Add(component);
    }
}

void GameObject::ComponentGroup::ResetGuid() const
{
    for (const auto& component : components_)
    {
        component->ResetGuid();
    }
}

void GameObject::ComponentGroup::OnDrawGui()
{
    std::vector<std::shared_ptr<Component::ComponentBase>> drawTargets;
    drawTargets.reserve(components_.size());

    for (const auto& comp : components_)
    {
        if (comp) drawTargets.push_back(comp);
    }

    for (const auto& component : drawTargets)
    {
        // 型名を取得して名前空間を除去
        std::string_view fullName = typeid(*component).name();
        const size_t lastColon = fullName.rfind("::");
        std::string_view displayClassName = (lastColon != std::string_view::npos)
            ? fullName.substr(lastColon + 2)
            : fullName;

        const std::string headerLabel = std::string(displayClassName) + "##" + component->GetGuid().Value();

        if (ImGui::CollapsingHeader(headerLabel.c_str()))
        {
            component->BasedOnDrawgui();
            component->OnDrawGui();
        }

        if (ImGui::BeginPopupContextItem(headerLabel.c_str()))
        {
            if (ImGui::MenuItem("Reset GUID"))
            {
                component->ResetGuid();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("ChangeEnable"))
            {
                component->SetEnable(!component->IsEnable());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete Component"))
            {
                if (const auto it = std::ranges::find(components_, component); it != components_.end())
                {
                    components_.erase(it);
                }
                ImGui::EndPopup();
                break;
            }
            ImGui::EndPopup();
        }
    }

    if (const auto addComponent = AddComponent::OnDrawGui(); addComponent != nullptr)
    {
        addComponent->InitComponent(ownerGameObject_);
        MoveAdd(addComponent);
    }
}

void GameObject::ComponentGroup::SetEnable(const bool enable) const
{
    for (const auto& component : components_)
    {
        component->SetEnable(enable);
    }
}

void GameObject::ComponentGroup::OnDestroy()
{
    for (const auto& component : components_)
    {
        component->OnDestroy();
    }   
}

void GameObject::ComponentGroup::MoveAdd(const std::shared_ptr<Component::ComponentBase>& move)
{
    Core::Application::ApplicationBase::GetMainWindow()->LifeCycle()
            .DynamicAddCallback(std::weak_ptr(move));

    components_.push_back(move);
}

template <class Archive>
void GameObject::ComponentGroup::save(Archive& archive, const std::uint32_t version) const {
    std::size_t count = components_.size();
    archive(cereal::make_nvp("componentCount", count));

    for (std::size_t i = 0; i < count; ++i) {
        archive(cereal::make_nvp("component_" + std::to_string(i), components_[i]));
    }
}

template <class Archive>
void GameObject::ComponentGroup::load(Archive& archive, const std::uint32_t version) {
    std::size_t count = 0;
    archive(cereal::make_nvp("componentCount", count));

    components_.clear();
    for (std::size_t i = 0; i < count; ++i) {
        std::shared_ptr<Component::ComponentBase> component;
        archive(cereal::make_nvp("component_" + std::to_string(i), component));
        if (component) {
            components_.emplace_back(component);
        }
    }
}

template void GameObject::ComponentGroup::save<cereal::JSONOutputArchive>(cereal::JSONOutputArchive&, const std::uint32_t) const;
template void GameObject::ComponentGroup::load<cereal::JSONInputArchive>(cereal::JSONInputArchive&, const std::uint32_t);
template void GameObject::ComponentGroup::save<cereal::PortableBinaryOutputArchive>(cereal::PortableBinaryOutputArchive&, const std::uint32_t) const;
template void GameObject::ComponentGroup::load<cereal::PortableBinaryInputArchive>(cereal::PortableBinaryInputArchive&, const std::uint32_t);
template void GameObject::ComponentGroup::save<LibCore::PrefabExtractArchive>(LibCore::PrefabExtractArchive&, const uint32_t) const;
template void GameObject::ComponentGroup::load<LibCore::PrefabExtractArchive>(LibCore::PrefabExtractArchive&, const uint32_t);
