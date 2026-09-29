#pragma once
#include <span>
#include <vector>

#include "Engine/Core/Api/NanamiApi.h"
#include "vec3.hpp"
#include "../ComponentBase.h"
#include "../../../../Libs/LibCore/DxLib/BlendMode.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"
#include "../../LifeCycleCallback/Update/IUpdatable.h"
#include "../../LifeCycleCallback/UserInterfaceRenderable/IUserInterfaceRenderable.h"

namespace NanamiEngine::Module::Component
{
    /**
     * @brief 折れ線に沿ってスプライトをビルボードで並べ、始点から終点へ流す（道しるべの蛍など）。
     * 折れ線と表示度は使う側が SetPath / SetVisibility で毎フレーム渡す
     */
    class NANAMI_API PathSpriteTrailRenderer final : public ComponentBase,
                                          public LifeCycleCallback::IUpdatable,
                                          public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        /** @brief ワールド座標の折れ線（始点から流れる） */
        void SetPath(std::span<const glm::vec3> points);
        void ClearPath();
        /** @brief 0〜1。全体のアルファに掛ける */
        void SetVisibility(float visibility);
        [[nodiscard]] float PathLength() const { return pathLength_; }

    private:
        void OnUpdate() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        [[serialize(0)]] int                      renderOrder_ = 0;
        [[serialize(0)]] FIELD(Asset::SpriteFile) sprite_;
        [[serialize(0)]] LibCore::Dxlib::BlendMode blendMode_ = LibCore::Dxlib::BlendMode::Add;
        /** @brief 始点からこの長さまで並べる */
        [[serialize(0)]] float                    trailLength_ = 300.0f;
        [[serialize(0)]] float                    startOffset_ = 12.0f;
        [[serialize(0)]] float                    spacing_ = 14.0f;
        [[serialize(0)]] float                    lift_ = 7.0f;
        [[serialize(0)]] float                    size_ = 3.2f;
        [[serialize(0)]] float                    flowSpeed_ = 16.0f;
        [[serialize(0)]] float                    driftAmplitude_ = 1.4f;
        [[serialize(0)]] float                    driftPeriod_secs_ = 2.4f;
        [[serialize(0)]] float                    alphaRate_ = 0.9f;

        std::vector<glm::vec3> path_;
        float                  pathLength_ = 0.0f;
        float                  visibility_ = 1.0f;
        float                  time_secs_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(sprite_));
            archive(CEREAL_NVP(blendMode_));
            archive(CEREAL_NVP(trailLength_));
            archive(CEREAL_NVP(startOffset_));
            archive(CEREAL_NVP(spacing_));
            archive(CEREAL_NVP(lift_));
            archive(CEREAL_NVP(size_));
            archive(CEREAL_NVP(flowSpeed_));
            archive(CEREAL_NVP(driftAmplitude_));
            archive(CEREAL_NVP(driftPeriod_secs_));
            archive(CEREAL_NVP(alphaRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(sprite_));
            if (version >= 0) archive(CEREAL_NVP(blendMode_));
            if (version >= 0) archive(CEREAL_NVP(trailLength_));
            if (version >= 0) archive(CEREAL_NVP(startOffset_));
            if (version >= 0) archive(CEREAL_NVP(spacing_));
            if (version >= 0) archive(CEREAL_NVP(lift_));
            if (version >= 0) archive(CEREAL_NVP(size_));
            if (version >= 0) archive(CEREAL_NVP(flowSpeed_));
            if (version >= 0) archive(CEREAL_NVP(driftAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(driftPeriod_secs_));
            if (version >= 0) archive(CEREAL_NVP(alphaRate_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::PathSpriteTrailRenderer, 0);
