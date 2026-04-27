#include "File.h"
#include <fstream>
#include <ranges>
#include <utility>

#include "../../../Module/Asset/Sprite/SpriteFile.h"
#include "../../../Module/Asset/Scene/SceneFile.h"
#include "../../Application/Window/Popup/Group/PopupWindowGroup.h"
#include "../../Application/Window/Popup/Inspector/InspectorWindow.h"
#include "cereal/archives/json.hpp"

namespace NanamiEngine::Core::FileSystem
{
    File File::OnLoadForMeta(
        const std::string& filePath, std::string fileName)
    {
        File file;
        file.filePath_ = filePath;
        file.fileName_ = std::move(fileName);
        file.content_  = Module::Asset::AssetFactory::Instance().Load(filePath);
        return std::move(file);
    }
    
    File File::CreateOrLoad(
        std::string filePath,
        std::string fileName)
    {
        if (std::shared_ptr<Module::Asset::AssetBase> asset; Module::Asset::AssetFactory::Instance().TryCreate(filePath, asset))
        {
            if (!std::filesystem::exists(filePath + ".meta"))
            {
                File file;
                file.filePath_ = std::move(filePath);
                file.fileName_ = std::move(fileName);
                file.content_ = asset;
                return std::move(file);
            }
            return std::move(OnLoadForMeta(filePath, fileName));
        }
    
        File file;
        file.filePath_ = std::move(filePath);
        file.fileName_ = std::move(fileName);
        return file;
    }
    
    void File::OnSave() const
    {
        if (!content_)
            return;
    
        std::ofstream ofStream(filePath_ + ".meta");
        if (!ofStream.is_open())
            return;
    
        cereal::JSONOutputArchive archive(ofStream);
        archive(content_);
    
        content_->OnSaveCallback();
    }
    
    void File::OnClick() const
    {
        for (auto* inspector : Application::ApplicationBase::PopupWindows().Catch<PopupWindow::InspectorWindow>())
        {
            inspector->TryAddDisplayObject(content_);
        }
    }
    
    void File::OnDoubleClick() const
    {
        content_->OnDoubleClick();
    }
}
