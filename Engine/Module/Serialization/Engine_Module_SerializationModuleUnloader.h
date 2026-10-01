#pragma once
#include <cstddef>

#include "../../Core/Api/NanamiApi.h"
#include "../../Core/Api/NanamiModule.h"

// ゲーム DLL が cereal に登録した多相型を表から消す
// WARNING: 順序はインスタンス破棄 → Unregister → FreeLibrary → ClearClassVersions
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
        static ModuleUnloadReport Unregister(Core::ModuleHandle module);
        static void               ClearClassVersions();
        /** @brief PolymorphicCasters に vtable が module にある caster が残っている数 */
        [[nodiscard]] static std::size_t CountLeftoverCasters(Core::ModuleHandle module);
    };
}
