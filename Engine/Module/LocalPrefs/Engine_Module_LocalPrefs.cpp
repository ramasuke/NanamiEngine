#include "Engine_Module_LocalPrefs.h"

namespace NanamiEngine::Module::LocalPrefs
{
    std::string BuildPath(const std::string& addPath, const std::string& key)
    {
        constexpr size_t BASE_FOLDER_PATH_SIZE = sizeof(LOCAL_PREFS_DATA_FOLDER_PATH) - 1;
        constexpr size_t EXTENSION_LABEL_SIZE  = sizeof(LOCAL_PREFS_DATA_FILE_EXTENSION_LABEL) - 1;

        std::string path;
        path.reserve(BASE_FOLDER_PATH_SIZE + addPath.size() + key.size() + EXTENSION_LABEL_SIZE);

        path.append(LOCAL_PREFS_DATA_FOLDER_PATH)
            .append(addPath)
            .append(key)
            .append(LOCAL_PREFS_DATA_FILE_EXTENSION_LABEL);

        return path;
    }

    void EnsureDirectory(const std::string& path)
    {
        const std::filesystem::path p(path);
        std::filesystem::create_directories(p.parent_path());
    }
}
