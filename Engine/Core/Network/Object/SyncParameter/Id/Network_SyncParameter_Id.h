#pragma once
#include "Engine/Core/Api/NanamiApi.h"

namespace NanamiEngine::Core::Network
{
    // NOTE: 同期する値のネットワーク共通 ID。上位 32bit = NetworkObjectId、下位 32bit = オブジェクト内の添字
    struct NANAMI_API ParameterId final
    {
        explicit ParameterId(uint64_t id = UINT64_MAX);

        auto operator<=>(const ParameterId&) const = default;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(id_);
        }

        [[nodiscard]] uint64_t Value() const { return id_; }
        [[nodiscard]] std::string ToString() const;

    private:
        uint64_t id_ = UINT64_MAX;
    };
}

template <>
struct std::hash<NanamiEngine::Core::Network::ParameterId>
{
    size_t operator()(const NanamiEngine::Core::Network::ParameterId& id) const noexcept
    {
        return std::hash<uint64_t>{}(id.Value());
    }
};