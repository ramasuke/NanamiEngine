#include "Engine_Module_ProjectConfig.h"

namespace NanamiEngine::Module::ProjectConfig
{
    std::string BuildPath(const std::string& addPath, const std::string& key)
    {
        return std::string(PROJECT_CONFIG_FOLDER_PATH) + addPath + key + PROJECT_CONFIG_FILE_EXT;
    }

    void EnsureDirectory(const std::string& path)
    {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    }
}
