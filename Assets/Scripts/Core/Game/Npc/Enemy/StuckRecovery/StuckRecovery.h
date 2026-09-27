#pragma once
#include <cstdint>
#include <deque>
#include <string>

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"
#include "vec3.hpp"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Component
{
    class RigidBody;
}

namespace NanamiEngine::Module::BlackBoard
{
    class ParameterGroup;
}

namespace GameCore::Npc::Enemy
{
    /**
     * @brief 地形にはまった敵を段階的に脱出させる
     * @details 進めない状態が続いたら開けた方向へ押し出し、それでも抜けなければ最後に自由に動けていた地点へワープする。
     *          BT の Tick の後に権威側だけで呼ぶ (ワープ後の位置は NetworkTransform が同期する)
     */
    class StuckRecovery final
    {
    public:
        void Tick(NanamiEngine::Module::GameObject::IGameObject& self,
                  NanamiEngine::Module::Component::RigidBody& rigidBody,
                  const NanamiEngine::Module::BlackBoard::ParameterGroup* parameters);
        void DrawGui();

    private:
        [[nodiscard]] bool IsMonitoring(const NanamiEngine::Module::BlackBoard::ParameterGroup* parameters) const;
        void ResetAt(const glm::vec3& position);
        void UpdateProgress(const glm::vec3& position, const glm::vec3& velocity, float deltaTime);
        [[nodiscard]] bool IsStrandedAwayFromPlayers(const glm::vec3& position, float deltaTime);
        void StartNudge(NanamiEngine::Module::GameObject::IGameObject& self);
        void Warp(NanamiEngine::Module::GameObject::IGameObject& self,
                  NanamiEngine::Module::Component::RigidBody& rigidBody,
                  const NanamiEngine::Module::BlackBoard::ParameterGroup* parameters,
                  const char* reason);

        /** この水平速度未満しか指示されていないフレームは判定しない */
        [[serialize(0)]] float minMoveSpeed_          = 5.0f;
        /** 指示速度に対して実際に進んだ割合がこれ未満なら進めていないとみなす */
        [[serialize(0)]] float progressRate_          = 0.25f;
        /** 進めない時間の合計がこの秒数を超えるごとに押し出す */
        [[serialize(0)]] float nudgeAfter_secs_       = 1.0f;
        [[serialize(0)]] int   maxNudges_             = 3;
        [[serialize(0)]] float nudge_secs_            = 0.6f;
        [[serialize(0)]] float nudgeSpeed_            = 40.0f;
        [[serialize(0)]] float nudgeLiftSpeed_        = 15.0f;
        [[serialize(0)]] float castHeight_            = 25.0f;
        [[serialize(0)]] float castRadius_            = 15.0f;
        [[serialize(0)]] float castDistance_          = 40.0f;
        [[serialize(0)]] float wallMaxNormalY_        = 0.6f;
        [[serialize(0)]] float warpAfter_secs_        = 4.0f;
        /** 最寄りのプレイヤーがこれより遠いのに strandedWindow_secs_ の間ほぼ動いていなければ取り残されたとみなす */
        [[serialize(0)]] float strandedDistance_      = 150.0f;
        [[serialize(0)]] float strandedWindow_secs_   = 8.0f;
        [[serialize(0)]] float strandedTolerance_     = 10.0f;
        /** 最後に自由に動けていた地点からこれ以上落ちたらワープする */
        [[serialize(0)]] float fallDropHeight_        = 60.0f;
        [[serialize(0)]] float recordInterval_secs_   = 0.5f;
        [[serialize(0)]] int   maxRecords_            = 12;
        /** 現在地からこれより近い記録はワープ先にしない */
        [[serialize(0)]] float minWarpDistance_       = 30.0f;
        [[serialize(0)]] float warpLift_              = 5.0f;
        [[serialize(0)]] std::string stateKeyName_      = "State";
        [[serialize(0)]] int         activeStateValue_  = 1;
        [[serialize(0)]] std::string stuckStateKeyName_ = "StuckState";
        [[serialize(0)]] std::string actKeyName_        = "Act";
        [[serialize(0)]] int         readyActValue_     = 0;

        bool                  isInitialized_       = false;
        glm::vec3             home_                = glm::vec3(0.0f);
        glm::vec3             prevPosition_        = glm::vec3(0.0f);
        glm::vec3             prevVelocity_        = glm::vec3(0.0f);
        float                 stuck_secs_          = 0.0f;
        int                   nudgeCount_          = 0;
        float                 nudgeRemain_secs_    = 0.0f;
        glm::vec3             nudgeDirection_      = glm::vec3(0.0f);
        float                 recordTimer_secs_    = 0.0f;
        std::deque<glm::vec3> goodPositions_;
        glm::vec3             strandedAnchor_      = glm::vec3(0.0f);
        float                 stranded_secs_       = 0.0f;

#pragma region Serialization Function
    public:
        template<class Archive>
        void serialize(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(minMoveSpeed_));
            archive(CEREAL_NVP(progressRate_));
            archive(CEREAL_NVP(nudgeAfter_secs_));
            archive(CEREAL_NVP(maxNudges_));
            archive(CEREAL_NVP(nudge_secs_));
            archive(CEREAL_NVP(nudgeSpeed_));
            archive(CEREAL_NVP(nudgeLiftSpeed_));
            archive(CEREAL_NVP(castHeight_));
            archive(CEREAL_NVP(castRadius_));
            archive(CEREAL_NVP(castDistance_));
            archive(CEREAL_NVP(wallMaxNormalY_));
            archive(CEREAL_NVP(warpAfter_secs_));
            archive(CEREAL_NVP(strandedDistance_));
            archive(CEREAL_NVP(strandedWindow_secs_));
            archive(CEREAL_NVP(strandedTolerance_));
            archive(CEREAL_NVP(fallDropHeight_));
            archive(CEREAL_NVP(recordInterval_secs_));
            archive(CEREAL_NVP(maxRecords_));
            archive(CEREAL_NVP(minWarpDistance_));
            archive(CEREAL_NVP(warpLift_));
            archive(CEREAL_NVP(stateKeyName_));
            archive(CEREAL_NVP(activeStateValue_));
            archive(CEREAL_NVP(stuckStateKeyName_));
            archive(CEREAL_NVP(actKeyName_));
            archive(CEREAL_NVP(readyActValue_));
        }
#pragma endregion
    };
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::StuckRecovery, 0);
#pragma endregion
