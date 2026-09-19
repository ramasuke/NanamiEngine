#pragma once
#include <memory>

#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Asset/Sound/SoundFile.h"
#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "../../../../../../Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../../../../../Data/EventNotice/Data_EventBoard.h"
#include "../Model/EventBoardModel.h"
#include "../Model/NoticeBoardModel.h"
#include "../Model/QuestBoardModel.h"
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
        [[nodiscard]] BoardListCursor& CurrentCursor() const;
        [[nodiscard]] bool CanAcceptSelected() const;

        void SwitchTab(int delta);
        void SelectTab(EventBoardTabType type);
        void Accept();
        void Refresh();
        void Close();

        [[serialize(0)]] FIELD(Asset::EventBoardData) board_;
        [[serialize(0)]] FIELD(Asset::SoundFile) acceptSound_;

        std::shared_ptr<EventBoardUi> view_;
        std::unique_ptr<QuestBoardModel>  questModel_;
        std::unique_ptr<EventBoardModel>  eventModel_;
        std::unique_ptr<NoticeBoardModel> noticeModel_;
        EventBoardTabType currentTab_ = EventBoardTabType::Quest;
        std::weak_ptr<GameCore::IPlayerAvatar> suspendedAvatar_;

        Keys previousKeys_;
        bool isClosed_ = false;

        // 調べるたびに二重に生えるのを防ぐ
        static bool isOpen_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(board_));
            archive(CEREAL_NVP(acceptSound_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(board_));
            if (version >= 0) archive(CEREAL_NVP(acceptSound_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::EventBoardPresenter, 0)
