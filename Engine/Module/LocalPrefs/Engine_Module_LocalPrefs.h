#pragma once
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

// ゲーム側のデータ保存・復元用モジュール
// セーブデータやプレイヤーの進行状況など、ゲーム固有のデータをローカルのJSONファイルに永続化するために使用する
namespace NanamiEngine::Module::LocalPrefs
{
    template<class T>
    concept Serializable = requires(T value, std::ostream& ofstream, std::istream& ifstream)
    {
        cereal::JSONOutputArchive(ofstream)(value);
        cereal::JSONInputArchive(ifstream)(value);
    };

    // 保存先ルートフォルダ。実行ファイルからの相対パスで解決される
    constexpr auto LOCAL_PREFS_DATA_FOLDER_PATH = "LocalPrefs/";
    constexpr auto LOCAL_PREFS_DATA_FILE_EXTENSION_LABEL = ".json";

    // NOTE: "LocalPrefs/[addPath][key].json" の形式でフルパスを組み立てる
    std::string BuildPath(const std::string& addPath, const std::string& key);
    
    // NOTE: ファイルパスの親ディレクトリが存在しない場合、再帰的に作成する
    void EnsureDirectory(const std::string& path);

    // Save/Load の公開APIが共通で使う内部実装
    // NOTE: 直接呼び出しは非推奨
    template<Serializable T>
    void SaveImpl(const std::string& fullPath,
                  const std::string& key,
                  const T& value)
    {
        EnsureDirectory(fullPath);

        // 書き込み中にクラッシュしても既存ファイルが壊れないよう、一時ファイルに書いてからリネームする
        const std::string tmpPath = fullPath + ".tmp";
        try
        {
            // JSON のルートキーをデータ型名ではなく key 文字列で固定することで、型名変更後も読み込めるようにする
            Serialization::SaveJsonFile(tmpPath, [&](cereal::JSONOutputArchive& archive)
            {
                archive(cereal::make_nvp(key, value));
            });
            // ofstream のスコープを抜けてフラッシュ・クローズが完了した後にリネームする
            std::filesystem::rename(tmpPath, fullPath);
        }
        catch (const Exception::SerializationException&)
        {
            // 書き込み失敗時は中途半端な一時ファイルを削除してから上位に投げる
            std::filesystem::remove(tmpPath);
            throw;
        }
        catch (const std::exception& exception)
        {
            // rename 失敗（filesystem_error）なども SerializeException に揃える
            std::filesystem::remove(tmpPath);
            throw Exception::SerializeException(fullPath, exception.what());
        }
    }

    template<Serializable T>
    T LoadImpl(const std::string& fullPath,
               const std::string& key)
    {
        T value;
        // ファイルが無い → FileNotFoundException、破損 → DeserializeException。
        // 呼び出し側が種類ごとに扱えるよう、型付き例外でそのまま通知する
        Serialization::LoadJsonFile(fullPath, [&](cereal::JSONInputArchive& archive)
        {
            // SaveImpl と同じキー名を指定することで、JSON上のフィールドと型を対応付ける
            archive(cereal::make_nvp(key, value));
        });
        return value;
    }

    /** --- 公開API --- */
    // ファイルが存在しない・破損している場合は例外を投げる。確実に存在することが前提のデータに使う
    template<Serializable T>
    void Save(const std::string& key, const T& value)
    {
        const std::string path = BuildPath("", key);
        SaveImpl(path, key, value);
    }

    // サブフォルダ付きで保存する。同じキー名のデータを種別ごとに分けたい場合に使う
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

    // ファイルが存在しない・読み込みエラーの場合は例外を投げずに defaultValue を返す。初回起動時など未保存状態が正常なデータに使う
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
            // JSONの破損やスキーマ不一致など、読み込みエラーはデフォルト値で継続するが、黙って握りつぶさず警告を残す
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

    // ファイルの有無や読み込みの成否を optional で返す。呼び出し側が存在チェックと値取得を同時に行いたい場合に使用する
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