#pragma once
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Network/Object/Component/Engine_Network_NetworkComponent.h"
#include "../../../../../Data/EnemyBehaviour/Data_EnemyBehaviourFile.h"
#include "../../PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"
#include "../../PlayerAvatar/LockOnTarget/ILockOnTarget.h"
#include "Status/EnemyStatus.h"

namespace GameCore::Npc
{
    class EnemyBase : public Network::NetworkComponent,
                      public LifeCycleCallback::IAwakable,
                      public LifeCycleCallback::IUpdatable,
                      public PlayerAvatar::ITakablePlayerAttack,
                      public PlayerAvatar::ILockOnTarget
    {
    public:
        explicit EnemyBase();
        virtual ~EnemyBase() override;
        [[nodiscard]] virtual std::shared_ptr<Enemy::BehaviourTree> BehaviourTree() const { return behaviour_; }

    protected:
        virtual void DoAwake() { }
        virtual void DoUpdate() { }
        [[nodiscard]] virtual SyncParam<Enemy::EnemyStatus>& NetworkStatus() { return currentStatus_; }
        
    private:
        void OnAwake () override;
        void OnUpdate() override;
        void OnTakeDamage(std::unique_ptr<IDamage> context) override;

        bool isNetworkSyncStatus_ = false;
        SyncParam<Enemy::EnemyStatus> currentStatus_ = CreateSyncParameter(Enemy::EnemyStatus());
        FIELD(Asset::EnemyBehaviourFile) behaviourData_;
        std::shared_ptr<Enemy::BehaviourTree> behaviour_;
        std::shared_ptr<std::queue<std::unique_ptr<IDamage>>> onDamagedStack_;
        bool hasNetworkBehaviourTree_ = false;
        
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
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version <= 3) archive(cereal::base_class<ComponentBase>(this));
            else if (version >= 4)archive(cereal::base_class<NetworkComponent>(this));
            
            if (version >= 1) archive(CEREAL_NVP(behaviourData_));
            if (version >= 4) archive(CEREAL_NVP(currentStatus_));
            if (version >= 4) archive(CEREAL_NVP(isNetworkSyncStatus_));
        }
#pragma endregion
    };
};
ENGINE_REGISTER_COMPONENT(GameCore::Npc::EnemyBase, 4)
