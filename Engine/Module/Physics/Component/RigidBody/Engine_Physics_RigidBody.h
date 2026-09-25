#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <optional>
#include <utility>

#include "vec3.hpp"
#include "../../../Component/ComponentBase.h"
#include "../../../LifeCycleCallback/Awake/IAwakable.h"
#include "../../../LifeCycleCallback/BeginPhysics/IBeginPhysics.h"
#include "../../../LifeCycleCallback/UpdatedPhysics/IEndPhysics.h"
#include "../Collider/Engine_Physics_Constraints.h"
#include "../../MotionType/Engine_Physics_MotionType.h"

namespace NanamiEngine::Module::Component
{
    /**
     * @brief 自分と子孫の Collider(Sensor 以外)を1つの Body にまとめて動かす
     * @note 子孫に別の RigidBody があれば、そこから下はその RigidBody の持ち物になる
     */
    class NANAMI_API RigidBody final : public ComponentBase,
                            public LifeCycleCallback::IAwakable,
                            public LifeCycleCallback::IBeginPhysics,
                            public LifeCycleCallback::IEndPhysics
    {
    public:
        [[nodiscard]] Physics::MotionType  MotionType       () const { return motionType_;  }
        [[nodiscard]] float                Mass             () const { return mass_;        }
        [[nodiscard]] bool                 IsGravity        () const { return isGravity_;   }
        [[nodiscard]] Physics::Constraints FreezeConstraints() const { return constraints_; }
        [[nodiscard]] bool                 IsPartOfParent   () const { return isPartOfParent_; }
        void SetMotionType   (Physics::MotionType  motionType );
        void SetGravity      (bool                 isGravity  );
        void SetFreezePhysics(Physics::Constraints constraints);
        void SetPartOfParent (bool                 isPartOfParent);

        [[nodiscard]] glm::vec3 LinearVelocity() const;
        void SetLinearVelocity(const glm::vec3& velocity) const;
        void AddLinearVelocity(const glm::vec3& velocity) const;
        [[nodiscard]] glm::vec3 AngularVelocity() const;
        void SetAngularVelocity(const glm::vec3& angularVelocity) const;
        void AddTorque(const glm::vec3& torque) const;
        [[nodiscard]] std::optional<glm::vec3> CenterOfMassPosition() const;
        [[nodiscard]] std::optional<std::pair<glm::vec3, glm::vec3>> WorldBounds() const;

    private:
        void OnAwake         () override;
        void OnBeginPhysics  () override;
        void OnUpdatedPhysics() override;
        void OnDestroy       () override;
        void OnDrawGui       () override;

        [[serialize(0)]] Physics::MotionType  motionType_  = Physics::MotionType::Dynamic;
        [[serialize(0)]] float                mass_        = 1.0f;
        [[serialize(0)]] bool                 isGravity_   = true;
        [[serialize(0)]] Physics::Constraints constraints_ = Physics::Constraints::None;
        // 手足のように親の RigidBody の一部として動く Body。親と衝突せず、当たり判定の持ち主は親になる(Physics::FindBodyOwner)
        [[serialize(1)]] bool                 isPartOfParent_ = false;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(motionType_));
            archive(CEREAL_NVP(mass_));
            archive(CEREAL_NVP(isGravity_));
            archive(CEREAL_NVP(constraints_));
            archive(CEREAL_NVP(isPartOfParent_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(motionType_));
            archive(CEREAL_NVP(mass_));
            archive(CEREAL_NVP(isGravity_));
            archive(CEREAL_NVP(constraints_));
            if (version >= 1) archive(CEREAL_NVP(isPartOfParent_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::RigidBody, 1);
