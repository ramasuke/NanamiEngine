#include "SpriteAnimationFile.h"

#include "DxLib.h"

namespace NanamiEngine::Module::Asset
{
    SpriteAnimationFile::SpriteAnimationFile(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
    }

    void SpriteAnimationFile::OnEnableAsset()
    {
        LoadSprite();
    }

    void SpriteAnimationFile::OnSaveCallback()
    {
        LoadSprite();
    }

    void SpriteAnimationFile::LoadSprite()
    {
        switch (sourceType_)
        {
        case AnimationSourceType::Individual:
            break;
        case AnimationSourceType::SpriteSheet:
            spritesDxlibHandle_.resize(splitCount_);
            LoadDivGraph(sprite_->GetContentPath().c_str(),
                splitCount_,
                splitXCount_,
                splitYCount_,
                splitSizeX_,
                splitSizeY_,
                spritesDxlibHandle_.data());
            break;
        }
    }

    void SpriteAnimationFile::OnDrawGui()
    {
        const char* items[] = { "Individual", "SpriteSheet" };
        int current = static_cast<int>(sourceType_);
        if (ImGui::Combo("Source Type", &current, items, IM_ARRAYSIZE(items)))
        {
            sourceType_ = static_cast<AnimationSourceType>(current);
        }

        switch (sourceType_)
        {
        case AnimationSourceType::Individual:
            ImGuiHelper::OnDrawInputField("spriteFiles_", sprites_, [this]
            {
                if (ImGui::Button("Add"))
                    sprites_.emplace_back();
            });
            break;
        case AnimationSourceType::SpriteSheet:
            ImGuiHelper::OnDrawInputField("sprite_", sprite_);
            ImGuiHelper::OnDrawInputField("splitCount_", splitCount_);
            ImGuiHelper::OnDrawInputField("splitXCount_", splitXCount_);
            ImGuiHelper::OnDrawInputField("splitYCount_", splitYCount_);
            ImGuiHelper::OnDrawInputField("splitX_", splitSizeX_);
            ImGuiHelper::OnDrawInputField("splitY_", splitSizeY_);
            break;
        }
    }
}
