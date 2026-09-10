#pragma once
#include "vec2.hpp"
#include "LayoutCrossAlign.h"
#include "../../Component/ComponentBase.h"
#include "../../LifeCycleCallback/LateUpdate/LateUpdate.h"

namespace NanamiEngine::Module::NanamiUi
{
    // 子GameObjectをX軸方向に一列に並べる。子の実際の見た目サイズは知らないため、
    // cellSize_を「各子が占めるものとみなす仮想サイズ」として並べる(実サイズの変更は行わない)。
    class HorizontalLayoutGroup final : public Component::ComponentBase,
                                        public LifeCycleCallback::ILateUpdatable
    {
    private:
        void OnLateUpdate() override;

        [[serialize(0)]] glm::vec2 cellSize_ = { 100.0f, 100.0f };
        [[serialize(0)]] float spacing_ = 0.0f;
        [[serialize(0)]] LayoutCrossAlign childAlignment_ = LayoutCrossAlign::Center;
        [[serialize(0)]] bool reverseArrangement_ = false;

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
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::ILateUpdatable>(this));
            if (version >= 0) archive(CEREAL_NVP(cellSize_));
            if (version >= 0) archive(CEREAL_NVP(spacing_));
            if (version >= 0) archive(CEREAL_NVP(childAlignment_));
            if (version >= 0) archive(CEREAL_NVP(reverseArrangement_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::NanamiUi::HorizontalLayoutGroup, 0)
