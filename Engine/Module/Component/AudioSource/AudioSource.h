#pragma once
#include "../ComponentBase.h"

namespace NanamiEngine::Module::Asset
{
    class SoundFile;
}

namespace NanamiEngine::Module::Component
{
    class AudioSource final : public ComponentBase,
                              public LifeCycleCallback::IAwakable,
                              public LifeCycleCallback::IUpdatable
    {
    public:
        void Play(const Asset::SoundFile& soundFile, const glm::vec3& soundPos) const;
        void SetLoop(bool loop);
        
    private:
        void OnAwake() override;
        void OnUpdate() override;

        [[serialize(0)]] float listenableRadius_ = 256.0f;
        [[serialize(0)]] bool  isLoop_ = false;
        [[serialize(0)]] int   volume_ = 255;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(listenableRadius_));
            archive(CEREAL_NVP(isLoop_));
            archive(CEREAL_NVP(volume_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(listenableRadius_));
            if (version >= 0) archive(CEREAL_NVP(isLoop_));
            if (version >= 0) archive(CEREAL_NVP(volume_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::AudioSource, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::AudioSource);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::Component::AudioSource);
#pragma endregion