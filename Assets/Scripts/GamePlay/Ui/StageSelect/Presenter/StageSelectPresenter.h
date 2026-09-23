#pragma once
#include <string>

#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../Model/StageSelectModel.h"
#include "../../../Network/Relay/RelayRoom.h"

namespace GamePlay::Ui
{
    class StageSelectUi;
    class StageSelectRoomUi;
}

namespace GamePlay::Ui
{
    class StageSelectPresenter final : public Component::ComponentBase,
                                       public LifeCycleCallback::IStartable,
                                       public LifeCycleCallback::IUpdatable
    {
    private:
        /** @brief 押した瞬間だけを拾うための、このフレームの入力 */
        struct RoomInput
        {
            bool previousMode = false;
            bool nextMode     = false;
            bool cursorLeft   = false;
            bool cursorRight  = false;
            bool digitUp      = false;
            bool digitDown    = false;
            bool erase        = false;
            int  typedDigit   = -1; // キーボードの 0〜9
        };

        void OnStart() override;
        void OnUpdate() override;
        void TryEnterWorld();

        void CycleMode(int delta);
        void UpdateRoomInput(const RoomInput& input);
        /** @brief cursor の桁を digit にする。まだ入れていない桁なら末尾に足す */
        void SetDigit(int digit);
        void MoveCursor(int delta);
        void Erase();
        void ApplyRoomToView() const;
        /** @brief 番号の桁数。部屋の行が無い prefab では 0 */
        [[nodiscard]] int CodeLength() const;
        [[nodiscard]] RoomInput ReadRoomInput() const;
        [[nodiscard]] bool IsRoomReady() const;

        std::shared_ptr<StageSelectUi> view_;
        std::unique_ptr<StageSelectModel> model_;
        bool wasConfirmPressed_ = false;
        RoomInput previousInput_{};

        Network::RelayRoom::Mode roomMode_ = Network::RelayRoom::Mode::Public;
        std::string roomCode_;
        int cursor_ = 0;

        // スティックを倒したと見なす傾き(XInput の -32768〜32767)
        [[serialize(1)]] int stickThreshold_ = 12000;

#pragma region Serialization Function
    public:
        void OnDrawGui() override
        {
            ImGuiHelper::OnDrawInputField("stickThreshold_", stickThreshold_);
        }

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(stickThreshold_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(stickThreshold_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::StageSelectPresenter, 1);
