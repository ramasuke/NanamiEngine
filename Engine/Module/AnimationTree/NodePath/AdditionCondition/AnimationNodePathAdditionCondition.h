#pragma once
#include "Interface/IAnimationNodePathAdditionCondition.h"

namespace NanamiEngine::Module::AnimationTree
{
    template<typename T>
    class AnimationNodePathAdditionCondition final : public IAnimationNodePathAdditionCondition
    {
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<IAnimationNodePathAdditionCondition>(this));
            archive(CEREAL_NVP(name_));
            archive(CEREAL_NVP(equalValue_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<IAnimationNodePathAdditionCondition>(this));
            if (version >= 0) archive(CEREAL_NVP(name_));
            if (version >= 0) archive(CEREAL_NVP(equalValue_));
        }
        
        [[nodiscard]] bool Check(BlackBoard::ParameterGroup& additionParameterGroup) override;
        void OnDrawGui() override;

    private:
        std::string name_;
        T equalValue_;
    };

    template <typename T>
    bool AnimationNodePathAdditionCondition<T>::Check(BlackBoard::ParameterGroup& additionParameterGroup)
    {
        auto checkParam = additionParameterGroup.Catch<T>(name_);
        if (!checkParam)
            return false;
        
        return checkParam->Get() == equalValue_;
    }

    template <typename T>
    void AnimationNodePathAdditionCondition<T>::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("name_", name_);
        LibCore::ImGuiHelper::OnDrawInputField("equalValue_", equalValue_);
    }
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<bool>, 0);
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<int>, 0);
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<float>, 0);

CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<bool>);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<int>);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<float>);

CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition,
    NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<bool>
);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition,
    NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<int>
);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationNodePathAdditionCondition,
    NanamiEngine::Module::AnimationTree::AnimationNodePathAdditionCondition<float>
);
