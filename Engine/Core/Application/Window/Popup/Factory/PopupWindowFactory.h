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
        template <typename T>
        void Register(const std::string& name)
        {
            static_assert(std::is_base_of_v<IPopupWindow, T>, "T must inherit from IPopupWindow");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");
            factories_[name] = []
            {
                return std::make_unique<T>();
            };
        }
        [[nodiscard]] const std::unordered_map<std::string, FactoryFunc>& GetAll() const { return factories_; }

    private:
        std::unordered_map<std::string, FactoryFunc> factories_;
    };
}

#define REGISTER_POPUP_WINDOW(TYPE) \
    namespace { \
        struct TYPE##AutoRegister { \
            TYPE##AutoRegister() { \
                NanamiEngine::Core::PopupWindow::PopupWindowFactory::Instance().Register<TYPE>(#TYPE); \
            } \
        }; \
        static TYPE##AutoRegister global_##TYPE##AutoRegister; \
    }
