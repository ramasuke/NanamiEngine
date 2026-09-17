#include "GuidRemap.h"

#include <utility>
#include "../../../../Module/Component/ComponentBase.h"
#include "../../../../Module/GameObject/ComponentGroup/ComponentGroup.h"
#include "../../../../Module/GameObject/Interface/IGameObject.h"
#include "../../../../Module/GameObject/Transform/Transform.h"

namespace NanamiEngine::Core::Object
{
    namespace
    {
        void GuidRemapCollect(
            Module::GameObject::IGameObject& source,
            Module::GameObject::IGameObject& copied,
            std::unordered_map<Guid, Guid, GuidHash>& copiedGuids)
        {
            copiedGuids.insert_or_assign(source.GetGuid(), copied.GetGuid());

            const auto sourceComponents = source.Components().Catches<Module::Component::ComponentBase>();
            const auto copiedComponents = copied.Components().Catches<Module::Component::ComponentBase>();
            for (std::size_t i = 0; i < sourceComponents.size() && i < copiedComponents.size(); ++i)
            {
                const auto sourceComponent = sourceComponents[i].lock();
                const auto copiedComponent = copiedComponents[i].lock();
                if (sourceComponent && copiedComponent)
                    copiedGuids.insert_or_assign(sourceComponent->GetGuid(), copiedComponent->GetGuid());
            }

            const auto sourceChildren = source.Transform().GetChildren();
            const auto copiedChildren = copied.Transform().GetChildren();
            for (std::size_t i = 0; i < sourceChildren.size() && i < copiedChildren.size(); ++i)
            {
                if (sourceChildren[i] && copiedChildren[i])
                    GuidRemapCollect(*sourceChildren[i], *copiedChildren[i], copiedGuids);
            }
        }
    }

    GuidRemap::GuidRemap(std::unordered_map<Guid, Guid, GuidHash> copiedGuids)
        : copiedGuids_(std::move(copiedGuids))
    {
    }

    GuidRemap GuidRemap::FromCopiedHierarchy(Module::GameObject::IGameObject& source, Module::GameObject::IGameObject& copied)
    {
        std::unordered_map<Guid, Guid, GuidHash> copiedGuids;
        GuidRemapCollect(source, copied, copiedGuids);
        return GuidRemap(std::move(copiedGuids));
    }

    std::optional<Guid> GuidRemap::Find(const Guid& sourceGuid) const
    {
        const auto it = copiedGuids_.find(sourceGuid);
        if (it == copiedGuids_.end())
            return std::nullopt;

        return it->second;
    }
}
