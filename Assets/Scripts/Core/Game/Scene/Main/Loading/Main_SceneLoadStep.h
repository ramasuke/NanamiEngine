#pragma once
#include <cstdint>

namespace GameCore::Scene::Main
{
    /** @brief ロード画面の進捗段階。NOTE: 実測は Deserializing だけで、残りは経過時間から埋める */
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
