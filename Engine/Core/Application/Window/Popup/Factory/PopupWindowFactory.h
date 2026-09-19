#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

#include "../../../../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "../Interface/IPopupWindow.h"


namespace NanamiEngine::Core::PopupWindow
{
    using FactoryFunc = std::function<std::unique_ptr<IPopupWindow>()>;

    class PopupWindowFactory final : public SingletonBase<PopupWindowFactory>
    {
    public:
        /** @param category ツールバーのメニューでの入れ子 ("A::B") */
        template <typename T>
        void Register(const std::string& name, const std::string& category)
        {
            static_assert(std::is_base_of_v<IPopupWindow, T>, "T must inherit from IPopupWindow");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
            factories_[name] = []
            {
                return std::make_unique<T>();
            };
            categories_[name] = category;
        }
        [[nodiscard]] const std::unordered_map<std::string, FactoryFunc>& GetAll() const { return factories_; }
        [[nodiscard]] const std::unordered_map<std::string, std::string>& GetCategories() const { return categories_; }

    private:
        std::unordered_map<std::string, FactoryFunc> factories_;
        std::unordered_map<std::string, std::string> categories_;
    };
}

#define REGISTER_POPUP_WINDOW(TYPE, CATEGORY) \
    namespace { \
        struct TYPE##AutoRegister { \
            TYPE##AutoRegister() { \
                NanamiEngine::Core::PopupWindow::PopupWindowFactory::Instance().Register<TYPE>(#TYPE, CATEGORY); \
            } \
        }; \
        static TYPE##AutoRegister global_##TYPE##AutoRegister; \
    }
