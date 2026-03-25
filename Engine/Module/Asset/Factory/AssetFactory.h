#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "../../../../Libs/LibCore/FilePathHelper/FilePathHelper.h"
#include "../../../../Libs/Singleton/LibCore_SingletonBase.h"
#include "../../../Core/Application/ApplicationBase.h"
#include "../../../Core/Object/Registry/ObjectRegistry.h"
#include "../AssetBase.h"
#include "../../../Core/Application/LifeCycle/ApplicationLifeCycle.h"
#include "../cereal/include/cereal/archives/json.hpp"

namespace NanamiEngine::Module::Asset
{
    using OnCreateAsset = std::function<bool(const std::string&, std::shared_ptr<AssetBase>&)>;
    class AssetFactory final : public SingletonBase<AssetFactory>
    {
    public:
        template <typename T>
        void Register(const std::string& extensionLabel);
        void RegisterCreatableAssetExtension(const std::string& assetNameLabel, const std::string& extensionLabel);
        bool TryCreate(const std::string& filePath, std::shared_ptr<AssetBase>& outAsset) const;
        [[nodiscard]] std::shared_ptr<AssetBase> Load(const std::string& filePath) const;
        template <typename T>
        void RegisterLoader(const std::string& extensionLabel);
        /** 新規作成可能なアセットの<アセット名, 拡張子>群 */
        [[nodiscard]] const std::vector<std::pair<std::string, std::string>>& CreatableAssets() const { return creatableAssetsData_; }

    private:
        /** filePathからfileを生成する関数群 */
        std::vector<OnCreateAsset> factories_;
        std::vector<std::function<std::shared_ptr<AssetBase>(const std::string&)>> loaderers_;
        /** 新規作成可能なアセットの<アセット名, 拡張子>群 */
        std::vector<std::pair<std::string, std::string>> creatableAssetsData_;
    };

    template <typename T>
    void AssetFactory::Register(const std::string& extensionLabel)
    {
        static_assert(std::is_base_of_v<AssetBase, T>, "T must inherit from AssetBase");
        static_assert(std::is_constructible_v<T, std::string>, "T must be constructible from std::string");

        factories_.emplace_back(
            [extensionLabel](const std::string& filePath, std::shared_ptr<AssetBase>& out)
            {
                if (!LibCore::FilePath::IsExtension(filePath, extensionLabel))
                {
                    out = nullptr;
                    return false;
                }


                auto file = std::make_shared<T>(filePath);
                out = file;
                std::weak_ptr<T> weak = file;
                Core::Application::ApplicationBase::ObjectRegistry      ().Add(weak);
                Core::Application::ApplicationBase::ApplicationLifeCycle().AddCallback(weak);
                return true;
            });
    }

    template <typename T>
    void AssetFactory::RegisterLoader(const std::string& extensionLabel)
    {
        static_assert(std::is_base_of_v<AssetBase, T>, "T must inherit from AssetBase");

        loaderers_.emplace_back(
            [extensionLabel](const std::string& filePath) -> std::shared_ptr<AssetBase>
            {
                if (!LibCore::FilePath::IsExtension(filePath, extensionLabel))
                    return nullptr;

                std::ifstream ifStream(filePath + ".meta");
                if (!ifStream.is_open())
                    return nullptr;

                cereal::JSONInputArchive archive(ifStream);
                std::shared_ptr<AssetBase> loaded;
                archive(loaded);

                if (auto loadShared = std::dynamic_pointer_cast<T>(loaded))
                {
                    std::weak_ptr<T> weak = loadShared;
                    Core::Application::ApplicationBase::ObjectRegistry().Add(weak);
                    Core::Application::ApplicationBase::ApplicationLifeCycle().AddCallback(weak);
                    return loadShared;
                }

                return nullptr;
            });
    }
}

#define REGISTER_ASSET(TYPE, LABEL)                                    \
namespace NanamiEngine::Module::Asset {                                \
struct TYPE##AutoRegister {                                            \
TYPE##AutoRegister() {                                                 \
auto& factory = NanamiEngine::Module::Asset::AssetFactory::Instance(); \
factory.Register<TYPE>(LABEL);                                         \
factory.RegisterLoader<TYPE>(LABEL);                                   \
}                                                                      \
};                                                                     \
static TYPE##AutoRegister global_##TYPE##AutoRegister;                 \
}

#define REGISTER_CREATABLE_ASSET_EXTENSION(ASSET_LABEL, EXTENSION_LABEL)                        \
namespace NanamiEngine::Module::Asset {                                                         \
struct AutoRegisterCreatable_##EXTENSION_LABEL {                                                \
AutoRegisterCreatable_##EXTENSION_LABEL() {                                                     \
auto& factory = NanamiEngine::Module::Asset::AssetFactory::Instance();                          \
factory.RegisterCreatableAssetExtension(ASSET_LABEL, EXTENSION_LABEL);                          \
}                                                                                               \
};                                                                                              \
static AutoRegisterCreatable_##EXTENSION_LABEL global_autoregister_creatable_##EXTENSION_LABEL; \
}
