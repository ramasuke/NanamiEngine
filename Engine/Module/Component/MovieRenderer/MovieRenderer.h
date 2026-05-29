#pragma once
#include "../ComponentBase.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Movie/MovieFile.h"

namespace NanamiEngine::Module::Component
{
    class MovieRenderer final : public ComponentBase,
                                public LifeCycleCallback::IInitRenderable,
                                public LifeCycleCallback::IUserInterfaceRenderable,
                                public LifeCycleCallback::IUpdatable
    {
    public:
        
    private:
        void InitRenderer         () override;
        void OnUserInterfaceRender() override;
        void OnUpdate() override;
        void OnDestroy() override;
        void TryDeleteResource();
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }

        [[serialize(0)]] FIELD(Asset::MovieFile) movieFile_;
        [[serialize(0)]] float playingDuration_secs_ = 0.0f;
        [[serialize(0)]] bool isRoop_      = true;
        [[serialize(0)]] int  renderOrder_ = 0;
        int   movieHandle_ = -1;
        float playingDuring_secs_ = 0.0f; 
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(movieFile_));
            archive(CEREAL_NVP(playingDuration_secs_));
            archive(CEREAL_NVP(isRoop_));
            archive(CEREAL_NVP(renderOrder_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(movieFile_));
            if (version >= 0) archive(CEREAL_NVP(playingDuration_secs_));
            if (version >= 0) archive(CEREAL_NVP(isRoop_));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::MovieRenderer, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::MovieRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::Component::MovieRenderer);
#pragma endregion