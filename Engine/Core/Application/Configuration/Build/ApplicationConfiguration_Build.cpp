#include "ApplicationConfiguration_Build.h"

#include <algorithm>
#include <cstdlib>

#include "../../ApplicationBase.h"
#include "../../../FileSystem/Directory/Directory.h"
#include "../../../../Module/Asset/Scene/SceneFile.h"
#include "../../../../Module/Exception/Engine_Module_Exception.h"
#include "../../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../../../Module/ProjectConfig/Engine_Module_ProjectConfig.h"

namespace NanamiEngine::Core::Application::Configuration
{
    constexpr auto BUILD_DEFAULT_PRODUCT_NAME     = "NanamiEngine";
    constexpr auto BUILD_DEFAULT_START_SCENE_PATH = "Assets/Scene/GameManage.scene";
    constexpr auto BUILD_DEFAULT_MSBUILD_PATH     = R"(C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe)";
    constexpr auto BUILD_DEFAULT_OUTPUT_DIRECTORY = "Build/Game";

    std::string              BuildConfiguration::productName_         = BUILD_DEFAULT_PRODUCT_NAME;
    std::string              BuildConfiguration::startSceneGuid_;
    BuildTargetConfiguration BuildConfiguration::targetConfiguration_ = BuildTargetConfiguration::Release;
    std::string              BuildConfiguration::msBuildPath_         = BUILD_DEFAULT_MSBUILD_PATH;
    std::string              BuildConfiguration::outputDirectory_     = BUILD_DEFAULT_OUTPUT_DIRECTORY;

    constexpr auto BUILD_CONFIG_PATH          = "Build/";
    // RuntimeConfigDirectory() と揃える
    constexpr auto BUILD_RUNTIME_CONFIG_PATH  = "Build/Runtime/";
    constexpr auto BUILD_PRODUCT_NAME_KEY     = "ProductName";
    constexpr auto BUILD_START_SCENE_GUID_KEY = "StartSceneGuid";
    constexpr auto BUILD_CONFIGURATION_KEY    = "Configuration";
    constexpr auto BUILD_MSBUILD_PATH_KEY     = "MsBuildPath";
    constexpr auto BUILD_OUTPUT_DIRECTORY_KEY = "OutputDirectory";

    namespace
    {
        // ImGui の入力は UTF-8 なので、ACP ではなく UTF-8 として wide 文字列にする
        std::filesystem::path BuildConfigUtf8ToPath(const std::string& utf8)
        {
            return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
        }

        std::string BuildConfigTargetToString(const BuildTargetConfiguration targetConfiguration)
        {
            return targetConfiguration == BuildTargetConfiguration::Debug ? "Debug" : "Release";
        }

        void BuildConfigCollectSceneFiles(FileSystem::Directory& directory, std::vector<std::shared_ptr<Module::Asset::SceneFile>>& outSceneFiles)
        {
            for (auto& file : directory.Files())
            {
                if (auto sceneFile = std::dynamic_pointer_cast<Module::Asset::SceneFile>(file.GetContent()))
                    outSceneFiles.push_back(std::move(sceneFile));
            }
            for (auto& child : directory.GetDirectories())
            {
                BuildConfigCollectSceneFiles(child, outSceneFiles);
            }
        }
    }

    void BuildConfiguration::Load()
    {
        productName_     = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>(BUILD_RUNTIME_CONFIG_PATH, BUILD_PRODUCT_NAME_KEY,     std::string(BUILD_DEFAULT_PRODUCT_NAME));
        startSceneGuid_  = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>(BUILD_RUNTIME_CONFIG_PATH, BUILD_START_SCENE_GUID_KEY, std::string());
        msBuildPath_     = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>(BUILD_CONFIG_PATH,         BUILD_MSBUILD_PATH_KEY,     std::string(BUILD_DEFAULT_MSBUILD_PATH));
        outputDirectory_ = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>(BUILD_CONFIG_PATH,         BUILD_OUTPUT_DIRECTORY_KEY, std::string(BUILD_DEFAULT_OUTPUT_DIRECTORY));

        const std::string configuration = Module::ProjectConfig::LoadOrDefaultWithPath<std::string>(BUILD_CONFIG_PATH, BUILD_CONFIGURATION_KEY, BuildConfigTargetToString(BuildTargetConfiguration::Release));
        if (configuration == BuildConfigTargetToString(BuildTargetConfiguration::Debug))
        {
            targetConfiguration_ = BuildTargetConfiguration::Debug;
        }
        else
        {
            if (configuration != BuildConfigTargetToString(BuildTargetConfiguration::Release))
                Module::LogWarning("BuildConfiguration: 不明な Configuration \"" + configuration + "\" なので Release を使います");
            targetConfiguration_ = BuildTargetConfiguration::Release;
        }
    }

