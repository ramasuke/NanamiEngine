#include "ImageRenderer.h"

#include "../../GameObject/Transform/Transform.h"

void Component::ImageRenderer::SetSprite(const std::weak_ptr<Asset::SpriteFile>& sprite)
{
    spriteFile_ = sprite;
}

void Component::ImageRenderer::InitRenderer()
{
}

void Component::ImageRenderer::OnUserInterfaceRender()
{
    if (!IsEnable() || !spriteFile_)
        return;
    
    const auto renderPos    = Transform().GetWorldPos  ();
    const auto renderRot    = Transform().GetWorldRot  ();
    const auto renderScale  = Transform().GetWorldScale();

    const float angle = renderRot  .z;
    const float scale = renderScale.x;

    DrawRotaGraphF(
        renderPos.x,
        renderPos.y,     
        scale,                        
        angle,                        
        spriteFile_->GetDxLibHandle(),
        TRUE                          
    );
}

void Component::ImageRenderer::OnDrawGui()
{
    ImGui::Text(std::to_string(IsEnable()).c_str());
    if (ImGui::Button("ChangeIsEnable"))
    {
        SetEnable(!IsEnable());
    }
    ImGuiHelper::OnDrawInputField("spriteFile_", spriteFile_);
    ImGuiHelper::OnDrawInputField("renderPriority_", renderPriority_);
}
