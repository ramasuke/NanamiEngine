#include "SpriteAnimationFile.h"

#include "DxLib.h"

namespace NanamiEngine::Module::Asset
{
    SpriteAnimationFile::SpriteAnimationFile(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
    }

    SpriteAnimationFile::~SpriteAnimationFile()
    {
        ReleaseSprites();
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
        // Save のたびに OnSaveCallback から読み直されるので、前回分を解放してから読み込む
        ReleaseSprites();

        switch (sourceType_)
        {
        case AnimationSourceType::Individual:
            break;
        case AnimationSourceType::SpriteSheet:
            spritesDxlibHandle_.assign(splitCount_, -1);
            if (LoadDivGraph(sprite_->GetContentPath().c_str(),
                splitCount_,
                splitXCount_,
                splitYCount_,
                splitSizeX_,
                splitSizeY_,
                spritesDxlibHandle_.data()) == -1)
            {
                spritesDxlibHandle_.clear();
            }
            break;
        }
    }

    void SpriteAnimationFile::ReleaseSprites()
    {
        for (const int handle : spritesDxlibHandle_)
        {
            if (handle != -1)
                DeleteGraph(handle);
        }
        spritesDxlibHandle_.clear();
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
