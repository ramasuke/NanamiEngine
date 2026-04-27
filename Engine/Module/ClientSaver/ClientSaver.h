#pragma once
#include <string>
#include <fstream>
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>

namespace NanamiEngine::Module::ClientSaver
{
    constexpr auto CLIENT_SAVER_DATA_FOLDER_PATH          = "ClientSaverData/";
    constexpr auto CLIENT_SAVER_DATA_FILE_EXTENSION_LABEL = ".json";

    namespace NanamiEngine::Module::ClientSaver
    {
        /**
        * @brief addFilePathしたディレクトリにValueの値をKeyの値をつかって保存する。
        * 
        *  @param key   保存する値のキー
        *  @param value 保存する値
        */
        template<typename T>
        void SaveWithPath(const std::string& addFilePath,
                          const std::string& key,
                          const T& value)
        {
            std::ofstream os(
                std::string(CLIENT_SAVER_DATA_FOLDER_PATH) +
                addFilePath +
                key +
                CLIENT_SAVER_DATA_FILE_EXTENSION_LABEL
            );

            if (!os)
            {
                throw std::runtime_error("Failed to open file for saving: " + key + ".json");
            }

            cereal::JSONOutputArchive archive(os);
            archive(value);
        }

        /**
        * @brief   addFilePathしたディレクトリからKeyの値を使って値Tを復元する。
        * @warning addFilePathしたディレクトリからKeyが存在しない場合はruntime_errorが投げられる。
        * 
        *  @param key 復元する値のキー
        */
        template<typename T>
        T LoadWithPath(const std::string& addFilePath,
                       const std::string& key)
        {
            std::ifstream is(
                std::string(CLIENT_SAVER_DATA_FOLDER_PATH) +
                addFilePath +
                key +
                CLIENT_SAVER_DATA_FILE_EXTENSION_LABEL
            );

            if (!is)
            {
                throw std::runtime_error("Failed to open file for loading: " + key + ".json");
            }

            cereal::JSONInputArchive archive(is);

            T value;
            archive(value);
            return value;
        }
    }
    
    /**
     * @brief Valueの値をKeyの値をつかって保存する。
     * 
     *  @param key   保存する値のキー
     *  @param value 保存する値
     */
    template<typename T>
    void Save(const std::string& key, const T& value)
    {
        std::ofstream ofStream(CLIENT_SAVER_DATA_FOLDER_PATH + key + CLIENT_SAVER_DATA_FILE_EXTENSION_LABEL);
        if (!ofStream)
        {
            throw std::runtime_error("Failed to open file for saving: " + key + ".json");
        }

        cereal::JSONOutputArchive archive(ofStream);
        archive(value);
    }

    /**
     * @brief   Keyの値を使って値Tを復元する。
     * @warning Keyが存在しない場合はruntime_errorが投げられる。
     * 
     *  @param key 復元する値のキー
     */
    template<typename T>
    T Load(const std::string& key)
    {
        std::ifstream ifStream(CLIENT_SAVER_DATA_FOLDER_PATH + key + CLIENT_SAVER_DATA_FILE_EXTENSION_LABEL);
        if (!ifStream)
        {
            throw std::runtime_error("Failed to open file for loading: " + key + ".json");
        }

        cereal::JSONInputArchive archive(ifStream);
        T value;
        archive(value);
        return value;
    }
}