#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <optional>
#include <stdexcept>

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>

namespace NanamiEngine::Module::LocalPrefs
{
    template<class T>
    concept Serializable = requires(T value, std::ostream& ofstream, std::istream& ifstream)
    {
        cereal::JSONOutputArchive(ofstream)(value);
        cereal::JSONInputArchive(ifstream)(value);
    };
    
    constexpr auto LOCAL_PREFS_DATA_FOLDER_PATH = "LocalPrefs/";
    constexpr auto LOCAL_PREFS_DATA_FILE_EXTENSION_LABEL = ".json";

    std::string BuildPath(const std::string& addPath, const std::string& key);
    void EnsureDirectory(const std::string& path);

    template<Serializable T>
    void SaveImpl(const std::string& fullPath,
                  const std::string& key,
                  const T& value)
    {
        EnsureDirectory(fullPath);
        const std::string tmpPath = fullPath + ".tmp";
        try
        {
            {
                std::ofstream ofstream(tmpPath);
                if (!ofstream)
                {
                    throw std::runtime_error("Failed to open temp file: " + tmpPath);
                }

                cereal::JSONOutputArchive archive(ofstream);
                archive(cereal::make_nvp(key, value));
            }

            std::filesystem::rename(tmpPath, fullPath);
        }
        catch (const std::exception& exception)
        {
            std::filesystem::remove(tmpPath);
            throw std::runtime_error("Save failed: " + std::string(exception.what()));
        }
    }

    template<Serializable T>
    T LoadImpl(const std::string& fullPath,
               const std::string& key)
    {
        std::ifstream ifstream(fullPath);

        if (!ifstream)
        {
            throw std::runtime_error("File not found: " + fullPath);
        }

        try
        {
            cereal::JSONInputArchive archive(ifstream);

            T value;
            archive(cereal::make_nvp(key, value));
            return value;
        }
        catch (const std::exception& exception)
        {
            throw std::runtime_error("Load failed: " + std::string(exception.what()));
        }
    }

    template<Serializable T>
    void Save(const std::string& key, const T& value)
    {
        const std::string path = BuildPath("", key);
        SaveImpl(path, key, value);
    }

    template<Serializable T>
    void SaveWithPath(const std::string& addPath,
                      const std::string& key,
                      const T& value)
    {
        const std::string path = BuildPath(addPath, key);
        SaveImpl(path, key, value);
    }

    template<Serializable T>
    T Load(const std::string& key)
    {
        const std::string path = BuildPath("", key);
        return LoadImpl<T>(path, key);
    }

    template<Serializable T>
    T LoadWithPath(const std::string& addPath,
                   const std::string& key)
    {
        const std::string path = BuildPath(addPath, key);
        return LoadImpl<T>(path, key);
    }

    template<Serializable T>
    T LoadOrDefault(const std::string& key, const T& defaultValue)
    {
        const std::string path = BuildPath("", key);

        if (!std::filesystem::exists(path))
        {
            return defaultValue;
        }

        try
        {
            return LoadImpl<T>(path, key);
        }
        catch (...)
        {
            return defaultValue;
        }
    }

    template<Serializable T>
    T LoadOrDefaultWithPath(const std::string& addPath,
                            const std::string& key,
                            const T& defaultValue)
    {
        const std::string path = BuildPath(addPath, key);

        if (!std::filesystem::exists(path))
        {
            return defaultValue;
        }

        try
        {
            return LoadImpl<T>(path, key);
        }
        catch (...)
        {
            return defaultValue;
        }
    }

    template<Serializable T>
    std::optional<T> TryLoad(const std::string& key)
    {
        const std::string path = BuildPath("", key);

        if (!std::filesystem::exists(path))
        {
            return std::nullopt;
        }

        try
        {
            return LoadImpl<T>(path, key);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
}