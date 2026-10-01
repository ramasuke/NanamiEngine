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
#include "../../../Sound/UiSoundBank.h"
#include "Packages/UiFlow/UiFlow.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Ui
{
    class EventBoardPresenter final : public Component::ComponentBase,
                                      public LifeCycleCallback::IStartable,
                                      public LifeCycleCallback::IUpdatable
    {
    private:
        void OnStart  () override;
        void OnUpdate () override;
        void OnDestroy() override;

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
        void UpdatePreview();
        void EndPreview();
        void Close();

        [[serialize(0)]] FIELD(Asset::EventBoardData) board_;
        [[serialize(0)]] FIELD(Asset::SoundFile) acceptSound_;
        [[serialize(1)]] FIELD(Asset::SoundFile) restoreSound_;
        [[serialize(1)]] FIELD(Asset::SoundFile) refuseSound_;
        [[serialize(2)]] FIELD(Asset::UiSoundBankData) uiSounds_;

        std::shared_ptr<UiFlow::UiScreen> screen_;
        std::shared_ptr<EventBoardUi> view_;
        std::unique_ptr<QuestBoardModel>  questModel_;
        std::unique_ptr<EventBoardModel>  eventModel_;
        std::unique_ptr<NoticeBoardModel> noticeModel_;
        std::unique_ptr<RestorationBoardModel> restorationModel_;
        QuestReadLog questReadLog_;
        EventBoardTabType currentTab_ = EventBoardTabType::Quest;
        std::weak_ptr<GameCore::IPlayerAvatar> suspendedAvatar_;
        std::optional<GameCore::Story::Facility> previewFacility_;

        bool isClosed_ = false;

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
            archive(CEREAL_NVP(uiSounds_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(board_));
            if (version >= 0) archive(CEREAL_NVP(acceptSound_));
            if (version >= 1) archive(CEREAL_NVP(restoreSound_));
            if (version >= 1) archive(CEREAL_NVP(refuseSound_));
            if (version >= 2) archive(CEREAL_NVP(uiSounds_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::EventBoardPresenter, 2);
