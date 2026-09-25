#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine/Core/Api/NanamiModule.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

#include "../../../../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "../Interface/IPopupWindow.h"


namespace NanamiEngine::Core::PopupWindow
{
    using FactoryFunc = std::function<std::unique_ptr<IPopupWindow>()>;

    class NANAMI_API PopupWindowFactory final : public SingletonBase<PopupWindowFactory>
    {
    public:
        static PopupWindowFactory& Instance();

    public:
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
            modules_   [name] = NANAMI_CURRENT_MODULE();
        }
        [[nodiscard]] const std::unordered_map<std::string, FactoryFunc>& GetAll() const { return factories_; }
        [[nodiscard]] const std::unordered_map<std::string, std::string>& GetCategories() const { return categories_; }

        /** @brief module が登録したウィンドウ種別を消す。戻り値は消した数 */
        std::size_t UnregisterModule(const ModuleHandle module)
        {
            std::size_t count = 0;
            for (auto it = modules_.begin(); it != modules_.end();)
            {
                if (it->second != module)
                {
                    ++it;
                    continue;
                }
                factories_ .erase(it->first);
                categories_.erase(it->first);
                it = modules_.erase(it);
                ++count;
            }
            return count;
        }

    private:
        std::unordered_map<std::string, FactoryFunc>  factories_;
        std::unordered_map<std::string, std::string>  categories_;
        std::unordered_map<std::string, ModuleHandle> modules_;
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
