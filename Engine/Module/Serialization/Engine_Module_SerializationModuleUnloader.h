#pragma once
#include <cstddef>

#include "../../Core/Api/NanamiApi.h"

// ゲーム DLL をアンロードする前に、その DLL が cereal に登録した多相型を表から消す (docs/HotReload.md §3.2 の記録方式)。
// 消す対象は SerializationTypeRegistry の記録 (name / type / base) から引き、保険として caster の vtable が
// その DLL にあるものも掃く。手順:
//   1. 型のインスタンスをすべて破棄する (シーン、アセット、ウィンドウ ...)
//   2. Unregister(module)            <- FreeLibrary の前
//   3. FreeLibrary
//   4. ClearClassVersions()          <- 次の LoadLibrary の前 (cereal の Versions は emplace なので古い番号が残る)
namespace NanamiEngine::Module::Serialization
{
    struct NANAMI_API ModuleUnloadReport
    {
        std::size_t inputBindings  = 0; // InputBindingMap (JSON + PortableBinary) から消した数
        std::size_t outputBindings = 0; // OutputBindingMap (JSON + PortableBinary) から消した数
        std::size_t casters        = 0; // PolymorphicCasters::map から消した (base, derived) の数
        std::size_t sweptCasters   = 0; // 記録に無かったが vtable がその DLL にあったので消した数
        std::size_t records        = 0; // SerializationTypeRegistry から消した記録の数
        std::size_t sharedStatics  = 0; // その DLL が作った cereal::StaticObject の実体で捨てた数
    };

    class NANAMI_API SerializationModuleUnloader final
    {
    public:
        static ModuleUnloadReport Unregister(const void* module);
        static void               ClearClassVersions();
        /** @brief PolymorphicCasters に vtable が module にある caster が残っているか (Debug の取り残し assert 用) */
        [[nodiscard]] static std::size_t CountLeftoverCasters(const void* module);
    };
}
