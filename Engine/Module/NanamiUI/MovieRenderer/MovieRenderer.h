#pragma once
#include "../../Component/ComponentBase.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Movie/MovieFile.h"
#include "../../../../Libs/LibCore/DxLib/BlendMode.h"

namespace NanamiEngine::Module::NanamiUi
{
    class MovieRenderer final : public Component::ComponentBase,
                                public LifeCycleCallback::IInitRenderable,
                                public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        void SetBlendRate(int blendRate);
        [[nodiscard]] int GetBlendRate() const { return blendRate_; }

    private:
        void InitRenderer         () override;
        void OnUserInterfaceRender() override;
        void OnDestroy() override;
        void TryDeleteResource();
        void UpdateRenderHandle();
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        [[serialize(0)]] FIELD(Asset::MovieFile) movieFile_;
        [[serialize(0)]] bool isRoop_      = true;
        [[serialize(0)]] int  renderOrder_ = 0;
        [[serialize(1)]] Dxlib::BlendMode blendMode_ = Dxlib::BlendMode::Alpha;
        [[serialize(1)]] int blendRate_ = 255;
        int   movieHandle_ = -1;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(movieFile_));
            archive(CEREAL_NVP(isRoop_));
            archive(CEREAL_NVP(renderOrder_));
            archive(CEREAL_NVP(blendMode_));
            archive(CEREAL_NVP(blendRate_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(movieFile_));
            if (version >= 0) archive(CEREAL_NVP(isRoop_));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
            if (version >= 1) archive(CEREAL_NVP(blendMode_));
            if (version >= 1) archive(CEREAL_NVP(blendRate_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::NanamiUi::MovieRenderer, 1)