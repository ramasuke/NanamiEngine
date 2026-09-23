#include "AddComponent.h"
#include "../ComponentHeaders.h"

namespace
{
    struct RegisteredMenu
    {
        NanamiEngine::Module::GameObject::AddComponent::DrawMenuFunc draw;
        int order;
    };

    // NOTE: static 初期化順に依存しないよう関数内 static
    std::vector<RegisteredMenu>& RegisteredMenus()
    {
        static std::vector<RegisteredMenu> menus;
        return menus;
    }
}

bool GameObject::AddComponent::RegisterMenu(const DrawMenuFunc draw, const int order)
{
    auto& menus = RegisteredMenus();
    const auto it = std::upper_bound(menus.begin(), menus.end(), order,
        [](const int value, const RegisteredMenu& menu) { return value < menu.order; });
    menus.insert(it, RegisteredMenu{ draw, order });
    return true;
}

std::shared_ptr<Component::ComponentBase> GameObject::AddComponent::OnDrawGui()
{
    if (ImGui::Button("Add Component"))
    {
        ImGui::OpenPopup("Add Component Menu");
    }

    std::shared_ptr<Component::ComponentBase> addComponent;
    if (ImGui::BeginPopup("Add Component Menu"))
    {
        OnDrawRendererGui   (addComponent);
        OnDrawUiRendererGui (addComponent);
        OnDrawSoundGui      (addComponent);
        OnDrawColliderGui   (addComponent);
        OnDrawCinemachineGui(addComponent);
        OnDrawNetworkGui    (addComponent);
        for (const auto& menu : RegisteredMenus())
        {
            menu.draw(addComponent);
        }
        ImGui::EndPopup();
        ImGui::Spacing();
    }
    
    return addComponent;
}

void GameObject::AddComponent::OnDrawRendererGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Renderer"))
    {
        OnDrawTryAddComponentGui<Component::SphereRenderer >(addComponent);
        OnDrawTryAddComponentGui<Component::ModelRenderer  >(addComponent);
        OnDrawTryAddComponentGui<Component::QuadRenderer   >(addComponent);
        OnDrawTryAddComponentGui<Component::Animator       >(addComponent);
        OnDrawTryAddComponentGui<Component::BoneSync       >(addComponent);
        OnDrawTryAddComponentGui<Component::LookAtBone     >(addComponent);
        OnDrawTryAddComponentGui<Component::ParticleSystem >(addComponent);
        OnDrawTryAddComponentGui<Component::DirectionLight >(addComponent);
        OnDrawTryAddComponentGui<Component::SkyDome3D      >(addComponent);
        OnDrawTryAddComponentGui<Component::Rotator        >(addComponent);
        OnDrawTryAddComponentGui<Component::CameraFollowTransform>(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawSoundGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Sound"))
    {
        OnDrawTryAddComponentGui<Component::AudioSource        >(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawUiRendererGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("UiRenderer"))
    {
        OnDrawTryAddComponentGui<Component::ImageRenderer        >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::BlendImageRenderer    >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::CircleGaugeRenderer   >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::BlendAnimationRenderer>(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::ImageAnimationRenderer>(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::Slider                >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::TextRenderer          >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::Billboard3D           >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::BillboardAnimation3D  >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::Button                >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::MovieRenderer         >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::GridLayoutGroup       >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::HorizontalLayoutGroup >(addComponent);
        OnDrawTryAddComponentGui<NanamiUi::VerticalLayoutGroup   >(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawColliderGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Collider"))
    {
        OnDrawTryAddComponentGui<Component::RigidBody         >(addComponent);
        OnDrawTryAddComponentGui<Component::BoxCollider       >(addComponent);
        OnDrawTryAddComponentGui<Component::SphereCollider    >(addComponent);
        OnDrawTryAddComponentGui<Component::CapsuleCollider   >(addComponent);
        OnDrawTryAddComponentGui<Component::CylinderCollider  >(addComponent);
        OnDrawTryAddComponentGui<Component::StaticMeshCollider>(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
    if (ImGui::TreeNode("Collision"))
    {
        OnDrawTryAddComponentGui<Component::CollisionListener >(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawCinemachineGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Cinema chineCamera"))
    {
        OnDrawTryAddComponentGui<CineMachine::CinemachineCameraBrain  >(addComponent);
        OnDrawTryAddComponentGui<CineMachine::CineMachineVirtualCamera>(addComponent);
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

void GameObject::AddComponent::OnDrawNetworkGui(std::shared_ptr<Component::ComponentBase>& addComponent)
{
    if (ImGui::TreeNode("Network"))
    {
        if (ImGui::TreeNode("Component"))
        {
            OnDrawTryAddComponentGui<Network::NetworkGameObject>(addComponent);
            OnDrawTryAddComponentGui<Network::NetworkTransform >(addComponent);
            OnDrawTryAddComponentGui<Network::NetworkAnimator  >(addComponent);
            ImGui::TreePop();
            ImGui::Spacing();
        }
        
        ImGui::TreePop();
        ImGui::Spacing();
    }
}

std::string GameObject::AddComponent::StripNamespace(const std::string& name)
{
    const auto pos = name.rfind("::");
    return pos != std::string::npos ? name.substr(pos + 2) : name;
}
