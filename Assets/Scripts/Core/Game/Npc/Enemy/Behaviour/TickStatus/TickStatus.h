#pragma once

namespace GameCore::Npc::Enemy::Behaviour
{
    enum class TickStatus : int
    {
        Failure,   // ノードは失敗した
        Running,   // ノードはまだ処理中
        Success,   // ノードは成功した
        Abort      // 強制中断（即停止）
    };
}
