#pragma once
#include "../cereal/include/cereal/cereal.hpp"
#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::LifeCycleCallback
{
    /**
     * @brief インスタンス生成後、次フレームの開始前に一度だけ呼ばれるOnAwake()を実装するインターフェース
     *
     * @details
     * 呼び出し順序:
     * - IAwakableインスタンスの生成
     * - 次フレームの開始時にIAwakable::OnAwake()の呼び出し（1回だけ）
     */
    class IAwakable : public virtual Object::IObject
    {
    public:
        virtual ~IAwakable() = default;
        /**@brief インスタンス生成後、次フレームに呼ばれる初期化処理 */
        virtual void OnAwake() = 0;
        
        template <class Archive> void save(Archive& archive, const std::uint32_t version) const { }
        template <class Archive> void load(Archive& archive, const std::uint32_t version)       { }
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IAwakable, 0);