#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"
#include "../../Component/ComponentBase.h"

namespace NanamiEngine::Module::NanamiUi
{
    class NANAMI_API Billboard3D final : public Component::ComponentBase,
                              public LifeCycleCallback::IInitRenderable,
                              public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        // 実行時の演出用（シリアライズしない）。0 で描画しない、1 で不透明
        void SetAlpha(float alpha);
        void SetAngle(float angle);
        [[nodiscard]] float GetAngle() const { return angle_; }

    private:
        void InitRenderer() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        float alpha_ = 1.0f;

        [[serialize(0)]] int renderOrder_ = 0;
        [[serialize(1)]] FIELD(Asset::SpriteFile) spriteFile_;
        [[serialize(2)]] float angle_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(spriteFile_));
            archive(CEREAL_NVP(angle_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 1) archive(CEREAL_NVP(spriteFile_));
            if (version >= 2) archive(CEREAL_NVP(angle_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiUi::Billboard3D, 2);
