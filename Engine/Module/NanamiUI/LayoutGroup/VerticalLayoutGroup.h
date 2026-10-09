#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "vec2.hpp"
#include "LayoutCrossAlign.h"
#include "../../Component/ComponentBase.h"
#include "../../LifeCycleCallback/LateUpdate/LateUpdate.h"

namespace NanamiEngine::Module::NanamiUi
{
    // NOTE: 子をY軸方向に一列に並べる
    class NANAMI_API VerticalLayoutGroup final : public Component::ComponentBase,
                                                 public LifeCycleCallback::ILateUpdatable
    {
    private:
        void OnLateUpdate() override;

        [[serialize(0)]] glm::vec2 cellSize_ = { 100.0f, 100.0f };
        [[serialize(0)]] float spacing_ = 0.0f;
        [[serialize(0)]] LayoutCrossAlign childAlignment_ = LayoutCrossAlign::Center;
        [[serialize(0)]] bool reverseArrangement_ = false;
        [[serialize(1)]] bool ignoreDisabledChildren_ = false;
        [[serialize(1)]] bool stackUpward_ = false;
        [[serialize(2)]] bool centerOnOrigin_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::ILateUpdatable>(this));
            archive(CEREAL_NVP(cellSize_));
            archive(CEREAL_NVP(spacing_));
            archive(CEREAL_NVP(childAlignment_));
            archive(CEREAL_NVP(reverseArrangement_));
            archive(CEREAL_NVP(ignoreDisabledChildren_));
            archive(CEREAL_NVP(stackUpward_));
            archive(CEREAL_NVP(centerOnOrigin_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::ILateUpdatable>(this));
            if (version >= 0) archive(CEREAL_NVP(cellSize_));
            if (version >= 0) archive(CEREAL_NVP(spacing_));
            if (version >= 0) archive(CEREAL_NVP(childAlignment_));
            if (version >= 0) archive(CEREAL_NVP(reverseArrangement_));
            if (version >= 1) archive(CEREAL_NVP(ignoreDisabledChildren_));
            if (version >= 1) archive(CEREAL_NVP(stackUpward_));
            if (version >= 2) archive(CEREAL_NVP(centerOnOrigin_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::NanamiUi::VerticalLayoutGroup, 2);
