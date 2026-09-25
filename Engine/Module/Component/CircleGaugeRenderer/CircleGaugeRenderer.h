#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../ComponentBase.h"
#include "../../../../Libs/LibCore/DxLib/BlendMode.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"

namespace NanamiEngine::Module::NanamiUi
{
    /**
     * 画像を 12 時から時計回りに扇形で削って描く（DxLib DrawCircleGaugeF）。
     * startPercent_ から spanPercent_ * fillRate だけ描き、100% をまたぐ弧は2回に分けて描く
     */
    class NANAMI_API CircleGaugeRenderer final : public Component::ComponentBase,
                                      public LifeCycleCallback::IInitRenderable,
                                      public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        void SetBlendRate(int blendRate);
        [[nodiscard]] int GetBlendRate() const { return blendRate_; }
        void SetSprite(const std::weak_ptr<Asset::SpriteFile>& sprite);
        /** @brief 描く割合（0〜1） */
        void SetFillRate(float fillRate);
        [[nodiscard]] float GetFillRate() const { return fillRate_; }

    private:
        void InitRenderer         () override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        [[serialize(0)]] Dxlib::BlendMode blendMode_ = Dxlib::BlendMode::Alpha;
        [[serialize(0)]] int blendRate_ = 255;
        [[serialize(0)]] FIELD(Asset::SpriteFile) spriteFile_;
        [[serialize(0)]] int renderOrder_ = 0;
        /** @brief 12 時を 0、1周を 100 とした描き始め */
        [[serialize(0)]] float startPercent_ = 0.0f;
        /** @brief fillRate が 1 のときに描く長さ（1周 = 100） */
        [[serialize(0)]] float spanPercent_ = 100.0f;
        [[serialize(0)]] float fillRate_ = 1.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            archive(CEREAL_NVP(blendMode_));
            archive(CEREAL_NVP(blendRate_));
            archive(CEREAL_NVP(spriteFile_));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(startPercent_));
            archive(CEREAL_NVP(spanPercent_));
            archive(CEREAL_NVP(fillRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(blendMode_));
            if (version >= 0) archive(CEREAL_NVP(blendRate_));
            if (version >= 0) archive(CEREAL_NVP(spriteFile_));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(startPercent_));
            if (version >= 0) archive(CEREAL_NVP(spanPercent_));
            if (version >= 0) archive(CEREAL_NVP(fillRate_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::NanamiUi::CircleGaugeRenderer, 0);
