#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/AnimationFile/SpriteAnimationFile.h"
#include "../../Component/ComponentBase.h"

namespace NanamiEngine::Module::NanamiUi
{
    // SpriteAnimationFile の1コマを3D空間にビルボード描画する。
    // コマは自動では進めず、呼び出し側が SetFrame で指定する
    class NANAMI_API BillboardAnimation3D final : public Component::ComponentBase,
                                       public LifeCycleCallback::IInitRenderable,
                                       public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        void SetFrame(int frame);
        [[nodiscard]] int GetFrameCount() const;

        // 実行時の演出用（シリアライズしない）。0 で描画しない、1 で最大
        void SetAlpha(float alpha);
        void SetAngle(float angle);

    private:
        void InitRenderer() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        int   frame_ = 0;
        float alpha_ = 1.0f;

        [[serialize(0)]] int renderOrder_ = 0;
        [[serialize(0)]] FIELD(Asset::SpriteAnimationFile) animationFile_;
        [[serialize(0)]] bool isAdditive_ = true;
        [[serialize(0)]] float angle_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(animationFile_));
            archive(CEREAL_NVP(isAdditive_));
            archive(CEREAL_NVP(angle_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 0) archive(CEREAL_NVP(animationFile_));
            if (version >= 0) archive(CEREAL_NVP(isAdditive_));
            if (version >= 0) archive(CEREAL_NVP(angle_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiUi::BillboardAnimation3D, 0);
