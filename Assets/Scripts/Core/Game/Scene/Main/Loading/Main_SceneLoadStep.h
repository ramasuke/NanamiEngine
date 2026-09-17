#pragma once
#include <cstdint>

namespace GameCore::Scene::Main
{
    /**
     * @brief ロード画面に出す進捗の段階。
     *        実測できるのは Deserializing だけで、残りは経過時間から飽和カーブで埋める
     */
    enum class SceneLoadStep : std::uint8_t
    {
        Idle,
        Deserializing,
        Warmup,
        Connecting,
        Spawning,
        Completed,
        Failed,
    };
}
