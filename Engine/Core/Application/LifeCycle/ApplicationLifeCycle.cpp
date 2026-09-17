#include "ApplicationLifeCycle.h"

namespace
{
    /** FieldInitStagingScope の中にいるスレッドだけが非 nullptr になる */
    thread_local std::vector<std::weak_ptr<NanamiEngine::Core::Object::IFieldContext>>* tlsApplicationLifeCycleFieldInitStaging = nullptr;
}

namespace NanamiEngine::Core::Application
{
    std::vector<std::weak_ptr<Object::IFieldContext>>* ApplicationLifeCycle::FieldInitStaging()
    {
        return tlsApplicationLifeCycleFieldInitStaging;
    }

    void ApplicationLifeCycle::AddStagedFieldInittables(
        const std::vector<std::weak_ptr<Object::IFieldContext>>& staged)
    {
        for (const auto& context : staged)
        {
            fieldInitableCallbacks_.Add(context);
        }
    }

    void ApplicationLifeCycle::OnUpdate()
    {
        fieldInitableCallbacks_.OnUpdatePushedContents();
        enableAssetCallbacks_  .OnUpdatePushedContents();

        fieldInitableCallbacks_.Invoke([](auto& callback) { callback.Init();           });
        enableAssetCallbacks_  .Invoke([](auto& callback) { callback.OnEnableAsset();  });
    }

    void ApplicationLifeCycle::OnUpdateFieldInittables()
    {
        fieldInitableCallbacks_.OnUpdatePushedContents();
        fieldInitableCallbacks_.Invoke([](auto& callback) { callback.Init();           });
    }

    void ApplicationLifeCycle::OnUpdateCopiedFieldInittables(const Object::GuidRemap& guidRemap)
    {
        fieldInitableCallbacks_.OnUpdatePushedContents();
        fieldInitableCallbacks_.Invoke([&guidRemap](auto& callback)
        {
            callback.RemapGuid(guidRemap);
            callback.Init();
        });
    }

    FieldInitStagingScope::FieldInitStagingScope(
        std::vector<std::weak_ptr<Object::IFieldContext>>& staging)
    {
        tlsApplicationLifeCycleFieldInitStaging = &staging;
    }

    FieldInitStagingScope::~FieldInitStagingScope()
    {
        tlsApplicationLifeCycleFieldInitStaging = nullptr;
    }
}
