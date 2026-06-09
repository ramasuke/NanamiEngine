#pragma once
#include <cstdint>
#include <compare>

#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Core::Network
{
    /**
     * 接続しているクライアント番号
     */
    struct PlayerId final
    {
        explicit PlayerId(int playerId = -1);
        static PlayerId Invalid();
        
        auto operator<=>(const PlayerId&) const = default;

        template<typename Archive>
        void serialize(Archive& archive)
        {
            archive(CEREAL_NVP(playerId_));
        }
        
    private:
        int8_t playerId_ = -1;
    };
}
