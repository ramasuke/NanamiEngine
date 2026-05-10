#pragma once
#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Data/EnemyBehaviour/Data_EnemyBehaviourFile.h"
#include "../../PlayerAvatar/ITakablePlayerAttack/ITakablePlayerAttack.h"
#include "Status/EnemyStatus.h"

namespace GameCore::Npc
{
    class EnemyBase : public Component::ComponentBase,
                      public LifeCycleCallback::IAwakable,
                      public LifeCycleCallback::IUpdatable,
                      public PlayerAvatar::ITakablePlayerAttack
    {
    public:
        explicit EnemyBase();
        virtual ~EnemyBase() override;

    protected:
        virtual void DoAwake() { }
        virtual void DoUpdate() { }
        virtual Enemy::EnemyStatus& Status() const { return *status_; }
        
    private:
        void OnAwake () override;
        void OnUpdate() override;
        void OnTakeDamage(std::unique_ptr<IDamage> context) override;

        std::unique_ptr<Enemy::EnemyStatus> status_;
        FIELD(Asset::EnemyBehaviourFile) behaviourData_;
        std::shared_ptr<Enemy::BehaviourTree> behaviour_;
        std::shared_ptr<std::queue<std::unique_ptr<IDamage>>> onDamagedStack_;
        
#pragma region Serialization Function
    public:
        void BasedOnDrawgui() override;
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(behaviourData_));
            archive(CEREAL_NVP(status_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(behaviourData_));
            if (version >= 2) archive(CEREAL_NVP(status_));
        }
#pragma endregion
    };
};
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Npc::EnemyBase, 2);
CEREAL_REGISTER_TYPE(GameCore::Npc::EnemyBase);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, GameCore::Npc::EnemyBase);
#pragma endregion
