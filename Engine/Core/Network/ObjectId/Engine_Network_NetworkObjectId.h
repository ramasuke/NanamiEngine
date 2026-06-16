#pragma once
#include <cstdint>
#include <compare>

#include "../cereal/include/cereal/cereal.hpp"
#include "../PlayerId/PlayerId.h"

namespace NanamiEngine::Core::Network
{
    /**
     * ネットワーク上で共有されるオブジェクトの識別子
     * フォーマット: 上位16bit = PlayerId, 下位16bit = オブジェクトインデックス
     */
    struct NetworkObjectId final
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
        
        /** このオブジェクトIDが指定したPlayerIdに属するか判別する */
        [[nodiscard]] bool IsOwnerBy(PlayerId playerId) const;
        
        void OnDrawGui();

    private:
        uint32_t networkObjectId_ = 0;
    };
}
