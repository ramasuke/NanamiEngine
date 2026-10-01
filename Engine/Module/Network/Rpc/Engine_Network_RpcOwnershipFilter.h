#pragma once

namespace NanamiEngine::Module::Network
{
    /** 対象NetworkObjectIdの所有者を基準に、ハンドラを呼ぶかどうかを決める */
    enum class RpcOwnershipFilter
    {
        None,        // 所有者判定を行わず、常に呼ぶ(全クライアントへの通知)
        SkipIfOwner, // 自分が所有者の場合は無視する(所有者以外への通知   )
        OnlyIfOwner, // 自分が所有者の場合のみ呼ぶ  (所有者への要求      )
    };
}
