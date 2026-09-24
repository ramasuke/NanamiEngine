#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "../../../../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "IEditorToolbarWidget.h"

namespace NanamiEngine::Core::Toolbar
{
    class EditorToolbarWidgetRegistry final : public SingletonBase<EditorToolbarWidgetRegistry>
    {
    public:
        struct Entry
        {
            std::string                           name;
            int                                   order;
            std::unique_ptr<IEditorToolbarWidget> widget;
        };

        /** @param order 小さいほど左。組み込みは 100 刻み */
        template <typename T>
        void Register(const std::string& name, const int order)
        {
            static_assert(std::is_base_of_v<IEditorToolbarWidget, T>, "T must inherit from IEditorToolbarWidget");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
            Add({ name, order, std::make_unique<T>() });
        }

        /** @brief order 順 (同じなら名前順) */
        [[nodiscard]] const std::vector<Entry>& GetWidgets() const { return entries_; }

        void DrawAll(EditorToolbarWidgetContext& context) const;

    private:
        void Add(Entry entry);

        std::vector<Entry> entries_;
    };
}

// NOTE: .cpp の末尾、TYPE と同じ namespace の中に置く (TYPE は名前空間なしで書く)
#define REGISTER_EDITOR_TOOLBAR_WIDGET(TYPE, ORDER) \
    namespace { \
        struct EditorToolbarWidgetAutoRegister_##TYPE { \
            EditorToolbarWidgetAutoRegister_##TYPE() { \
                ::NanamiEngine::Core::Toolbar::EditorToolbarWidgetRegistry::Instance().Register<TYPE>(#TYPE, ORDER); \
            } \
        }; \
        const EditorToolbarWidgetAutoRegister_##TYPE editorToolbarWidgetAutoRegister_##TYPE; \
    }
