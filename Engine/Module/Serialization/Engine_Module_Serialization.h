#pragma once
#include <fstream>
#include <istream>
#include <ostream>
#include <string>
#include <utility>

#include <../cereal/include/cereal/archives/json.hpp>
#include <../cereal/include/cereal/archives/portable_binary.hpp>

#include "../Exception/Engine_Module_Exception.h"

// NOTE: cereal での読み書きのヘルパー。cereal / iostream の例外は必ず SerializationException 系に変換して投げる
namespace NanamiEngine::Module::Serialization
{
    namespace Detail
    {
        // NOTE: stream 上に ArchiveT を作って read を呼ぶ。cereal / iostream の例外は DeserializeException にする
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
                // NOTE: ネストした読み込みが既に変換済みの例外を投げた場合はそのまま通す
                throw;
            }
            catch (const std::exception& exception)
            {
                throw Exception::DeserializeException(label, exception.what());
            }
        }

        // NOTE: stream 上に ArchiveT を作って write を呼ぶ。cereal / iostream の例外は SerializeException にする
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

    // NOTE: 開けなければ FileNotFoundException、cereal の失敗は DeserializeException を投げる
    template <class ReadFn>
    void LoadJsonFile(const std::string& filePath, ReadFn&& read)
    {
        std::ifstream ifStream(filePath);
        if (!ifStream.is_open())
            throw Exception::FileNotFoundException(filePath);

        Detail::ReadWith<cereal::JSONInputArchive>(ifStream, filePath, std::forward<ReadFn>(read));
    }

    // NOTE: ファイルが無い(または 0 byte)ときは何もせず false を返す。破損は DeserializeException を投げる
    // NOTE: 0 byte も未作成扱いにするのは cereal が空ストリームで例外を投げるため
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

    // NOTE: 開けない・cereal の失敗は SerializeException を投げる
    template <class WriteFn>
    void SaveJsonFile(const std::string& filePath, WriteFn&& write)
    {
        std::ofstream ofStream(filePath);
        if (!ofStream.is_open())
            throw Exception::SerializeException(filePath, "Failed to open file for writing");

        Detail::WriteWith<cereal::JSONOutputArchive>(ofStream, filePath, std::forward<WriteFn>(write));
    }

    // NOTE: in-memory バイナリ（stringstream など）からの読み込み。label は例外メッセージ用の識別子
    template <class ReadFn>
    void LoadPortableBinary(std::istream& stream, const std::string& label, ReadFn&& read)
    {
        Detail::ReadWith<cereal::PortableBinaryInputArchive>(stream, label, std::forward<ReadFn>(read));
    }
}
