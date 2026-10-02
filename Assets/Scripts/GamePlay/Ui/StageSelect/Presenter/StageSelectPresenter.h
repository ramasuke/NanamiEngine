#pragma once
#include <string>
#include <vector>

#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Packages/UiFlow/UiFlow.h"
#include "../Model/StageSelectModel.h"
#include "../../../Network/Relay/RelayRoom.h"
#include "../../../Sound/UiSoundBank.h"

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
        void OnStart() override;
        void OnUpdate() override;
        void TryEnterWorld();
        void Close();

        void CycleMode(int delta);
        void MoveStage(int delta);
        void UpdateRoomInput();
        void ApplyInputMap() const;
        void SetDigit(int digit);
        void MoveCursor(int delta);
        void Erase();
        void ApplyRoomToView() const;
        [[nodiscard]] int CodeLength() const;
        [[nodiscard]] bool IsRoomReady() const;
        [[nodiscard]] bool IsSelectedStageLocked() const;

        std::shared_ptr<StageSelectUi> view_;
        std::unique_ptr<StageSelectModel> model_;
        std::shared_ptr<UiFlow::UiScreen> screen_;

        Network::RelayRoom::Mode roomMode_ = Network::RelayRoom::Mode::Public;
        std::string roomCode_;
        int cursor_ = 0;
        // NOTE: 解放前に隠す行 (StageData::HidesWhenLocked)。行の並びは view と揃えたまま、カーソルで飛ばす
        std::vector<bool> isHidden_;

        [[serialize(1)]] int stickThreshold_ = 12000;
        [[serialize(2)]] FIELD(Asset::UiSoundBankData) uiSounds_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override
        {
            ImGuiHelper::OnDrawInputField("stickThreshold_", stickThreshold_);
            ImGuiHelper::OnDrawInputField("uiSounds_", uiSounds_);
        }

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(stickThreshold_));
            archive(CEREAL_NVP(uiSounds_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(stickThreshold_));
            if (version >= 2) archive(CEREAL_NVP(uiSounds_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::StageSelectPresenter, 2);
