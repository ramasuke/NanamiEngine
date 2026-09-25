#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <cstdint>
#include <compare>

#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Core::Network
{
    /**
     * 接続しているクライアント番号
     */
    struct NANAMI_API PlayerId final
    {
        explicit PlayerId(int playerId = -1);
        static PlayerId Invalid();
        
        auto operator<=>(const PlayerId&) const = default;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(playerId_);
        }

        [[nodiscard]] int8_t Value() const { return playerId_; }
        [[nodiscard]] std::string ToString() const;

    private:
        int8_t playerId_ = -1;
    };
}
