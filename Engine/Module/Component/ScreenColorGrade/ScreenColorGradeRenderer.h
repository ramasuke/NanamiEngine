#pragma once
#include "../ComponentBase.h"

namespace NanamiEngine::Module::NanamiUi
{
    // UI描画フェーズでバックバッファを取り込み、HSBフィルタをかけて描き戻す。renderOrder_ より後に描かれるUIには影響しない
    class ScreenColorGradeRenderer final : public Component::ComponentBase,
                                           public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        void SetSaturation(int saturation);
        void SetBright    (int bright);
        [[nodiscard]] int GetSaturation() const { return saturation_; }
        [[nodiscard]] int GetBright    () const { return bright_;     }

    private:
        void OnUserInterfaceRender() override;
        void OnDestroy() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        [[serialize(0)]] int renderOrder_ = -1000;

        int saturation_   = 0;
        int bright_       = 0;
        int screenHandle_ = -1;
        int screenWidth_  = 0;
        int screenHeight_ = 0;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            archive(CEREAL_NVP(renderOrder_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IUserInterfaceRenderable>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::NanamiUi::ScreenColorGradeRenderer, 0)
