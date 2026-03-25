#pragma once
#include <memory>
#include "../../../../../Module/Component/ComponentBase.h"
#include "../../../../../Module/LifeCycleCallback/Update/IUpdatable.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "../../Libs/glm/gtx/quaternion.hpp"

namespace NanamiEngine::Module::Component
{
    class Editor3DCamera final
    {
    public:
        Editor3DCamera();
        void OnUpdate();
        [[nodiscard]] glm::mat4 GetViewMatrix() const;
        [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

        [[nodiscard]] glm::vec3 GetPosition () const { return cameraPosition_; }
        [[nodiscard]] glm::quat GetRotataion() const { return cameraRotation_; }
        
    private:
        void OnUpdateCameraRotation();
        void OnUpdateCameraPosition();

        float fov_      = glm::radians(60.0f);
        float near_ = 3.0f;
        float far_  = 2300.0f;
        glm::ivec2 previewMousePos_;
        glm::vec3 cameraPosition_;
        glm::quat cameraRotation_;
        glm::mat4 matrix_;

        float moveRunSpeedCoefficient_ = 20.0f;
        [[serialize(0)]] float rotationSpeed_   = 0.03f;
        [[serialize(0)]] float moveSpeed_       = 0.1f;
        [[serialize(0)]] float sensitivity_     = 0.0025f;
#pragma region Serialization Function
public:
void OnDrawGui() {
    LibCore::ImGuiHelper::OnDrawInputField("rotationSpeed_", rotationSpeed_);
    LibCore::ImGuiHelper::OnDrawInputField("moveSpeed_", moveSpeed_);
    LibCore::ImGuiHelper::OnDrawInputField("sensitivity_", sensitivity_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(CEREAL_NVP(rotationSpeed_));
    archive(CEREAL_NVP(moveSpeed_));
    archive(CEREAL_NVP(sensitivity_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    if (version >= 0) archive(CEREAL_NVP(rotationSpeed_));
    if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
    if (version >= 0) archive(CEREAL_NVP(sensitivity_));
}
#pragma endregion
};
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::Editor3DCamera, 0);
#pragma endregion
