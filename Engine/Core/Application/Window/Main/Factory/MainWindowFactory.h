#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cassert>
#include <type_traits>

#include "../../../../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "../../../ApplicationBase.h"
#include "../Interface/IMainWindow.h"

namespace NanamiEngine::Core::MainWindow
{
    class MainWindowFactory final : public SingletonBase<MainWindowFactory>
    {
    public:
        /** @param category ツールバーのメニューでの入れ子 ("A::B") */
        template <typename T>
        void Register(const std::string& name, const std::string& category)
        {
            static_assert(std::is_base_of_v<IMainWindow, T>, "T must inherit from IMainWindow");
            static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

            factories_[name] = [] {
                return std::make_shared<T>();
            };

            loaders_[name] = [] {
                return Application::ApplicationBase::MainWindows().Catch<T>();
            };

            categories_[name] = category;
        }

        std::shared_ptr<IMainWindow> Load(const std::string& name)
        {
            if (const auto loaderIt = loaders_.find(name); loaderIt != loaders_.end())
            {
                return loaderIt->second(); // Catch<T>() に登録済みのインスタンスを返す
            }

            const auto factoryIt = factories_.find(name);
            assert(factoryIt != factories_.end() && "Factory not registered for this name");
            return factoryIt->second();
        }

        const std::unordered_map<std::string, std::function<std::shared_ptr<IMainWindow>()>>& GetFactories() const
        {
            return factories_;
        }

        const std::unordered_map<std::string, std::function<std::shared_ptr<IMainWindow>()>>& GetLoaders() const
        {
            return loaders_;
        }

        const std::unordered_map<std::string, std::string>& GetCategories() const
        {
            return categories_;
        }

    private:
        std::unordered_map<std::string, std::function<std::shared_ptr<IMainWindow>()>> factories_;
        std::unordered_map<std::string, std::function<std::shared_ptr<IMainWindow>()>> loaders_;
        std::unordered_map<std::string, std::string>                                    categories_;
    };
}

#define REGISTER_MAIN_WINDOW(TYPE, CATEGORY) \
namespace { \
struct TYPE##AutoRegister { \
TYPE##AutoRegister() { \
NanamiEngine::Core::MainWindow::MainWindowFactory::Instance().Register<TYPE>(#TYPE, CATEGORY); \
} \
}; \
static TYPE##AutoRegister global_##TYPE##AutoRegister; \
}