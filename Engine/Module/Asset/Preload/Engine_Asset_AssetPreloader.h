#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace NanamiEngine::Module::Asset
{
    /**
     * @brief シーンファイルから GUID でたどれるアセットを先読みし、開いているどのシーンからも
     *        参照されないアセットを解放する。対象は IPreloadableAsset を実装したアセットだけ
     */
    class NANAMI_API AssetPreloader final
    {
    public:
        /** @brief GUID 文字列 → contentPath */
        using Index = std::unordered_map<std::string, std::string>;

        /** @brief 登録済みの全アセットの表を作る。メインスレッド専用 */
        [[nodiscard]] static Index BuildIndex();
        /**
         * @brief sceneFilePath から参照をたどって届くアセットの GUID を集める。
         *        ファイルを読むだけなのでワーカースレッドからも呼べる。ほかの .scene の先へはたどらない
         */
        [[nodiscard]] static std::vector<std::string> CollectDependencies(const Index& index, const std::string& sceneFilePath);
        /** @brief 非同期で読み込みを要求する。メインスレッド専用 */
        static void RequestLoads(const std::vector<std::string>& guids);
        /** @brief BuildIndex → CollectDependencies → RequestLoads をまとめて行う。メインスレッド専用 */
        static void RequestForScene(const std::string& sceneFilePath);
        /** @brief keptSceneFilePaths のどれからも届かない、読み込み済みのアセットを解放する。メインスレッド専用 */
        static void ReleaseUnused(const std::vector<std::string>& keptSceneFilePaths);
    };
}
