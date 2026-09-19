#include "Sha256.h"

#include <array>
#include <fstream>
#include <vector>

#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "Bcrypt.lib")

namespace NanamiEngine::AssetUpdater
{
    namespace
    {
        constexpr std::size_t SHA256_READ_CHUNK_BYTES = 1 << 20;
        constexpr std::size_t SHA256_DIGEST_BYTES     = 32;

        class Sha256Algorithm final
        {
        public:
            Sha256Algorithm()
            {
                if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&handle_, BCRYPT_SHA256_ALGORITHM, nullptr, 0)))
                    handle_ = nullptr;
            }
            ~Sha256Algorithm()
            {
                if (handle_ != nullptr)
                    BCryptCloseAlgorithmProvider(handle_, 0);
            }

            Sha256Algorithm(const Sha256Algorithm&)            = delete;
            Sha256Algorithm& operator=(const Sha256Algorithm&) = delete;

            [[nodiscard]] BCRYPT_ALG_HANDLE Get() const { return handle_; }

        private:
            BCRYPT_ALG_HANDLE handle_ = nullptr;
        };

        class Sha256Hash final
        {
        public:
            explicit Sha256Hash(const BCRYPT_ALG_HANDLE algorithm)
            {
                if (!BCRYPT_SUCCESS(BCryptCreateHash(algorithm, &handle_, nullptr, 0, nullptr, 0, 0)))
                    handle_ = nullptr;
            }
            ~Sha256Hash()
            {
                if (handle_ != nullptr)
                    BCryptDestroyHash(handle_);
            }

            Sha256Hash(const Sha256Hash&)            = delete;
            Sha256Hash& operator=(const Sha256Hash&) = delete;

            [[nodiscard]] BCRYPT_HASH_HANDLE Get() const { return handle_; }

        private:
            BCRYPT_HASH_HANDLE handle_ = nullptr;
        };
    }

    std::optional<std::string> Sha256OfFile(const std::filesystem::path& filePath)
    {
        std::ifstream stream(filePath, std::ios::binary);
        if (!stream)
            return std::nullopt;

        const Sha256Algorithm algorithm;
        if (algorithm.Get() == nullptr)
            return std::nullopt;
        const Sha256Hash hash(algorithm.Get());
        if (hash.Get() == nullptr)
            return std::nullopt;

        std::vector<char> buffer(SHA256_READ_CHUNK_BYTES);
        while (stream)
        {
            stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize read = stream.gcount();
            if (read <= 0)
                break;
            if (!BCRYPT_SUCCESS(BCryptHashData(hash.Get(), reinterpret_cast<PUCHAR>(buffer.data()), static_cast<ULONG>(read), 0)))
                return std::nullopt;
        }
        if (stream.bad())
            return std::nullopt;

        std::array<UCHAR, SHA256_DIGEST_BYTES> digest{};
        if (!BCRYPT_SUCCESS(BCryptFinishHash(hash.Get(), digest.data(), static_cast<ULONG>(digest.size()), 0)))
            return std::nullopt;

        constexpr char HEX_DIGITS[] = "0123456789abcdef";
        std::string text;
        text.reserve(digest.size() * 2);
        for (const UCHAR byte : digest)
        {
            text.push_back(HEX_DIGITS[byte >> 4]);
            text.push_back(HEX_DIGITS[byte & 0x0F]);
        }
        return text;
    }
}
