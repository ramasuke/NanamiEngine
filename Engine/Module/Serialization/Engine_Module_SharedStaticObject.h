#pragma once
#include <cstddef>

#include "../../Core/Api/NanamiApi.h"
#include "../../Core/Api/NanamiModule.h"

// cereal の StaticObject<T> を全モジュールで共有する表の、モジュール単位の破棄口
// NOTE: CEREAL_NANAMI_SHARED_STATIC_OBJECT が無いビルドでは表は空
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
