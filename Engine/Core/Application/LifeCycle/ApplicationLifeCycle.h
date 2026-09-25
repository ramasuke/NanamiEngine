#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>
#include <vector>

#include "../../../Module/LifeCycleCallback/EnableAsset/IEnablableAsset.h"
#include "../../../Module/LifeCycleCallback/Group/OnceCallbackGroup/LifeCycleOnceCallbackGroup.h"
#include "../../Object/Field/Interface/IFieldContext.h"

namespace NanamiEngine::Core::Application
{
    class NANAMI_API ApplicationLifeCycle final
    {
    public:
        void OnUpdate();
        void OnUpdateFieldInittables();
        void OnUpdateCopiedFieldInittables(const Object::GuidRemap& guidRemap);

        template<typename T>
        void AddCallback(std::weak_ptr<T> add);

        /** @brief FieldInitStagingScope の中にいるスレッドだけ非 nullptr を返す */
        static std::vector<std::weak_ptr<Object::IFieldContext>>* FieldInitStaging();
        /** @brief 貯めておいた FIELD の初期化待ちを共有キューへ移す */
        void AddStagedFieldInittables(const std::vector<std::weak_ptr<Object::IFieldContext>>& staged);

    private:
        LifeCycleOnceCallbackGroup<Object::IFieldContext> fieldInitableCallbacks_;
        LifeCycleOnceCallbackGroup<Module::LifeCycleCallback::IEnablableAsset> enableAssetCallbacks_;
    };

    /**
     * @brief このスレッドで積まれた FIELD の初期化待ちを、共有キューではなく staging に貯める。
     *        ワーカースレッドで .scene をデシリアライズする間だけ使う。
     *        共有キューに直接積むと、まだ ObjectRegistry に登録されていない GameObject を
     *        メインスレッドが解決しようとして、参照が null のまま確定してしまう
     *        （LifeCycleOnceCallbackGroup は 1 度しか Invoke しない）
     */
    class NANAMI_API FieldInitStagingScope final
    {
    public:
        explicit FieldInitStagingScope(std::vector<std::weak_ptr<Object::IFieldContext>>& staging);
        ~FieldInitStagingScope();
        FieldInitStagingScope(const FieldInitStagingScope&)            = delete;
        FieldInitStagingScope& operator=(const FieldInitStagingScope&) = delete;
    };

    template <typename T>
    void ApplicationLifeCycle::AddCallback(std::weak_ptr<T> add)
    {
        if constexpr (std::derived_from<T, Module::LifeCycleCallback::IEnablableAsset>)
        {
            enableAssetCallbacks_.Add(add);
        }
        if constexpr (std::derived_from<T, Object::IFieldContext>)
        {
            if (auto* staging = FieldInitStaging())
                staging->push_back(add);
            else
                fieldInitableCallbacks_.Add(add);
        }
    }
}
