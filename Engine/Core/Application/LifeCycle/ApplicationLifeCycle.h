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

        // NOTE: FieldInitStagingScope の中にいるスレッドだけ非 nullptr を返す
        static std::vector<std::weak_ptr<Object::IFieldContext>>* FieldInitStaging();
        // NOTE: 貯めておいた FIELD の初期化待ちを共有キューへ移す
        void AddStagedFieldInittables(const std::vector<std::weak_ptr<Object::IFieldContext>>& staged);
        // NOTE: 呼び出し待ちを全部捨てる (ゲーム DLL を外す前。weak_ptr の制御ブロックが DLL のコードを指しているため)
        void Clear();

    private:
        LifeCycleOnceCallbackGroup<Object::IFieldContext> fieldInitableCallbacks_;
        LifeCycleOnceCallbackGroup<Module::LifeCycleCallback::IEnablableAsset> enableAssetCallbacks_;
    };

    // NOTE: このスレッドで積まれた FIELD の初期化待ちを、共有キューではなく staging に貯める
    // WARNING: 共有キューに直接積むと、未登録の GameObject を解決して参照が null のまま確定する
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
