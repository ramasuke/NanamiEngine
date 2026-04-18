#pragma once
#include "../cereal/include/cereal/cereal.hpp"
#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    /**
    * @brief IAwakable::OnAwake()の後に一度だけ呼ばれるOnStart()を実装するインターフェース
    *
    * @details
    * 呼び出し順序:
    * - IAwakable::OnAwake() の後に呼ばれる（1回だけ）
    * 使用目的:
    * - Awake 後の初期化処理
    */
    class IStartable : public virtual Object::IObject
    {
    public:
        virtual ~IStartable() = default;
        /** @brief インスタンス生成後、次フレームにOnAwakeより後に呼ばれる初期化処理 */
        virtual void OnStart() = 0;
        
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const { }
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)       { }
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IStartable, 0);