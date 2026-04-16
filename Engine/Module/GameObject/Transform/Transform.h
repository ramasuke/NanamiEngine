#pragma once
#include "DxLib.h"
#include <../../Libs/glm/glm.hpp>
#include <../../Libs/glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <../../Libs/glm/gtx/quaternion.hpp>
#include <vector>
#include <memory>

#include "../../../../Libs/LibCore/Tween/Vec3Tween/Vec3Tween.h"
#include "../cereal/include/cereal/cereal.hpp"
#include "../cereal/include/cereal/types/memory.hpp"
#include "../../Libs/LibCore/cereal/glm/GlmHelper.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;

    class Transform final
    {
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const;
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version);

        Transform();
        void InitTransform(const std::weak_ptr<IGameObject>& parent, const std::weak_ptr<IGameObject>& ownerGameObject);

        void SetWorldMatrix(const glm::mat4& worldMatrix);
        void SetWorldPos   (const glm::vec3& worldPos   );
        void SetWorldRot   (const glm::quat& worldRot   );
        void SetWorldScale (const glm::vec3& worldScale );
        void SetLocalMatrix(const glm::mat4& localMatrix);
        void SetLocalPos   (const glm::vec3& pos        );
        void SetLocalRot   (const glm::quat& rot        );
        void SetLocalScale (const glm::vec3& scale      );
        void LookAt(const glm::vec3& targetPos);
        void LookAtY(const glm::vec3& targetPos);

        // LibCore::Tween::Vec3Tween DoMove(glm::vec3 targetPos, float duration_msecs) const;
        
        void Translate(const glm::vec3& delta);
        void Rotate(const glm::quat& deltaRot);

        [[nodiscard]] const glm::mat4& GetWorldMatrix   () const { return worldMatrix_; }
        [[nodiscard]] glm::vec3        GetWorldPos      () const;
        [[nodiscard]] glm::quat        GetWorldRot      () const;
        [[nodiscard]] glm::vec3        GetWorldEulerAngle() const;
        [[nodiscard]] glm::vec3        GetWorldScale    () const;
        [[nodiscard]] VECTOR           GetDxWorldPos    () const;
        [[nodiscard]] MATRIX           GetDxWorldMatrix () const;
        
        void SetParent(const std::weak_ptr<IGameObject>& parent, bool keepWorldScale = true);
        [[nodiscard]] std::shared_ptr<IGameObject> GetParent    () const { return parent_         .lock(); }
        [[nodiscard]] std::shared_ptr<IGameObject> GetGameObject() const { return ownerGameObject_.lock(); }

        [[nodiscard]] std::vector<std::shared_ptr<IGameObject>> GetAllChildren() const;
        [[nodiscard]] std::vector<std::shared_ptr<IGameObject>> GetChildren() const;
        [[nodiscard]] std::shared_ptr<IGameObject>              CatchChild(const std::string& childName) const;
        void AddChild   (const std::shared_ptr<IGameObject>& child);
        void RemoveChild(const std::shared_ptr<IGameObject>& child);

        [[nodiscard]] const glm::vec3& GetLocalPos()   const { return localPos_;   }
        [[nodiscard]] const glm::quat& GetLocalRot()   const { return localRot_;   }
        [[nodiscard]] const glm::vec3& GetLocalScale() const { return localScale_; }
        [[nodiscard]] glm::mat4 GetLocalMatrix() const;

        void OnEnable(bool enable) const;
        void OnDrawGui();

    private:
        void UpdateMatrix();

        glm::vec3 guiLocalEuler_;
        
        glm::vec3 localPos_;
        glm::quat localRot_;
        glm::vec3 localScale_;
        glm::mat4 worldMatrix_;

        std::weak_ptr<IGameObject> parent_;
        std::vector<std::shared_ptr<IGameObject>> children_;
        std::weak_ptr<IGameObject> ownerGameObject_;
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::GameObject::Transform, 0);
