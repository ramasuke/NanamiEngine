#include "File.h"
#include <fstream>
#include <ranges>
#include <utility>

#include <filesystem>
#include "../../../Module/Asset/Sprite/SpriteFile.h"
#include "../../../Module/Asset/Scene/SceneFile.h"
#include "../../../Module/Asset/Hlsl/HlslFile.h"
#include "../../../Module/Asset/Hlsl/HlslVsFile.h"
#include "../../../Module/Asset/Hlsl/HlslPsFile.h"
#include "../../Application/Window/Popup/Group/PopupWindowGroup.h"
#include "../../Application/Window/Popup/Inspector/InspectorWindow.h"
#include "cereal/archives/json.hpp"
#include "cereal/archives/portable_binary.hpp"

namespace NanamiEngine::Core::FileSystem
{
    File File::LoadFileForMeta(
        const std::string& filePath, std::string fileName)
    {
        File file;
        file.filePath_ = filePath;
        file.fileName_ = std::move(fileName);
        file.content_  = Module::Asset::AssetFactory::Instance().Load(filePath);
        return std::move(file);
    }
    
    File File::CreateOrLoadFile(
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
            return std::move(LoadFileForMeta(filePath, fileName));
        }
    
        File file;
        file.filePath_ = std::move(filePath);
        file.fileName_ = std::move(fileName);
        return file;
    }

    File File::Copy() const
    {
        File copied;

        //fileName_
        {
            std::filesystem::path p(fileName_);
            std::string newName = p.stem().string() + "_copy" + p.extension().string();
            copied.fileName_ = newName;
        }

        //filePath_
        {
            std::filesystem::path p(filePath_);
            std::string newName = p.stem().string() + "_copy" + p.extension().string();

            if (p.has_parent_path())
                copied.filePath_ = (p.parent_path() / newName).string();
            else
                copied.filePath_ = newName;
        }

        //contentのコピー
        if (content_)
        {
            std::stringstream ss;
            {
                cereal::PortableBinaryOutputArchive oarchive(ss);
                oarchive(content_);
            }

            {
                cereal::PortableBinaryInputArchive iarchive(ss);
                iarchive(copied.content_);
                copied.content_->CopiedInit();
            }
        }

        return copied;
    }


    bool File::Rename(const std::string& newFileName)
    {
        if (newFileName.empty() || newFileName == fileName_)
            return false;

        const std::filesystem::path oldPath(filePath_);
        const std::filesystem::path newPath = oldPath.has_parent_path()
            ? oldPath.parent_path() / newFileName
            : std::filesystem::path(newFileName);

        std::error_code ec;
        if (std::filesystem::exists(oldPath))
        {
            std::filesystem::rename(oldPath, newPath, ec);
            if (ec)
                return false;
        }

        const std::filesystem::path oldMetaPath = filePath_ + ".meta";
        if (std::filesystem::exists(oldMetaPath))
        {
            std::filesystem::rename(oldMetaPath, newPath.string() + ".meta", ec);
            if (ec)
                return false;
        }

        filePath_ = newPath.string();
        fileName_ = newFileName;

        if (content_)
        {
            content_->OnRenamed(filePath_);
            OnSave();
        }

        return true;
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
