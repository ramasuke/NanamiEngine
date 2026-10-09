#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../cereal/include/cereal/cereal.hpp"
#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    // NOTE: OnAwake の後に一度だけ呼ばれる OnStart() を実装するインターフェース
    class NANAMI_API IStartable : public virtual Object::IObject
    {
    public:
        virtual ~IStartable() = default;
        virtual void OnStart() = 0;
        
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const { }
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)       { }
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IStartable, 0);