#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "../../../../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    class NANAMI_API EditorToolbarWidgetRegistry final : public SingletonBase<EditorToolbarWidgetRegistry>
    {
    public:
        static EditorToolbarWidgetRegistry& Instance();

    public:
        struct NANAMI_API Entry
        {
            std::string                           name;
            int                                   order;
            std::unique_ptr<IEditorToolbarWidget> widget;
        };

        template <typename T>
        void Register(const std::string& name, const int order)
        {
            static_assert(std::is_base_of_v<IEditorToolbarWidget, T>, "T must inherit from IEditorToolbarWidget");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
            Add({ name, order, std::make_unique<T>() });
        }

        [[nodiscard]] const std::vector<Entry>& GetWidgets() const { return entries_; }

        void DrawAll(EditorToolbarWidgetContext& context) const;

    private:
        void Add(Entry entry);

        std::vector<Entry> entries_;
    };
}

#define REGISTER_EDITOR_TOOLBAR_WIDGET(TYPE, ORDER) \
    namespace { \
        struct EditorToolbarWidgetAutoRegister_##TYPE { \
            EditorToolbarWidgetAutoRegister_##TYPE() { \
                ::NanamiEngine::Core::Toolbar::EditorToolbarWidgetRegistry::Instance().Register<TYPE>(#TYPE, ORDER); \
            } \
        }; \
        const EditorToolbarWidgetAutoRegister_##TYPE editorToolbarWidgetAutoRegister_##TYPE; \
    }
