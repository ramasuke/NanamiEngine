#pragma once
#include "../ComponentBase.h"
#include "../../../../Assets/Scripts/Core/Game/PlayerAvatar/Animator/AnimationSync/Transform/Animatoin_SyncTransform.h"
#include "../../AnimationTree/Node/IAnimationNode.h"
#include "../../Asset/AnimationTree/AnimationTreeFile.h"

namespace NanamiEngine::Module::Component
{
    class Animator final : public ComponentBase,
                           public LifeCycleCallback::IAwakable,
                           public LifeCycleCallback::IUpdatable,
                           public LifeCycleCallback::ILateUpdatable
    {
    public:
        void OnAwake () override;
        void OnUpdate() override {}
        void OnLateUpdate() override;
        template<typename T>
        AnimationTree::AnimationParameter<T>& Param(std::string paramName);

    private:
        void InitAnimationTree();
        
        [[serialize(0)]] FIELD(Asset::AnimationTreeFile) animationTreeFile_;
        [[serialize(2)]] std::vector<std::unique_ptr<AnimationTree::AnimationSyncBase>> animationSyncs_;
        std::shared_ptr<AnimationTree::AnimationTree> animationTree_ = nullptr;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            //archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
            archive(CEREAL_NVP(animationTreeFile_));
                    
            // 個別保存
            const std::size_t count = animationSyncs_.size();
            archive(cereal::make_nvp("animationSyncCount", count));
            
            for (const auto& sync : animationSyncs_)
            {
                archive(cereal::make_nvp("animationSync", sync));
            }
        }
    
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IAwakable>(this));
            if (version >= 0) archive(CEREAL_NVP(animationTreeFile_));
            if (version >= 2)
            {
                std::size_t count = 0;
                archive(cereal::make_nvp("animationSyncCount", count));
        
                animationSyncs_.clear();
                animationSyncs_.reserve(count);
        
                for (std::size_t i = 0; i < count; ++i)
                {
                    std::unique_ptr<AnimationTree::AnimationSyncBase> sync;
                    archive(cereal::make_nvp("animationSync", sync));
                    animationSyncs_.push_back(std::move(sync));
                }
            }
        }
#pragma endregion
};

    template <typename T>
    AnimationTree::AnimationParameter<T>& Animator::Param(const std::string paramName)
    {
        if (!animationTree_)
            InitAnimationTree();
        
        return *animationTree_->Param().Catch<T>(paramName);
    }
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::Animator, 3);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::Animator);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::Component::Animator);
#pragma endregion