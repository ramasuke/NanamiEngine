#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#include <cereal/archives/json.hpp>

namespace NanamiEngine::Module::ProjectConfig
{
    template<class T>
    concept Serializable = requires(T value, std::ostream& os, std::istream& is)
    {
        cereal::JSONOutputArchive(os)(value);
        cereal::JSONInputArchive(is)(value);
    };

    constexpr auto PROJECT_CONFIG_FOLDER_PATH = "ProjectConfig/";
    constexpr auto PROJECT_CONFIG_FILE_EXT    = ".json";

    std::string BuildPath(const std::string& addPath, const std::string& key);
    void        EnsureDirectory(const std::string& path);

    template<Serializable T>
    void SaveWithPath(const std::string& addPath, const std::string& key, const T& value)
    {
        const std::string path    = BuildPath(addPath, key);
        const std::string tmpPath = path + ".tmp";
        EnsureDirectory(path);
        try
        {
            {
                std::ofstream os(tmpPath);
                cereal::JSONOutputArchive ar(os);
                ar(cereal::make_nvp(key, value));
            }
            std::filesystem::rename(tmpPath, path);
        }
        catch (const std::exception& e)
        {
            std::filesystem::remove(tmpPath);
            throw std::runtime_error("ProjectConfig Save failed: " + std::string(e.what()));
        }
    }

    template<Serializable T>
    T LoadOrDefaultWithPath(const std::string& addPath, const std::string& key, const T& defaultValue)
    {
        const std::string path = BuildPath(addPath, key);
        if (!std::filesystem::exists(path))
            return defaultValue;
        try
        {
            std::ifstream is(path);
            cereal::JSONInputArchive ar(is);
            T value;
            ar(cereal::make_nvp(key, value));
            return value;
        }
        catch (...) { return defaultValue; }
    }
}
