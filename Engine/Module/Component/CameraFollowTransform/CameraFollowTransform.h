#pragma once
#include "../ComponentBase.h"
#include "../../LifeCycleCallback/LateUpdate/LateUpdate.h"

namespace NanamiEngine::Module::Component
{
    /** @brief ワールド座標をカメラ位置に追従させる。天候パーティクルのように常に視点周りへ置きたいもの用 */
    class CameraFollowTransform final : public ComponentBase,
                                        public LifeCycleCallback::ILateUpdatable
    {
    private:
        void OnLateUpdate() override;

        [[serialize(0)]] glm::vec3 offset_  = glm::vec3(0.0f, 0.0f, 0.0f);
        [[serialize(0)]] bool      followX_ = true;
        [[serialize(0)]] bool      followY_ = true;
        [[serialize(0)]] bool      followZ_ = true;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(offset_));
            archive(CEREAL_NVP(followX_));
            archive(CEREAL_NVP(followY_));
            archive(CEREAL_NVP(followZ_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(offset_));
            if (version >= 0) archive(CEREAL_NVP(followX_));
            if (version >= 0) archive(CEREAL_NVP(followY_));
            if (version >= 0) archive(CEREAL_NVP(followZ_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::CameraFollowTransform, 0)
