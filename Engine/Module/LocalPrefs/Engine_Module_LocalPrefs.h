#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <optional>
#include <stdexcept>

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>

#include "../Exception/Engine_Module_Exception.h"
#include "../Log/NanamiEngine_Module_Log.h"
#include "../Serialization/Engine_Module_Serialization.h"

// NOTE: ゲーム固有のデータをローカルの JSON ファイルに保存・復元するモジュール
namespace NanamiEngine::Module::LocalPrefs
{
    template<class T>
    concept Serializable = requires(T value, std::ostream& ofstream, std::istream& ifstream)
    {
        cereal::JSONOutputArchive(ofstream)(value);
        cereal::JSONInputArchive(ifstream)(value);
    };

    // NOTE: 保存先ルートフォルダ。カレントディレクトリからの相対パスで解決される
    constexpr auto LOCAL_PREFS_DATA_FOLDER_PATH = "LocalPrefs/";
    constexpr auto LOCAL_PREFS_DATA_FILE_EXTENSION_LABEL = ".json";

    // NOTE: "LocalPrefs/[addPath][key].json" の形式でフルパスを組み立てる
    NANAMI_API std::string BuildPath(const std::string& addPath, const std::string& key);
    
    // NOTE: ファイルパスの親ディレクトリが存在しない場合、再帰的に作成する
    NANAMI_API void EnsureDirectory(const std::string& path);

    // NOTE: Save/Load 系の共通実装。直接は呼ばない
    template<Serializable T>
    void SaveImpl(const std::string& fullPath,
                  const std::string& key,
                  const T& value)
    {
        EnsureDirectory(fullPath);

        // NOTE: 書き込み中にクラッシュしても既存ファイルが壊れないよう、一時ファイルに書いてからリネームする
        const std::string tmpPath = fullPath + ".tmp";
        try
        {
            // NOTE: ルートキーを型名ではなく key にして、型名を変えても読めるようにする
            Serialization::SaveJsonFile(tmpPath, [&](cereal::JSONOutputArchive& archive)
            {
                archive(cereal::make_nvp(key, value));
            });
            // NOTE: ofstream のスコープを抜けてフラッシュ・クローズが完了した後にリネームする
            std::filesystem::rename(tmpPath, fullPath);
        }
        catch (const Exception::SerializationException&)
        {
            // NOTE: 書き込み失敗時は中途半端な一時ファイルを削除してから上位に投げる
            std::filesystem::remove(tmpPath);
            throw;
        }
        catch (const std::exception& exception)
        {
            // NOTE: rename 失敗（filesystem_error）なども SerializeException に揃える
            std::filesystem::remove(tmpPath);
            throw Exception::SerializeException(fullPath, exception.what());
        }
    }

    template<Serializable T>
    T LoadImpl(const std::string& fullPath,
               const std::string& key)
    {
        T value;
        // NOTE: ファイルが無い → FileNotFoundException、破損 → DeserializeException
        Serialization::LoadJsonFile(fullPath, [&](cereal::JSONInputArchive& archive)
        {
            // NOTE: SaveImpl と同じキー名を指定することで、JSON上のフィールドと型を対応付ける
            archive(cereal::make_nvp(key, value));
        });
        return value;
    }

    template<Serializable T>
    void Save(const std::string& key, const T& value)
    {
        const std::string path = BuildPath("", key);
        SaveImpl(path, key, value);
    }

    // NOTE: サブフォルダ付きで保存する。同じキー名のデータを種別ごとに分けたい場合に使う
    template<Serializable T>
    void SaveWithPath(const std::string& addPath,
                      const std::string& key,
                      const T& value)
    {
        const std::string path = BuildPath(addPath, key);
        SaveImpl(path, key, value);
    }

    // NOTE: ファイルが無い・壊れている時は例外を投げる。確実に存在することが前提のデータに使う
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

    // NOTE: ファイルが無い・読めない時は例外でなく defaultValue を返す。未保存が正常なデータに使う
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
        catch (const Exception::SerializationException& exception)
        {
            // NOTE: 読み込みエラーはデフォルト値で続行するが、黙って握りつぶさず警告は残す
            LogWarning("LocalPrefs: " + std::string(exception.what()) + " -> デフォルト値を使用します");
            return defaultValue;
        }
    }

    template<Serializable T>
    T LoadOrDefaultWithPath(const std::string& addPath,
                            const std::string& key,
                            T defaultValue)
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
        catch (const Exception::SerializationException& exception)
        {
            LogWarning("LocalPrefs: " + std::string(exception.what()) + " -> デフォルト値を使用します");
            return defaultValue;
        }
    }

    // NOTE: ファイルが無い・読めない時は nullopt。存在確認と取得を一度に済ませたい場合に使う
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
        catch (const Exception::SerializationException& exception)
        {
            LogWarning("LocalPrefs: " + std::string(exception.what()) + " -> nullopt を返します");
            return std::nullopt;
        }
    }
}