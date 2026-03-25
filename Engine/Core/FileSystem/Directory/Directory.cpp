#include "Directory.h"

#include "../../../Module/Asset/Sprite/SpriteFile.h"
#include "../../../Module/Asset/Factory/AssetFactory.h"

NanamiEngine::Core::FileSystem::Directory::Directory(const std::string& ownPath)
{
    if (!std::filesystem::exists(ownPath) || !std::filesystem::is_directory(ownPath))
        return;

    ownPath_ = ownPath;
    name_ = std::filesystem::path(ownPath).filename().string();

    for (const auto& entry : std::filesystem::directory_iterator(ownPath))
    {
        const std::string fileName = entry.path().filename().string();
        const std::string filePath = entry.path().string();

        if (entry.is_directory())
        {
            directories_.emplace_back(filePath);
        }
        else if (entry.is_regular_file())
        {
            if (LibCore::FilePath::IsExtension(filePath, ".meta"))
                continue;
            
            files_.push_back(File::CreateOrLoad(filePath, fileName));
        }
    }
}

void NanamiEngine::Core::FileSystem::Directory::OnSave()
{
    for (auto& dir : directories_)
    {
        dir.OnSave();
    }
    for (const auto& file : files_)
    {
        file.OnSave();
    }
}

void NanamiEngine::Core::FileSystem::Directory::AddFile(const File& file)
{
    files_.push_back(file);
}

NanamiEngine::Core::FileSystem::Directory& NanamiEngine::Core::FileSystem::Directory::AddDirectory()
{
    directories_.emplace_back("");
    return directories_.back();
}