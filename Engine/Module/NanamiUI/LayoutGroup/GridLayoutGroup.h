#pragma once
#include <array>
#include <string_view>
#include "vec2.hpp"
#include "../../Component/ComponentBase.h"
#include "../../LifeCycleCallback/LateUpdate/LateUpdate.h"

namespace NanamiEngine::Module::NanamiUi
{
    // 列数/行数のどちらを固定して折り返すか。UnityのFlexibleに相当する
    // 「利用可能幅から自動算出」は、このエンジンにRectTransform(幅の概念)が無いため未対応。
    enum class GridConstraint : int
    {
        FixedColumnCount = 0,
        FixedRowCount = 1,
    };

    constexpr std::array GRID_CONSTRAINTS
    {
        GridConstraint::FixedColumnCount,
        GridConstraint::FixedRowCount,
    };

    constexpr std::string_view ToString(const GridConstraint constraint)
    {
        switch (constraint)
        {
        case GridConstraint::FixedColumnCount: return "FixedColumnCount";
        case GridConstraint::FixedRowCount:    return "FixedRowCount";
        }
        return "Unknown";
    }

    // グリッドがどの角を起点(1番目の子の位置)にして広がっていくか。
    enum class GridStartCorner : int
    {
        UpperLeft = 0,
        UpperRight = 1,
        LowerLeft = 2,
        LowerRight = 3,
    };

    constexpr std::array GRID_START_CORNERS
    {
        GridStartCorner::UpperLeft,
        GridStartCorner::UpperRight,
        GridStartCorner::LowerLeft,
        GridStartCorner::LowerRight,
    };

    constexpr std::string_view ToString(const GridStartCorner corner)
    {
        switch (corner)
        {
        case GridStartCorner::UpperLeft:  return "UpperLeft";
        case GridStartCorner::UpperRight: return "UpperRight";
        case GridStartCorner::LowerLeft:  return "LowerLeft";
        case GridStartCorner::LowerRight: return "LowerRight";
        }
        return "Unknown";
    }

    // 子GameObjectを格子状に並べる。子の実際の見た目サイズは知らないため、
    // cellSize_を「各子が占めるものとみなす仮想サイズ」として並べる(実サイズの変更は行わない)。
    class GridLayoutGroup final : public Component::ComponentBase,
                                  public LifeCycleCallback::ILateUpdatable
    {
    private:
        void OnLateUpdate() override;

        [[serialize(0)]] glm::vec2 cellSize_ = { 100.0f, 100.0f };
        [[serialize(0)]] glm::vec2 spacing_ = { 0.0f, 0.0f };
        [[serialize(0)]] GridConstraint constraint_ = GridConstraint::FixedColumnCount;
        [[serialize(0)]] int constraintCount_ = 1;
        [[serialize(0)]] GridStartCorner startCorner_ = GridStartCorner::UpperLeft;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::ILateUpdatable>(this));
            archive(CEREAL_NVP(cellSize_));
            archive(CEREAL_NVP(spacing_));
            archive(CEREAL_NVP(constraint_));
            archive(CEREAL_NVP(constraintCount_));
            archive(CEREAL_NVP(startCorner_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::ILateUpdatable>(this));
            if (version >= 0) archive(CEREAL_NVP(cellSize_));
            if (version >= 0) archive(CEREAL_NVP(spacing_));
            if (version >= 0) archive(CEREAL_NVP(constraint_));
            if (version >= 0) archive(CEREAL_NVP(constraintCount_));
            if (version >= 0) archive(CEREAL_NVP(startCorner_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::NanamiUi::GridLayoutGroup, 0);
