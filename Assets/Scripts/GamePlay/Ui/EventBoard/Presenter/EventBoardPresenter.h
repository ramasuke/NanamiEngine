#pragma once
#include <memory>
#include <optional>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../../../Data/EventNotice/Data_EventBoard.h"
#include "../Model/EventBoardModel.h"
#include "../Model/NoticeBoardModel.h"
#include "../Model/QuestBoardModel.h"
#include "../Model/RestorationBoardModel.h"
#include "../UI_EventBoard.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Ui
{
    /**
     * @brief 掲示板UIの開閉と入力。依頼・催し・お知らせはこのプレハブが持つ .eventBoard から読む。
     * 依頼は A でその場で受け、プレイヤーのクエストに入れて保存する。
     * 復興は A でその場でお金を払って直し、StoryProgress に残す(島の見た目は RestorationGate が変える)。
     * 復興の頁で施設を選んでいる間は、その施設の RestorationGate に下見をさせる(カメラが寄り、直った姿が建つ)。
     */
    class EventBoardPresenter final : public Component::ComponentBase,
                                      public LifeCycleCallback::IStartable,
                                      public LifeCycleCallback::IUpdatable
    {
    private:
        struct Keys
        {
            bool prev    = false;
            bool next    = false;
            bool tabPrev = false;
            bool tabNext = false;
            bool confirm = false;
            bool cancel  = false;
        };

        void OnStart  () override;
        void OnUpdate () override;
        void OnDestroy() override;

        [[nodiscard]] static Keys ReadKeys();
        [[nodiscard]] bool IsAnotherOpen() const;
        [[nodiscard]] BoardListCursor& CurrentCursor() const;
        [[nodiscard]] bool CanAcceptSelected() const;
        [[nodiscard]] bool CanRestoreSelected() const;
        [[nodiscard]] EventBoardConfirmHint ConfirmHint() const;

        void SwitchTab(int delta);
        void SelectTab(EventBoardTabType type);
        void Confirm();
        void AcceptQuest();
        void RestoreFacility();
        void PlaySe(const FIELD(Asset::SoundFile)& sound) const;
        void Refresh();
        /** @brief 今の頁と選択に合わせて、下見する施設を切り替える */
        void UpdatePreview();
        void EndPreview();
        void Close();

        [[serialize(0)]] FIELD(Asset::EventBoardData) board_;
        [[serialize(0)]] FIELD(Asset::SoundFile) acceptSound_;
        [[serialize(1)]] FIELD(Asset::SoundFile) restoreSound_;
        [[serialize(1)]] FIELD(Asset::SoundFile) refuseSound_;

        std::shared_ptr<EventBoardUi> view_;
        std::unique_ptr<QuestBoardModel>  questModel_;
        std::unique_ptr<EventBoardModel>  eventModel_;
        std::unique_ptr<NoticeBoardModel> noticeModel_;
        std::unique_ptr<RestorationBoardModel> restorationModel_;
        EventBoardTabType currentTab_ = EventBoardTabType::Quest;
        std::weak_ptr<GameCore::IPlayerAvatar> suspendedAvatar_;
        std::optional<GameCore::Story::Facility> previewFacility_;

        Keys previousKeys_;
        bool isClosed_ = false;
        // 調べるたびに二重に生えるのを防ぐ
        bool isOpen_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(board_));
            archive(CEREAL_NVP(acceptSound_));
            archive(CEREAL_NVP(restoreSound_));
            archive(CEREAL_NVP(refuseSound_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(board_));
            if (version >= 0) archive(CEREAL_NVP(acceptSound_));
            if (version >= 1) archive(CEREAL_NVP(restoreSound_));
            if (version >= 1) archive(CEREAL_NVP(refuseSound_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::EventBoardPresenter, 1);
