#include "Prop_StoryMovieParts.h"

#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Physics/Physics.h"
#include "Engine/Module/Component/ParticleRenderer/ParticleSystem.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Physics/BodyAssembler/Engine_Physics_BodyAssembler.h"
#include "Engine/Module/Physics/Component/Collider/Engine_Physics_ColliderBase.h"

namespace GamePlay::Prop::StoryMovie
{
    void SetChildParticlesPlaying(NanamiEngine::Module::GameObject::IGameObject& root, const bool isPlaying)
    {
        for (const auto& child : root.Transform().GetAllChildren())
        {
            if (const auto particle = child->Components().Catch<NanamiEngine::Module::Component::ParticleSystem>().lock())
            {
                if (isPlaying)
                    particle->Play();
                else
                    particle->Stop();
            }
        }
    }

    void RebuildColliders(NanamiEngine::Module::GameObject::IGameObject& root)
    {
        auto& bodies = NanamiEngine::Core::Application::ApplicationBase::Physics().Bodies();
        const auto rebuild = [&bodies](NanamiEngine::Module::GameObject::IGameObject& gameObject)
        {
            for (const auto& weak : gameObject.Components().Catches<NanamiEngine::Module::Component::ColliderBase>())
            {
                if (const auto collider = weak.lock())
                    bodies.MarkDirty(*collider);
            }
        };
        rebuild(root);
        for (const auto& child : root.Transform().GetAllChildren())
            rebuild(*child);
    }

    void MoveBy(NanamiEngine::Module::GameObject::IGameObject& gameObject, const glm::vec3& offset)
    {
        gameObject.Transform().SetWorldPos(gameObject.Transform().GetWorldPos() + offset);
    }
}
