#pragma once
#include <stdexcept>
#include <string>
#include <utility>

// エンジン共通の例外階層。
// すべて std::runtime_error（= std::exception）派生なので、
//   catch (const Exception::DeserializeException&)   ... 種類を絞って捕捉
//   catch (const Exception::NanamiException&)        ... エンジン由来の例外を一括捕捉
//   catch (const std::exception&)                    ... 標準例外と一緒に捕捉
// のいずれでも受けられる。
// cereal / iostream の生例外は Serialization ヘルパー（Engine/Module/Serialization）で必ずこの型に変換する。
namespace NanamiEngine::Module::Exception
{
    /** エンジンが投げる全例外の基底 */
    class NanamiException : public std::runtime_error
    {
    public:
        explicit NanamiException(const std::string& message)
            : std::runtime_error(message)
        {
        }
    };

    /** cereal でのシリアライズ／デシリアライズ失敗の基底。対象ファイルパス（in-memory の場合は識別ラベル）を保持する */
    class SerializationException : public NanamiException
    {
    public:
        SerializationException(std::string filePath, const std::string& message)
            : NanamiException(message + " [" + filePath + "]")
            , filePath_(std::move(filePath))
        {
        }

        [[nodiscard]] const std::string& FilePath() const noexcept { return filePath_; }

    private:
        std::string filePath_;
    };

    /** ファイルを開けなかった（存在しない・権限なし） */
    class FileNotFoundException final : public SerializationException
    {
    public:
        explicit FileNotFoundException(const std::string& filePath)
            : SerializationException(filePath, "File not found")
        {
        }
    };

    /** ファイルは開けたが cereal が読み込みに失敗した
     *  （JSON 破損・未登録の polymorphic 型・バージョン不一致・型不一致など）。cereal 側の what() を InnerMessage() で参照できる */
    class DeserializeException final : public SerializationException
    {
    public:
        DeserializeException(const std::string& filePath, std::string innerMessage)
            : SerializationException(filePath, "Deserialize failed: " + innerMessage)
            , innerMessage_(std::move(innerMessage))
        {
        }

        [[nodiscard]] const std::string& InnerMessage() const noexcept { return innerMessage_; }

    private:
        std::string innerMessage_;
    };

    /** 書き込みに失敗した（出力ファイルを開けない・rename 失敗・cereal 失敗） */
    class SerializeException final : public SerializationException
    {
    public:
        SerializeException(const std::string& filePath, std::string innerMessage)
            : SerializationException(filePath, "Serialize failed: " + innerMessage)
            , innerMessage_(std::move(innerMessage))
        {
        }

        [[nodiscard]] const std::string& InnerMessage() const noexcept { return innerMessage_; }

    private:
        std::string innerMessage_;
    };

    /** ネットワークパケットのデシリアライズ失敗（範囲外読み取り・cereal 失敗）。
     *  外部からの入力なので、ファイル系とは別に扱えるよう基底から直接派生させる */
    class PacketDeserializeException final : public NanamiException
    {
    public:
        explicit PacketDeserializeException(const std::string& message)
            : NanamiException("Packet deserialize failed: " + message)
        {
        }
    };
}
