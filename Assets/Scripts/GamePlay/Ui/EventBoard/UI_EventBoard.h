#pragma once
#include <array>
#include <memory>
#include <string_view>

#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "Page/Ui_EventBoard_EventPage.h"
#include "Page/Ui_EventBoard_NoticePage.h"
#include "Page/Ui_EventBoard_QuestPage.h"
#include "Tab/Ui_EventBoard_Tab.h"

namespace GamePlay::Ui
{
    /** @brief 木札の並び順 */
    enum class EventBoardTabType : int
    {
        Quest = 0,
        Event,
        Notice,
    };

    constexpr size_t EVENT_BOARD_TAB_COUNT = 3;

    [[nodiscard]] std::string_view ToEventBoardTabLabel(EventBoardTabType type);

    /**
     * @brief 掲示板の見た目のまとめ役。上に木札の見出しを吊り、下に選んだ見出しの頁(依頼 / 催し / お知らせ)だけを出す。
     * 見出しと頁はそれぞれ自分の prefab を持ち、ここで生成する。
     */
    class EventBoardUi final : public Component::ComponentBase
    {
    public:
        /** @brief 見出しと頁を作る。2回目以降は何もしない */
        void Build();

        [[nodiscard]] std::shared_ptr<EventBoardQuestPage>  QuestPage () const { return questPage_ .lock(); }
        [[nodiscard]] std::shared_ptr<EventBoardEventPage>  EventPage () const { return eventPage_ .lock(); }
        [[nodiscard]] std::shared_ptr<EventBoardNoticePage> NoticePage() const { return noticePage_.lock(); }
        [[nodiscard]] std::shared_ptr<EventBoardTab>        Tab(EventBoardTabType type) const;

        /**
         * @brief 選んだ見出しを垂らし、その頁だけを出す。
         * 頁を出し直すと子の部品が全部有効に戻るので、呼んだ側はこのあと頁を Bind し直すこと
         */
        void ShowTab(EventBoardTabType type) const;
        /** @brief 操作ガイドに「A 受注する」を出すか */
        void ShowAcceptHint(bool canAccept) const;

    private:
        template<typename PageT>
        std::weak_ptr<PageT> InstantiatePage(const FIELD(Asset::PrefabGameObjectFile)& prefab) const;
        [[nodiscard]] std::shared_ptr<GameObject::IGameObject> PageObject(EventBoardTabType type) const;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) tabPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) tabsRoot_;
        [[serialize(0)]] float tabSpacing_px_ = 178.0f;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) questPagePrefab_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) eventPagePrefab_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) noticePagePrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) pagesRoot_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) hintsWithAccept_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) hintsWithoutAccept_;

        std::array<std::weak_ptr<EventBoardTab>, EVENT_BOARD_TAB_COUNT> tabs_;
        std::weak_ptr<EventBoardQuestPage>  questPage_;
        std::weak_ptr<EventBoardEventPage>  eventPage_;
        std::weak_ptr<EventBoardNoticePage> noticePage_;
        bool isBuilt_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(tabPrefab_));
            archive(CEREAL_NVP(tabsRoot_));
            archive(CEREAL_NVP(tabSpacing_px_));
            archive(CEREAL_NVP(questPagePrefab_));
            archive(CEREAL_NVP(eventPagePrefab_));
            archive(CEREAL_NVP(noticePagePrefab_));
            archive(CEREAL_NVP(pagesRoot_));
            archive(CEREAL_NVP(hintsWithAccept_));
            archive(CEREAL_NVP(hintsWithoutAccept_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(tabPrefab_));
            if (version >= 0) archive(CEREAL_NVP(tabsRoot_));
            if (version >= 0) archive(CEREAL_NVP(tabSpacing_px_));
            if (version >= 0) archive(CEREAL_NVP(questPagePrefab_));
            if (version >= 0) archive(CEREAL_NVP(eventPagePrefab_));
            if (version >= 0) archive(CEREAL_NVP(noticePagePrefab_));
            if (version >= 0) archive(CEREAL_NVP(pagesRoot_));
            if (version >= 0) archive(CEREAL_NVP(hintsWithAccept_));
            if (version >= 0) archive(CEREAL_NVP(hintsWithoutAccept_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardUi, 0)
