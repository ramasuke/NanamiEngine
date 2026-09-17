#pragma once
#include <optional>
#include <unordered_map>
#include "../../../../Module/Guid/Guid.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Core::Object
{
    //NOTE: 値オブジェクト
    struct GuidRemap final
    {
        explicit GuidRemap(std::unordered_map<Guid, Guid, GuidHash> copiedGuids);
        /** @brief 複製元と複製先のヒエラルキーを同じ順番でたどり、GameObject / Component の元の GUID を複製先の GUID に対応付ける */
        [[nodiscard]] static GuidRemap FromCopiedHierarchy(Module::GameObject::IGameObject& source, Module::GameObject::IGameObject& copied);

        /** @brief 複製したヒエラルキー内の GUID なら複製先の GUID を返す */
        [[nodiscard]] std::optional<Guid> Find(const Guid& sourceGuid) const;

        bool operator==(const GuidRemap& other) const { return copiedGuids_ == other.copiedGuids_; }
        bool operator!=(const GuidRemap& other) const { return !(*this == other); }

    private:
        std::unordered_map<Guid, Guid, GuidHash> copiedGuids_;
    };
}
