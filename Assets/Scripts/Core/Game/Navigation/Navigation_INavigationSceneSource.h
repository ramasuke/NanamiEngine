#pragma once
#include <memory>
#include <optional>
#include <string_view>

#include "Packages/R4/R4.h"

namespace NanamiEngine::Module::Asset
{
    class HeightGridMap;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GameCore::Navigation
{
    /** @brief ナビの目的地。目印は object の位置から markerHeight だけ上 */
    struct NavigationTargetRef
    {
        std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject> object;
        float markerHeight = 2.0f;
    };

    /** @brief 今いるメインシーンがナビに渡すもの。シーンの種類は見せない */
    class INavigationSceneSource
    {
    public:
        virtual ~INavigationSceneSource() = default;

        /** @return 蛍の道を探す格子。無いシーンでは nullptr */
        [[nodiscard]] virtual std::shared_ptr<NanamiEngine::Module::Asset::HeightGridMap> NavigationGrid() const = 0;
        /** @return id の目的地がこのシーンに無ければ空 */
        [[nodiscard]] virtual std::optional<NavigationTargetRef> FindNavigationTarget(std::string_view id) const = 0;
        /** @brief シーンの中だけの一時的な目標(大砲が使えるなど)。保存しない */
        [[nodiscard]] virtual bool HasNavigationObjective(std::string_view id) const = 0;
        [[nodiscard]] virtual NanamiEngine::R4::Observable<NanamiEngine::R4::Unit> OnNavigationObjectivesChanged() const = 0;
    };
}
