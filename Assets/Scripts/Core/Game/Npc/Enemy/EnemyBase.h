#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Network/Object/Component/Engine_Network_NetworkComponent.h"
#include "../../../../../Data/Drop/Data_DropTable.h"
#include "../../../../../Data/EnemyBehaviour/Data_EnemyBehaviourFile.h"
#include "../../Damage/Flinch/Game_Damage_FlinchPower.h"
#include "../../PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"
#include "../../PlayerAvatar/LockOnTarget/ILockOnTarget.h"
#include "../../PlayerAvatar/LockOnTarget/LockOnPoint.h"
#include "Status/EnemyStatus.h"
#include "Type/EnemyKind.h"

#include <optional>

namespace GameCore::Npc::Enemy
{
    class IShowHealthGaugeProvider;
}

namespace GameCore::Npc
{
    class EnemyBase : public Module::Network::NetworkComponent,
                      public LifeCycleCallback::IAwakable,
                      public LifeCycleCallback::IUpdatable,
                      public PlayerAvatar::ITakablePlayerAttack,
                      public PlayerAvatar::ILockOnTarget
    {
    public:
        explicit EnemyBase();
        virtual ~EnemyBase() override;
        [[nodiscard]] virtual std::shared_ptr<Enemy::BehaviourTree> BehaviourTree() const { return behaviour_; }
        [[nodiscard]] glm::vec3 LockOnPosition() override;
        void NotifyDefeated();
        void ApplyNetworkHealth(int value);

    protected:
        virtual void DoAwake() { }
        virtual void DoUpdate() { }
        [[nodiscard]] virtual SyncParam<Enemy::EnemyStatus>& NetworkStatus() { return currentStatus_; }
        [[nodiscard]] virtual std::optional<Enemy::EnemyKind> RecordKind() const { return std::nullopt; }
        
    private:
        void OnAwake () override;
        void OnUpdate() override;
        void OnTakeDamage(std::unique_ptr<IDamage> context) override;
        void SendHealthIfChanged();

        bool isNetworkSyncStatus_ = false;
        SyncParam<Enemy::EnemyStatus> currentStatus_ = CreateSyncParameter(Enemy::EnemyStatus());
        FIELD(Asset::EnemyBehaviourFile) behaviourData_;
        std::shared_ptr<Enemy::BehaviourTree> behaviour_;
        std::shared_ptr<std::queue<std::unique_ptr<IDamage>>> onDamagedStack_;
        std::optional<Damage::FlinchPower> pendingFlinchPower_;
        bool hasNetworkBehaviourTree_ = false;
        bool isDefeatRecorded_ = false;
        std::optional<int> lastSentHealth_;
        Enemy::IShowHealthGaugeProvider* showHealthGaugeProvider_ = nullptr;
        [[serialize(5)]] FIELD(PlayerAvatar::LockOnPoint) lockOnPoint_;
        [[serialize(6)]] FIELD(Asset::DropTable) dropTable_;

#pragma region Serialization Function
    public:
        void BasedOnDrawgui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            if (version <= 3) archive(cereal::base_class<ComponentBase>(this));
            else if (version >= 4)archive(cereal::base_class<NetworkComponent>(this));
            archive(CEREAL_NVP(behaviourData_));
            archive(CEREAL_NVP(currentStatus_));
            archive(CEREAL_NVP(isNetworkSyncStatus_));
            archive(CEREAL_NVP(lockOnPoint_));
            archive(CEREAL_NVP(dropTable_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version <= 3) archive(cereal::base_class<ComponentBase>(this));
            else if (version >= 4)archive(cereal::base_class<NetworkComponent>(this));
            
            if (version >= 1) archive(CEREAL_NVP(behaviourData_));
            if (version >= 4) archive(CEREAL_NVP(currentStatus_));
            if (version >= 4) archive(CEREAL_NVP(isNetworkSyncStatus_));
            if (version >= 5) archive(CEREAL_NVP(lockOnPoint_));
            if (version >= 6) archive(CEREAL_NVP(dropTable_));
        }
#pragma endregion
    };
};
CEREAL_CLASS_VERSION(GameCore::Npc::EnemyBase, 6);
