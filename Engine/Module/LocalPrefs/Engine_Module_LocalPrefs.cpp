#include "Engine_Module_LocalPrefs.h"

namespace NanamiEngine::Module::LocalPrefs
{
    std::string BuildPath(const std::string& addPath, const std::string& key)
    {
        // NOTE: sizeof(constexpr文字列) は null終端を含むため -1 して純粋な文字数を得る
        constexpr size_t BASE_FOLDER_PATH_SIZE = sizeof(LOCAL_PREFS_DATA_FOLDER_PATH) - 1;
        constexpr size_t EXTENSION_LABEL_SIZE  = sizeof(LOCAL_PREFS_DATA_FILE_EXTENSION_LABEL) - 1;

        std::string path;
        // NOTE: append のたびに再アロケーションが起きないよう、最終的なサイズを先に確保する
        path.reserve(BASE_FOLDER_PATH_SIZE + addPath.size() + key.size() + EXTENSION_LABEL_SIZE);

        // NOTE: 結果: "LocalPrefs/[addPath][key].json"
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
