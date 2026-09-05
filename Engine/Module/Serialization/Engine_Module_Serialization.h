#pragma once
#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <utility>

#include <../cereal/include/cereal/archives/json.hpp>
#include <../cereal/include/cereal/archives/portable_binary.hpp>

#include "../Exception/Engine_Module_Exception.h"

// cereal での読み書きをまとめるヘルパー。
// ここが cereal / iostream の生例外を見る唯一の場所で、必ず Exception::SerializationException 系に変換して投げる。
// 呼び出し側は投げっぱなしにし、回復できる境界（nullptr / 既定値 / スキップを決められる場所）で
// catch (const Exception::SerializationException&) する。
namespace NanamiEngine::Module::Serialization
{
    namespace Detail
    {
        /** 開いた入力ストリームに対して ArchiveT を作り read を呼ぶ。cereal / iostream の例外を DeserializeException に変換する */
        template <class ArchiveT, class ReadFn>
        void ReadWith(std::istream& stream, const std::string& label, ReadFn&& read)
        {
            try
            {
                ArchiveT archive(stream);
                read(archive);
            }
            catch (const Exception::NanamiException&)
            {
                // ネストした読み込みが既に変換済みの例外を投げた場合はそのまま通す
                throw;
            }
            catch (const std::exception& exception)
            {
                throw Exception::DeserializeException(label, exception.what());
            }
        }

        /** 開いた出力ストリームに対して ArchiveT を作り write を呼ぶ。cereal / iostream の例外を SerializeException に変換する */
        template <class ArchiveT, class WriteFn>
        void WriteWith(std::ostream& stream, const std::string& label, WriteFn&& write)
        {
            try
            {
                ArchiveT archive(stream);
                write(archive);
            }
            catch (const Exception::NanamiException&)
            {
                throw;
            }
            catch (const std::exception& exception)
            {
                throw Exception::SerializeException(label, exception.what());
            }
        }
    }

    /** ファイルを開き JSONInputArchive を read に渡す。
     *  開けなければ FileNotFoundException、cereal の失敗は DeserializeException を投げる */
    template <class ReadFn>
    void LoadJsonFile(const std::string& filePath, ReadFn&& read)
    {
        std::ifstream ifStream(filePath);
        if (!ifStream.is_open())
            throw Exception::FileNotFoundException(filePath);

        Detail::ReadWith<cereal::JSONInputArchive>(ifStream, filePath, std::forward<ReadFn>(read));
    }

    /** ファイルが無い（または 0 byte）ときは何もせず false を返す版。
     *  「未作成 = 空オブジェクト」を許容する読み込み（エディタの新規作成 → Save フロー）で使う。
     *  0 byte を未作成扱いにするのは、cereal が空ストリームで RapidJSONException を投げるため（防御的な措置）。
     *  破損は LoadJsonFile と同じく DeserializeException を投げる */
    template <class ReadFn>
    bool LoadJsonFileIfExists(const std::string& filePath, ReadFn&& read)
    {
        std::ifstream ifStream(filePath);
        if (!ifStream.is_open())
            return false;
        if (ifStream.peek() == std::char_traits<char>::eof())
            return false;

        Detail::ReadWith<cereal::JSONInputArchive>(ifStream, filePath, std::forward<ReadFn>(read));
        return true;
    }

    /** ファイルを開き JSONOutputArchive を write に渡す。開けない・cereal の失敗は SerializeException を投げる */
    template <class WriteFn>
    void SaveJsonFile(const std::string& filePath, WriteFn&& write)
    {
        std::ofstream ofStream(filePath);
        if (!ofStream.is_open())
            throw Exception::SerializeException(filePath, "Failed to open file for writing");

        Detail::WriteWith<cereal::JSONOutputArchive>(ofStream, filePath, std::forward<WriteFn>(write));
    }

    /** in-memory バイナリ（stringstream など）からの読み込み。label は例外メッセージ用の識別子 */
    template <class ReadFn>
    void LoadPortableBinary(std::istream& stream, const std::string& label, ReadFn&& read)
    {
        Detail::ReadWith<cereal::PortableBinaryInputArchive>(stream, label, std::forward<ReadFn>(read));
    }
}
