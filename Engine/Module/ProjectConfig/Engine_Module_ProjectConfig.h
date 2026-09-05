#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <stdexcept>

#include <cereal/archives/json.hpp>

#include "../Exception/Engine_Module_Exception.h"
#include "../Log/NanamiEngine_Module_Log.h"
#include "../Serialization/Engine_Module_Serialization.h"

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
            Serialization::SaveJsonFile(tmpPath, [&](cereal::JSONOutputArchive& ar)
            {
                ar(cereal::make_nvp(key, value));
            });
            std::filesystem::rename(tmpPath, path);
        }
        catch (const Exception::SerializationException&)
        {
            std::filesystem::remove(tmpPath);
            throw;
        }
        catch (const std::exception& e)
        {
            std::filesystem::remove(tmpPath);
            throw Exception::SerializeException(path, e.what());
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
            T value;
            Serialization::LoadJsonFile(path, [&](cereal::JSONInputArchive& ar)
            {
                ar(cereal::make_nvp(key, value));
            });
            return value;
        }
        catch (const Exception::SerializationException& e)
        {
            // 設定ファイルの破損はデフォルト値で継続するが、黙って握りつぶさず警告を残す
            LogWarning("ProjectConfig: " + std::string(e.what()) + " -> デフォルト値を使用します");
            return defaultValue;
        }
    }
}
