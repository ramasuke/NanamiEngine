#pragma once
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

#include "../AnimationParameter.h"
#include "../IAnimationParameter.h"

namespace NanamiEngine::Module::BlackBoard
{
    class ParameterGroup final
    {
    public:
        ///Tで指定した型のAnimationParameterを取得
        template <typename T>
        std::shared_ptr<AnimationTree::AnimationParameter<T>> Catch(const std::string& name) const;

        void OnDrawGui();

        template <typename Archive>
        void save(Archive& archive) const
        {
            std::size_t count = conditionParameters_.size();
            archive(count);

            for (const auto& param : conditionParameters_)
            {
                archive(param);
            }
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            std::size_t count = 0;
            archive(count);

            conditionParameters_.clear();
            conditionParameters_.reserve(count);

            for (std::size_t i = 0; i < count; ++i)
            {
                std::shared_ptr<AnimationTree::IAnimationParameter> param;
                archive(param);
                conditionParameters_.push_back(param);
            }
        }

    private:
        std::vector<std::shared_ptr<AnimationTree::IAnimationParameter>> conditionParameters_;
    };

    template <typename T>
    std::shared_ptr<AnimationTree::AnimationParameter<T>> ParameterGroup::Catch(const std::string& name) const
    {
        const auto it = std::ranges::find_if(conditionParameters_,
                                             [&](const std::shared_ptr<AnimationTree::IAnimationParameter>& param)
                                             {
                                                 return param && param->Name() == name;
                                             });

        if (it != conditionParameters_.end())
        {
            if (auto casted = std::dynamic_pointer_cast<AnimationTree::AnimationParameter<T>>(*it))
                return casted;
        }

        return nullptr;
    }
}