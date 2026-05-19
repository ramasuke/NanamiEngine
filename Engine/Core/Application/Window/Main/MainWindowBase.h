#pragma once
#include "../../../../Module/Guid/Guid.h"
#include "../../../Object/IObject.h"
#include "../LifeCycle/WindowLifeCycle.h"
#include "DrawGuiContext/MainWindowDrawGuiContext.h"
#include "Interface/IMainWindow.h"

namespace NanamiEngine::Core::MainWindow
{
    template <typename ContentT>
    requires std::derived_from<ContentT, Module::Object::IObject>
    class MainWindowBase : public IMainWindow
    {
    public:
        explicit MainWindowBase(bool useShadowMap);
        virtual void AddContent(const std::shared_ptr<ContentT>& content);
        void RemoveContent(const std::shared_ptr<ContentT>& content);
        [[nodiscard]] Application::WindowLifeCycle& LifeCycle() override { return lifeCycle_; }
        
    protected:
        std::unordered_map<Guid, std::shared_ptr<ContentT>, GuidHash> contents_;

    private:
        void OnSave() override = 0;
        void OnDrawGui(MainWindowDrawGuiContext context) override = 0;

        Application::WindowLifeCycle lifeCycle_;
    };

    template <typename ContentT> requires std::derived_from<ContentT, Module::Object::IObject>
    MainWindowBase<ContentT>::MainWindowBase(const bool useShadowMap)
        : lifeCycle_(useShadowMap)
    {
        
    }

    template <typename ContentT> requires std::derived_from<ContentT, Module::Object::IObject>
    void MainWindowBase<ContentT>::AddContent(const std::shared_ptr<ContentT>& content)
    {
        contents_[content->GetGuid()] = content;
    }

    template <typename ContentT> requires std::derived_from<ContentT, Module::Object::IObject>
    void MainWindowBase<ContentT>::RemoveContent(const std::shared_ptr<ContentT>& content)
    {
        contents_.erase(content->GetGuid());
    }
}
