#pragma once
#include "vec2.hpp"
#include "LayoutCrossAlign.h"
#include "../../Component/ComponentBase.h"
#include "../../LifeCycleCallback/LateUpdate/LateUpdate.h"

namespace NanamiEngine::Module::NanamiUi
{
    // 子GameObjectをY軸方向に一列に並べる。子の実際の見た目サイズは知らないため、
    // cellSize_を「各子が占めるものとみなす仮想サイズ」として並べる(実サイズの変更は行わない)。
    // LayoutElement を持つ子は、その割合だけ枠を占める
    class VerticalLayoutGroup final : public Component::ComponentBase,
                                      public LifeCycleCallback::ILateUpdatable
    {
    private:
        void OnLateUpdate() override;

        [[serialize(0)]] glm::vec2 cellSize_ = { 100.0f, 100.0f };
        [[serialize(0)]] float spacing_ = 0.0f;
        [[serialize(0)]] LayoutCrossAlign childAlignment_ = LayoutCrossAlign::Center;
        [[serialize(0)]] bool reverseArrangement_ = false;
        // GameObject::IsEnable() は実行時の SetEnable を反映しないため、コンポーネントが全て無効な子を非表示とみなす
        [[serialize(1)]] bool ignoreDisabledChildren_ = false;
        // 原点から上方向へ積む。最初の枠が原点に固定されるので、下端を揃えたいときに使う
        [[serialize(1)]] bool stackUpward_ = false;
        // 並び全体の中心を原点に置く。子の数や LayoutElement の割合が変わると、両端が同じだけ伸び縮みする
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
