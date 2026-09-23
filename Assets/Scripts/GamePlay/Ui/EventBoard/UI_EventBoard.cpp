#include "UI_EventBoard.h"

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    std::string_view ToEventBoardTabLabel(const EventBoardTabType type)
    {
        switch (type)
        {
        case EventBoardTabType::Quest:  return "依 頼";
        case EventBoardTabType::Event:  return "催 し";
        case EventBoardTabType::Notice: return "お知らせ";
        case EventBoardTabType::Restoration: return "復 興";
        }
        return "";
    }

    template<typename PageT>
    std::weak_ptr<PageT> EventBoardUi::InstantiatePage(const FIELD(Asset::PrefabGameObjectFile)& prefab) const
    {
        if (!prefab || !pagesRoot_)
            return {};

        const auto pageObject = Scene::GameObject::Instantiate(*prefab.get(), pagesRoot_.get()).lock();
        if (!pageObject)
            return {};

        return pageObject->Components().Catch<PageT>();
    }

    void EventBoardUi::Build()
    {
        if (isBuilt_)
            return;
        isBuilt_ = true;

        if (tabPrefab_ && tabsRoot_)
        {
            const auto tabsObject = tabsRoot_.get();
            for (size_t i = 0; i < EVENT_BOARD_TAB_COUNT; ++i)
            {
                const auto tabObject = Scene::GameObject::Instantiate(*tabPrefab_.get(), tabsObject).lock();
                if (!tabObject)
                    continue;

                tabs_[i] = tabObject->Components().Catch<EventBoardTab>();
                if (const auto tab = tabs_[i].lock())
                {
                    tab->Place(glm::vec3(static_cast<float>(i) * tabSpacing_px_, 0.0f, 0.0f));
                    tab->SetLabel(std::string(ToEventBoardTabLabel(static_cast<EventBoardTabType>(i))));
                }
            }
        }

        questPage_  = InstantiatePage<EventBoardQuestPage>(questPagePrefab_);
        eventPage_  = InstantiatePage<EventBoardEventPage>(eventPagePrefab_);
        noticePage_ = InstantiatePage<EventBoardNoticePage>(noticePagePrefab_);
        restorationPage_ = InstantiatePage<EventBoardRestorationPage>(restorationPagePrefab_);
    }

    std::shared_ptr<EventBoardTab> EventBoardUi::Tab(const EventBoardTabType type) const
    {
        const auto index = static_cast<size_t>(type);
        return index < tabs_.size() ? tabs_[index].lock() : nullptr;
    }

    std::shared_ptr<GameObject::IGameObject> EventBoardUi::PageObject(const EventBoardTabType type) const
    {
        switch (type)
        {
        case EventBoardTabType::Quest:
            if (const auto page = QuestPage())
                return page->Entity().lock();
            return nullptr;
        case EventBoardTabType::Event:
            if (const auto page = EventPage())
                return page->Entity().lock();
            return nullptr;
        case EventBoardTabType::Notice:
            if (const auto page = NoticePage())
                return page->Entity().lock();
            return nullptr;
        case EventBoardTabType::Restoration:
            if (const auto page = RestorationPage())
                return page->Entity().lock();
            return nullptr;
        }
        return nullptr;
    }

    void EventBoardUi::ShowTab(const EventBoardTabType type) const
    {
        for (size_t i = 0; i < EVENT_BOARD_TAB_COUNT; ++i)
        {
            const auto tabType = static_cast<EventBoardTabType>(i);
            if (const auto tab = Tab(tabType))
                tab->SetSelected(tabType == type);
            if (const auto page = PageObject(tabType))
                page->SetEnable(tabType == type);
        }
        if (const auto veil = veil_.get())
            veil->SetEnable(type != EventBoardTabType::Restoration);
    }

    void EventBoardUi::ShowConfirmHint(const EventBoardConfirmHint hint) const
    {
        if (const auto hints = hintsWithAccept_.get())
            hints->SetEnable(hint == EventBoardConfirmHint::Accept);
        if (const auto hints = hintsWithRestore_.get())
            hints->SetEnable(hint == EventBoardConfirmHint::Restore);
        if (const auto hints = hintsWithoutAccept_.get())
            hints->SetEnable(hint == EventBoardConfirmHint::None);
    }

    void EventBoardUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("tabPrefab_", tabPrefab_);
        ImGuiHelper::OnDrawInputField("tabsRoot_", tabsRoot_);
        ImGuiHelper::OnDrawInputField("tabSpacing_px_", tabSpacing_px_);
        ImGuiHelper::OnDrawInputField("questPagePrefab_", questPagePrefab_);
        ImGuiHelper::OnDrawInputField("eventPagePrefab_", eventPagePrefab_);
        ImGuiHelper::OnDrawInputField("noticePagePrefab_", noticePagePrefab_);
        ImGuiHelper::OnDrawInputField("restorationPagePrefab_", restorationPagePrefab_);
        ImGuiHelper::OnDrawInputField("pagesRoot_", pagesRoot_);
        ImGuiHelper::OnDrawInputField("hintsWithAccept_", hintsWithAccept_);
        ImGuiHelper::OnDrawInputField("hintsWithoutAccept_", hintsWithoutAccept_);
        ImGuiHelper::OnDrawInputField("hintsWithRestore_", hintsWithRestore_);
        ImGuiHelper::OnDrawInputField("veil_", veil_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardUi);
#pragma endregion
