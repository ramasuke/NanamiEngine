#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Packages/R4/R4.h"
#include "PlayerId/PlayerId.h"

namespace NanamiEngine::Core::Network
{
    struct Packet;
}

namespace NanamiEngine::Core::Network
{
    class NANAMI_API IPacketSender
    {
    public:
        virtual ~IPacketSender() = default;
        virtual void Send(const Packet& packet) = 0;
        /** ホストのみ: target の 1 人にだけ送る */
        virtual void SendTo(PlayerId target, const Packet& packet) = 0;
        /** ホストのみ: クライアントが参加して PlayerId を割り当てた直後に通知する */
        virtual R4::Observable<PlayerId> OnConnectPlayer() = 0;
    };
}
