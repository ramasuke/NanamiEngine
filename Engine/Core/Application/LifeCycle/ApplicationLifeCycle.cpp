#include "ApplicationLifeCycle.h"

void NanamiEngine::Core::Application::ApplicationLifeCycle::OnUpdate()
{
    fieldInitableCallbacks_.OnUpdatePushedContents();
    enableAssetCallbacks_  .OnUpdatePushedContents();
    
    fieldInitableCallbacks_.Invoke([](auto& callback) { callback.Init();           });
    enableAssetCallbacks_  .Invoke([](auto& callback) { callback.OnEnableAsset();  });
}

void NanamiEngine::Core::Application::ApplicationLifeCycle::OnUpdateFieldInittables()
{
    fieldInitableCallbacks_.OnUpdatePushedContents();
    fieldInitableCallbacks_.Invoke([](auto& callback) { callback.Init();           });
}
