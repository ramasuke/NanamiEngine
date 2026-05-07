#pragma once
#include "fwd.hpp"
#include "../ComponentBase.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Particle/ParticleFile.h"
#include "detail/type_quat.hpp"

namespace NanamiEngine::Module::Component
{
    class ParticleSystem final : public ComponentBase,
                                 public LifeCycleCallback::IUpdatable,
                                 public LifeCycleCallback::IInitRenderable,
                                 public LifeCycleCallback::IRenderable
    {
    public:
        void Play();
        
    private:
        void OnUpdate           () override;
        void InitRenderer       () override;
        void OnRender           () override;
        void TryUpdateRenderPos  ();
        void TryUpdateRenderRot  ();
        void TryUpdateRenderScale();
        void OnDestroy          () override;

        FIELD(Asset::ParticleFile) particleFile_;
        int   resourceEffectHandle_ = -1;
        int   playingEffectHandle_  = -1;
        float playingDuration_secs_ = 0.0f;
        float playingDuring_secs_   = 0.0f;
        bool  isRoop_ = true;
        
        glm::vec3 prevPos_{};
        glm::quat prevRot_{};
        glm::vec3 prevScale_{};
        bool firstUpdate_ = true;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
            archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
            archive(CEREAL_NVP(particleFile_));
            archive(CEREAL_NVP(playingDuration_secs_));
            archive(CEREAL_NVP(isRoop_));
        }
        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            archive(cereal::base_class<LifeCycleCallback::IInitRenderable>(this));
            archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
            archive(CEREAL_NVP(particleFile_));
            archive(CEREAL_NVP(playingDuration_secs_));
            archive(CEREAL_NVP(isRoop_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::ParticleSystem, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::ParticleSystem);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::Component::ParticleSystem);
#pragma endregion