#pragma once
#include <filesystem>
#include <vector>

namespace LibCore::FilePath
{
    static std::vector<std::filesystem::path> SortFileExtension(
        const std::filesystem::path& directory,
        const std::string& extension,
        const bool recursive = false
    )
    {
        std::vector<std::filesystem::path> result;

        if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
        {
            return result;
        }   

        if (recursive)
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
            {
                if (entry.is_regular_file() && entry.path().extension() == extension)
                {
                    result.push_back(entry.path());
                }
            }
        }
        else
        {
            for (const auto& entry : std::filesystem::directory_iterator(directory))
            {
                if (entry.is_regular_file() && entry.path().extension() == extension)
                {
                    result.push_back(entry.path());
                }
            }
        }

        return result;
    }

    [[nodiscard]] static bool IsExtension(const std::filesystem::path& path, const std::string& extension)
    {
        return path.extension() == extension;
    }
}
