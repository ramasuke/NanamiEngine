#pragma once
#include "../BoneSyncBase.h"
#include "../../../../../Core/Object/Field/Field.h"
#include "../../../../GameObject/Interface/IGameObject.h"

namespace NanamiEngine::Module::Bone
{
    class TransformSync final : public BoneSyncBase
    {
        void ApplyBonePose(const BonePose& bonePose) override;

        [[serialize(0)]] FIELD(GameObject::IGameObject) target_;
        [[serialize(0)]] bool syncPosition_ = true;
        [[serialize(0)]] bool syncRotation_ = true;

#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<BoneSyncBase>(this));
            archive(CEREAL_NVP(target_));
            archive(CEREAL_NVP(syncPosition_));
            archive(CEREAL_NVP(syncRotation_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<BoneSyncBase>(this));
            archive(CEREAL_NVP(target_));
            archive(CEREAL_NVP(syncPosition_));
            archive(CEREAL_NVP(syncRotation_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Bone::TransformSync, 0);
#pragma endregion
