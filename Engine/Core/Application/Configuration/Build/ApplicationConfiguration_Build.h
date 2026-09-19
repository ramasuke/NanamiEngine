#pragma once
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
     *        製品名と起動シーンはゲームも読むので ProjectConfig/Build/Runtime/ に置いて同梱し、残りはエディタ専用
     */
    class BuildConfiguration final
    {
    public:
        static void Load();
        static void Save();

        [[nodiscard]] static const std::string& ProductName() { return productName_; }
        static void SetProductName(const std::string& productName);
        /** @brief exe 名に使えない製品名なら理由を返す。使えるなら空文字 */
        [[nodiscard]] static std::string ValidateProductName(const std::string& productName);

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
        [[nodiscard]] static std::filesystem::path MsBuildPath();

        [[nodiscard]] static const std::string& OutputDirectoryUtf8() { return outputDirectory_; }
        static void SetOutputDirectory(const std::string& outputDirectory);
        /** @brief 相対パスは作業ディレクトリ (リポジトリルート) 基準で絶対パスにして返す */
        [[nodiscard]] static std::filesystem::path OutputDirectory();
        /** @brief ソースを複製してビルドする場所。Editor 版の中間ファイルと分けるため、OneDrive の外に置く */
        [[nodiscard]] static std::filesystem::path StagingDirectory();

        /** @brief ゲームに同梱する設定のフォルダ (リポジトリルートからの相対パス) */
        [[nodiscard]] static const wchar_t* RuntimeConfigDirectory();

    private:
        static std::string              productName_;
        static std::string              startSceneGuid_;
        static BuildTargetConfiguration targetConfiguration_;
        static std::string              msBuildPath_;
        static std::string              outputDirectory_;
    };
}
