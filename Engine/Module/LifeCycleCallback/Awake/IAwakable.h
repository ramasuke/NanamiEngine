#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../cereal/include/cereal/cereal.hpp"
#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class NANAMI_API IAwakable : public virtual Object::IObject
    {
    public:
        virtual ~IAwakable() = default;
        // NOTE: 生成後、次フレームの開始前に一度だけ呼ばれる初期化処理
        virtual void OnAwake() = 0;
        
        template <class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template <class Archive> void load(Archive& archive, const std::uint32_t version)       { }
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IAwakable, 0);