#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace NanamiEngine::Module::Asset
{
    // NOTE: シーンから GUID でたどれる IPreloadableAsset を先読みし、どのシーンからも届かないものを解放する
    class NANAMI_API AssetPreloader final
    {
    public:
        // NOTE: GUID 文字列 → contentPath
        using Index = std::unordered_map<std::string, std::string>;

        // NOTE: 登録済みの全アセットの表を作る。メインスレッド専用
        [[nodiscard]] static Index BuildIndex();
        // NOTE: sceneFilePath から届くアセットの GUID を集める。ワーカースレッド可。ほかの .scene はたどらない
        [[nodiscard]] static std::vector<std::string> CollectDependencies(const Index& index, const std::string& sceneFilePath);
        // NOTE: 非同期で読み込みを要求する。メインスレッド専用
        static void RequestLoads(const std::vector<std::string>& guids);
        // NOTE: BuildIndex → CollectDependencies → RequestLoads をまとめて行う。メインスレッド専用
        static void RequestForScene(const std::string& sceneFilePath);
        // NOTE: keptSceneFilePaths のどれからも届かない、読み込み済みのアセットを解放する。メインスレッド専用
        static void ReleaseUnused(const std::vector<std::string>& keptSceneFilePaths);
    };
}
