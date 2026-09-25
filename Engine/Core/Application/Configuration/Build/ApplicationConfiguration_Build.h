#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace NanamiEngine::Module::Asset
{
    class SceneFile;
}

namespace NanamiEngine::Core::Application::Configuration
{
    enum class BuildTargetConfiguration
    {
        Release,
        Debug,
    };

    /**
     * @brief Build Settings ウィンドウで Game 版をビルドするときの設定。
     *        製品名・起動シーン・クライアント版はゲームも読むので ProjectConfig/Build/Runtime/ に置いて同梱し、残りはエディタ専用
     */
    class NANAMI_API BuildConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static const std::string& ProductName() { return productName_; }
        static void SetProductName(const std::string& productName);
        /** @brief exe 名に使えない製品名なら理由を返す。使えるなら空文字 */
        [[nodiscard]] static std::string ValidateProductName(const std::string& productName);

        /** @brief ゲーム本体の版。配信 manifest の requiredClientVersion と比べて、古い本体にはアセット更新を当てない */
        [[nodiscard]] static const std::string& ClientVersion() { return clientVersion_; }
        static void SetClientVersion(const std::string& clientVersion);
        /** @brief 版として使えない文字列なら理由を返す。使えるなら空文字 */
        [[nodiscard]] static std::string ValidateClientVersion(const std::string& clientVersion);

        /** @brief 空なら既定の起動シーンを使う */
        [[nodiscard]] static const std::string& StartSceneGuid() { return startSceneGuid_; }
        static void SetStartSceneGuid(const std::string& startSceneGuid);
        /** @brief StartSceneGuid の SceneFile。未設定か見つからなければ nullptr */
        [[nodiscard]] static std::shared_ptr<Module::Asset::SceneFile> FindStartSceneFile();
        /** @brief 起動シーンのパス。未設定なら既定のシーン、設定したシーンが見つからなければ警告して既定のシーンを返す */
        [[nodiscard]] static std::string StartScenePath();
        [[nodiscard]] static const char* DefaultStartScenePath();
        [[nodiscard]] static std::vector<std::shared_ptr<Module::Asset::SceneFile>> CollectSceneFiles();

        [[nodiscard]] static BuildTargetConfiguration TargetConfiguration() { return targetConfiguration_; }
        static void SetTargetConfiguration(BuildTargetConfiguration targetConfiguration);
        /** @brief MSBuild の Configuration 名。x64 以下の出力フォルダ名でもある */
        [[nodiscard]] static const wchar_t* TargetConfigurationName(BuildTargetConfiguration targetConfiguration);

        [[nodiscard]] static const std::string& MsBuildPathUtf8() { return msBuildPath_; }
        static void SetMsBuildPath(const std::string& msBuildPath);
        /** @brief 未設定なら vswhere で見つけた MSBuild。見つからなければ空 */
        [[nodiscard]] static std::filesystem::path MsBuildPath();

        [[nodiscard]] static const std::string& OutputDirectoryUtf8() { return outputDirectory_; }
        static void SetOutputDirectory(const std::string& outputDirectory);
        /** @brief 相対パスは作業ディレクトリ (リポジトリルート) 基準で絶対パスにして返す */
        [[nodiscard]] static std::filesystem::path OutputDirectory();

        /** @brief 書き出したゲームに installed.json を置き、配信中のアセットへの更新を有効にするか */
        [[nodiscard]] static bool AssetUpdatesEnabled() { return assetUpdatesEnabled_; }
        static void SetAssetUpdatesEnabled(bool assetUpdatesEnabled);

        /** @brief ゲームに同梱する設定のフォルダ (リポジトリルートからの相対パス) */
        [[nodiscard]] static const wchar_t* RuntimeConfigDirectory();

    private:
        static std::string              productName_;
        static std::string              clientVersion_;
        static std::string              startSceneGuid_;
        static BuildTargetConfiguration targetConfiguration_;
        static std::string              msBuildPath_;
        static std::string              outputDirectory_;
        static bool                     assetUpdatesEnabled_;
    };
}