    void BuildConfiguration::Save()
    {
        try
        {
            Module::ProjectConfig::SaveWithPath<std::string>(BUILD_RUNTIME_CONFIG_PATH, BUILD_PRODUCT_NAME_KEY,     productName_);
            Module::ProjectConfig::SaveWithPath<std::string>(BUILD_RUNTIME_CONFIG_PATH, BUILD_START_SCENE_GUID_KEY, startSceneGuid_);
            Module::ProjectConfig::SaveWithPath<std::string>(BUILD_CONFIG_PATH,         BUILD_CONFIGURATION_KEY,    BuildConfigTargetToString(targetConfiguration_));
            Module::ProjectConfig::SaveWithPath<std::string>(BUILD_CONFIG_PATH,         BUILD_MSBUILD_PATH_KEY,     msBuildPath_);
            Module::ProjectConfig::SaveWithPath<std::string>(BUILD_CONFIG_PATH,         BUILD_OUTPUT_DIRECTORY_KEY, outputDirectory_);
        }
        catch (const Module::Exception::NanamiException& exception)
        {
            Module::LogError("BuildConfiguration: ビルド設定を保存できませんでした: " + std::string(exception.what()));
        }
    }

    void BuildConfiguration::SetProductName(const std::string& productName)
    {
        if (productName_ == productName)
            return;
        productName_ = productName;
        Save();
    }

    std::string BuildConfiguration::ValidateProductName(const std::string& productName)
    {
        if (productName.empty())
            return "Product name is empty";
        if (productName.find_first_of("\\/:*?\"<>|") != std::string::npos)
            return "Product name can't contain \\ / : * ? \" < > |";
        if (std::ranges::any_of(productName, [](const char c) { return static_cast<unsigned char>(c) < 0x20; }))
            return "Product name can't contain control characters";
        if (productName.back() == '.' || productName.back() == ' ')
            return "Product name can't end with '.' or a space";
        return {};
    }

    void BuildConfiguration::SetStartSceneGuid(const std::string& startSceneGuid)
    {
        if (startSceneGuid_ == startSceneGuid)
            return;
        startSceneGuid_ = startSceneGuid;
        Save();
    }

    std::shared_ptr<Module::Asset::SceneFile> BuildConfiguration::FindStartSceneFile()
    {
        if (startSceneGuid_.empty())
            return nullptr;

        for (auto& sceneFile : CollectSceneFiles())
        {
            if (sceneFile->GetGuid().Value() == startSceneGuid_)
                return sceneFile;
        }
        return nullptr;
    }

    std::string BuildConfiguration::StartScenePath()
    {
        if (startSceneGuid_.empty())
            return BUILD_DEFAULT_START_SCENE_PATH;

        if (const auto sceneFile = FindStartSceneFile())
            return sceneFile->GetContentPath();

        Module::LogWarning("BuildConfiguration: 起動シーン (" + startSceneGuid_ + ") が見つからないので " + BUILD_DEFAULT_START_SCENE_PATH + " を開きます");
        return BUILD_DEFAULT_START_SCENE_PATH;
    }

    const char* BuildConfiguration::DefaultStartScenePath()
    {
        return BUILD_DEFAULT_START_SCENE_PATH;
    }

    std::vector<std::shared_ptr<Module::Asset::SceneFile>> BuildConfiguration::CollectSceneFiles()
    {
        std::vector<std::shared_ptr<Module::Asset::SceneFile>> sceneFiles;
        BuildConfigCollectSceneFiles(ApplicationBase::AssetsDirectory(), sceneFiles);
        std::ranges::sort(sceneFiles, [](const auto& lhs, const auto& rhs) { return lhs->GetContentPath() < rhs->GetContentPath(); });
        return sceneFiles;
    }

    void BuildConfiguration::SetTargetConfiguration(const BuildTargetConfiguration targetConfiguration)
    {
        if (targetConfiguration_ == targetConfiguration)
            return;
        targetConfiguration_ = targetConfiguration;
        Save();
    }

    const wchar_t* BuildConfiguration::TargetConfigurationName(const BuildTargetConfiguration targetConfiguration)
    {
        return targetConfiguration == BuildTargetConfiguration::Debug ? L"Debug" : L"Release";
    }

    void BuildConfiguration::SetMsBuildPath(const std::string& msBuildPath)
    {
        if (msBuildPath_ == msBuildPath)
            return;
        msBuildPath_ = msBuildPath;
        Save();
    }

    std::filesystem::path BuildConfiguration::MsBuildPath()
    {
        return BuildConfigUtf8ToPath(msBuildPath_);
    }

    void BuildConfiguration::SetOutputDirectory(const std::string& outputDirectory)
    {
        if (outputDirectory_ == outputDirectory)
            return;
        outputDirectory_ = outputDirectory;
        Save();
    }

    std::filesystem::path BuildConfiguration::OutputDirectory()
    {
        return std::filesystem::absolute(BuildConfigUtf8ToPath(outputDirectory_)).lexically_normal();
    }

    std::filesystem::path BuildConfiguration::StagingDirectory()
    {
        std::filesystem::path localAppData;
        wchar_t* value = nullptr;
        size_t   length = 0;
        if (_wdupenv_s(&value, &length, L"LOCALAPPDATA") == 0 && value != nullptr)
        {
            localAppData = value;
        }
        free(value);

        if (localAppData.empty())
        {
            localAppData = std::filesystem::temp_directory_path();
        }
        return localAppData / L"NanamiEngine" / L"GameBuild" / L"Staging";
    }

    const wchar_t* BuildConfiguration::RuntimeConfigDirectory()
    {
        return L"ProjectConfig/Build/Runtime";
    }
}
