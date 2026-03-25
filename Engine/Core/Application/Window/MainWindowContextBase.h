#pragma once
#include "../../../../../Module/Guid/Guid.h"
#include "../../../../../Resource/Object/Interface/IObject.h"
#include "../../../LifeCycle/Window/WindowLifeCycle.h"
#include "Interface/IMainWindow.h"

namespace NanamiEngine::Core::MainWindow
{
    template <typename ContentT> requires std::derived_from<ContentT, Module::Object::IObject>
    class MainWindowContextBase : public IMainWindow
    {
    public:
        void AddContent(const std::shared_ptr<ContentT>& add);
        void RemoveContent(const std::shared_ptr<ContentT>& remove);

    protected:
        std::unordered_map<Guid, std::shared_ptr<ContentT>, GuidHash> contents_;

    private:
        void OnDrawGui() override = 0;
        void OnUpdateContext() override;
        virtual void OnUpdate();

        [[nodiscard]] Application::WindowLifeCycle& LifeCycle() override { return lifeCycle_; }

        Application::WindowLifeCycle lifeCycle_;
    };

    template <typename ContentT> requires std::derived_from<ContentT, Module::Object::IObject>
    void MainWindowContextBase<ContentT>::OnUpdateContext()
    {
        lifeCycle_.OnUpdate();
        OnUpdate();
    }

    template <typename ContentT> requires std::derived_from<ContentT, Module::Object::IObject>
    void MainWindowContextBase<ContentT>::OnUpdate()
    {
    }

    template <typename ContentT> requires std::derived_from<ContentT, Module::Object::IObject>
    void MainWindowContextBase<ContentT>::AddContent(const std::shared_ptr<ContentT>& add)
    {
        contents_[add->GetGuid()] = add;
    }

    template <typename ContentT> requires std::derived_from<ContentT, Module::Object::IObject>
    void MainWindowContextBase<ContentT>::RemoveContent(const std::shared_ptr<ContentT>& remove)
    {
        contents_.erase(remove->GetGuid());
    }
}
