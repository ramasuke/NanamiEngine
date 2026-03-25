#pragma once
#include <optional>
#include "../File/File.h"

namespace NanamiEngine::Core::FileSystem
{
    constexpr auto EDITOR_DRAGGING_ITEM_PAYLOAD_TYPE = "EDITOR_DRAGGING_ITEM";
    
    class EditorDraggingHand final
    {
    public:
        void SetDraggingItem(const Guid& dragging);
        [[nodiscard]] bool HasDraggingFile() const
        {
            return draggingFile_ != nullptr;
        }
        
        [[nodiscard]] bool HasDraggingItem() const
        {
            return draggingFile_ != nullptr || draggingItemGuid_.has_value();
        }

        [[nodiscard]] std::optional<Guid> TakeDraggingItemGuid()
        {
            return draggingItemGuid_;
        }

    private:
        File* draggingFile_ = nullptr;
        std::optional<Guid> draggingItemGuid_;
    };
}
