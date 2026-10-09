#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <compare>

#include "../cereal/include/cereal/cereal.hpp"
#include "../PlayerId/PlayerId.h"

namespace NanamiEngine::Core::Network
{
    // NOTE: ネットワーク上で共有されるオブジェクトの識別子 (bit16-23 = 採番したピア, 下位16bit = ピア内インデックス)
    // NOTE: 上位バイトは採番の衝突避けだけで、所有者を表さない
    struct NANAMI_API NetworkObjectId final
    {
        explicit NetworkObjectId(uint32_t networkObjectId = 0);
        static NetworkObjectId Invalid();

        auto operator<=>(const NetworkObjectId&) const = default;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(CEREAL_NVP(networkObjectId_));
        }

        [[nodiscard]] uint32_t Value() const { return networkObjectId_; }
        [[nodiscard]] std::string ToString() const;

        void OnDrawGui();

    private:
        uint32_t networkObjectId_ = 0;
    };
}
