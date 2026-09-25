#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "Engine/Core/Api/NanamiModule.h"
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
#include "../../Exception/Engine_Module_Exception.h"
#include "../../Log/NanamiEngine_Module_Log.h"
#include "../../Serialization/Engine_Module_Serialization.h"
#include "../cereal/include/cereal/archives/json.hpp"

namespace NanamiEngine::Module::Asset
{
    using OnCreateAsset = std::function<bool(const std::string&, std::shared_ptr<AssetBase>&)>;

    /** ProjectWindow の「+」から新規作成できるアセット */
    struct NANAMI_API CreatableAsset
    {
        std::string name;
        std::string extension;
        /** 「+」メニューでの入れ子 ("A::B") */
        std::string category;
    };

    class NANAMI_API AssetFactory final : public SingletonBase<AssetFactory>
    {
    public:
        static AssetFactory& Instance();

    public:
        template <typename T>
        void Register(const std::string& extensionLabel);
        /** @param module 登録元のモジュール。REGISTER_CREATABLE_ASSET_EXTENSION が NANAMI_CURRENT_MODULE() を渡す */
        void RegisterCreatableAssetExtension(const std::string& assetNameLabel, const std::string& extensionLabel, const std::string& categoryLabel, Core::ModuleHandle module = {});
        bool TryCreate(const std::string& filePath, std::shared_ptr<AssetBase>& outAsset) const;
        /** アセットを生成せずに、filePath の拡張子が Register 済みかだけを判定する */
        [[nodiscard]] bool IsRegisteredExtension(const std::string& filePath) const;
        [[nodiscard]] std::shared_ptr<AssetBase> Load(const std::string& filePath) const;
        template <typename T>
        void RegisterLoader(const std::string& extensionLabel);
        [[nodiscard]] std::vector<CreatableAsset> CreatableAssets() const;
        /** @brief module が登録した拡張子・ローダー・新規作成メニューを消す。戻り値は消した数 */
        std::size_t UnregisterModule(Core::ModuleHandle module);

    private:
        template <typename EntryT>
        struct Registered
        {
            EntryT            entry;
            Core::ModuleHandle module;
        };
        /** filePathからfileを生成する関数群 */
        std::vector<Registered<OnCreateAsset>> factories_;
        /** factories_ に登録された拡張子群 */
        std::vector<Registered<std::string>> registeredExtensions_;
        std::vector<Registered<std::function<std::shared_ptr<AssetBase>(const std::string&)>>> loaderers_;
        std::vector<Registered<CreatableAsset>> creatableAssetsData_;
    };

    template <typename T>
    void AssetFactory::Register(const std::string& extensionLabel)
    {
        static_assert(std::is_base_of_v<AssetBase, T>, "T must inherit from AssetBase");
        static_assert(std::is_constructible_v<T, std::string>, "T must be constructible from std::string");

        const Core::ModuleHandle module = NANAMI_CURRENT_MODULE();
        registeredExtensions_.push_back({ extensionLabel, module });
        factories_.push_back({
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
            }, module });
    }

    template <typename T>
    void AssetFactory::RegisterLoader(const std::string& extensionLabel)
    {
        static_assert(std::is_base_of_v<AssetBase, T>, "T must inherit from AssetBase");

        const Core::ModuleHandle module = NANAMI_CURRENT_MODULE();
        loaderers_.push_back({
            [extensionLabel](const std::string& filePath) -> std::shared_ptr<AssetBase>
            {
                if (!LibCore::FilePath::IsExtension(filePath, extensionLabel))
                    return nullptr;

                std::shared_ptr<AssetBase> loaded;
                try
                {
                    const bool exists = Serialization::LoadJsonFileIfExists(filePath + ".meta", [&loaded](cereal::JSONInputArchive& archive)
                    {
                        archive(loaded);
                    });
                    if (!exists)
                        return nullptr;
                }
                catch (const Exception::SerializationException& exception)
                {
                    // 壊れた .meta が 1 つあっても起動時のスキャンや Reload Assets 全体を止めない。
                    // このファイルは「中身の無い File」（content_ == nullptr）として扱われ、Save でも上書きされない
                    LogError("AssetFactory: .meta の読み込みに失敗しました: " + std::string(exception.what()));
                    return nullptr;
                }

                if (auto loadShared = std::dynamic_pointer_cast<T>(loaded))
                {
                    std::weak_ptr<T> weak = loadShared;
                    Core::Application::ApplicationBase::ObjectRegistry().Add(weak);
                    Core::Application::ApplicationBase::ApplicationLifeCycle().AddCallback(weak);
                    return loadShared;
                }

                return nullptr;
            }, module });
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

#define REGISTER_CREATABLE_ASSET_EXTENSION(ASSET_LABEL, EXTENSION_LABEL, CATEGORY_LABEL)        \
namespace NanamiEngine::Module::Asset {                                                         \
struct AutoRegisterCreatable_##EXTENSION_LABEL {                                                \
AutoRegisterCreatable_##EXTENSION_LABEL() {                                                     \
auto& factory = NanamiEngine::Module::Asset::AssetFactory::Instance();                          \
factory.RegisterCreatableAssetExtension(ASSET_LABEL, EXTENSION_LABEL, CATEGORY_LABEL, NANAMI_CURRENT_MODULE()); \
}                                                                                               \
};                                                                                              \
static AutoRegisterCreatable_##EXTENSION_LABEL global_autoregister_creatable_##EXTENSION_LABEL; \
}
