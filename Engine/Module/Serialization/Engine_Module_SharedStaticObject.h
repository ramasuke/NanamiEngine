#pragma once
#include <cstddef>

#include "../../Core/Api/NanamiApi.h"
#include "../../Core/Api/NanamiModule.h"

// cereal の StaticObject<T> を全モジュール (exe / dll) で 1 つにするための表 (docs/HotReload.md §3.2)。
// 実体の出し入れは Libs/cereal/.../static_object.hpp のパッチが cereal::detail::nanami_shared_static_object() 経由で行い、
// ここにはゲーム DLL のアンロード時に「そのモジュールが作った実体」を捨てるための入口だけを置く。
// CEREAL_NANAMI_SHARED_STATIC_OBJECT が定義されていないビルド (今の静的 lib) では表は空のまま。
namespace NanamiEngine::Module::Serialization
{
    class NANAMI_API SharedStaticObjects final
    {
    public:
        /** @brief module が作った実体を (module がまだロードされているうちに) 破棄して表から外す。戻り値は捨てた数 */
        static std::size_t ReleaseOwnedBy(Core::ModuleHandle module);
        /** @brief module が作った実体の数 (アンロード前の取り残し確認用) */
        [[nodiscard]] static std::size_t CountOwnedBy(Core::ModuleHandle module);
        [[nodiscard]] static std::size_t Count();
    };
}
