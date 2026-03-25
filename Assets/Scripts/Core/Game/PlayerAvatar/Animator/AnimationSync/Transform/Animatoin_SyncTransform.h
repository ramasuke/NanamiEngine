#pragma once
#include "../IAnimationSync.h"
#include "../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"

namespace NanamiEngine::Module::AnimationTree
{
    class TransformSync final : public AnimationSyncBase
    {
        void UpdateSync(const int& animationModelHandle) override;
        
        [[serialize(0)]] FIELD(GameObject::IGameObject) syncChildTransform_;
        
#pragma region Serialization Function
    public:
        void DoDrawGui() override;
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<AnimationSyncBase>(this));
            archive(CEREAL_NVP(syncChildTransform_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<AnimationSyncBase>(this));
            archive(CEREAL_NVP(syncChildTransform_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::TransformSync, 4);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::TransformSync);
CEREAL_REGISTER_POLYMORPHIC_RELATION(AnimationTree::AnimationSyncBase, NanamiEngine::Module::AnimationTree::TransformSync);
#pragma endregion