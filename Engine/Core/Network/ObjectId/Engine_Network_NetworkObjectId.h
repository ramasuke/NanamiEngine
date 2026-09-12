#pragma once
#include <cstdint>
#include <compare>

#include "../cereal/include/cereal/cereal.hpp"
#include "../PlayerId/PlayerId.h"

namespace NanamiEngine::Core::Network
{
    /**
     * ネットワーク上で共有されるオブジェクトの識別子
     * フォーマット: bit16-23 = Spawn したピアの PlayerId(8bit), 下位16bit = そのピア内のオブジェクトインデックス
     * NOTE: 上位バイトは「誰が Spawn したか」であり「今の所有者」ではない。
     *       所有者(権威)は INetworkObjectInstanceRegistry::OwnerOf() / NetworkRunnerBase::IsLocallyOwned() で判定すること
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
        
        /** このオブジェクトを Spawn したピアの PlayerId(ID の bit16-23)。現在の所有者ではないので権威判定には使わないこと */
        [[nodiscard]] PlayerId SpawnerId() const;
        
        void OnDrawGui();

    private:
        uint32_t networkObjectId_ = 0;
    };
}
