#include "HlslFile.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    HlslFile::HlslFile(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
    }

    std::string HlslFile::GetContentPath() const
    {
        return contentPath_;
    }

    void HlslFile::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::HlslFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::HlslFile);
REGISTER_ASSET(HlslFile, ".hlsl")
#pragma endregion
