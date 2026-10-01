#pragma once
#include <cstdint>
#include <memory>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sound/SoundFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "Packages/AssetUpdater/Task/AssetUpdateTask.h"
#include "Packages/UiFlow/UiFlow.h"
#include "../Ui_AssetUpdateTag.h"
#include "../../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    class AssetUpdatePresenter final : public Component::ComponentBase,
                                       public LifeCycleCallback::IStartable,
                                       public LifeCycleCallback::IUpdatable
    {
    public:
        [[nodiscard]] bool TryStartGame();
        [[nodiscard]] bool IsPrompting() const;

    private:
        enum class Preview
        {
            None,
            Offer,
            Receiving,
            Undelivered,
            Received,
            WrongVersion,
        };

        void OnStart () override;
        void OnUpdate() override;

        [[nodiscard]] std::unique_ptr<AssetUpdater::IAssetUpdater> CreateAssetUpdater(const AssetUpdater::AssetUpdaterPaths& paths) const;
        void SyncScreen();
        void OnStateChanged(AssetUpdater::AssetUpdateState state);
        void ShowPromptFor(AssetUpdater::AssetUpdateState state);
        void ShowProgress();
        void Confirm();
        void Cancel();
        void Quit() const;
        [[nodiscard]] AssetUpdateParcel Parcel() const;
        void PlaySound(const FIELD(Asset::SoundFile)& sound) const;
        void UpdatePreview();
        void PlayPreviewDownload();

        [[serialize(0)]] FIELD(Asset::SoundFile) stampSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile) confirmSound_;
        [[serialize(1)]] FIELD(Asset::UiSoundBankData) uiSounds_;

        std::shared_ptr<AssetUpdateTagUi> view_;
        std::shared_ptr<UiFlow::UiScreen> screen_;
        std::unique_ptr<AssetUpdater::AssetUpdateTask> task_;
        AssetUpdater::AssetUpdateState shownState_ = AssetUpdater::AssetUpdateState::Idle;

        Preview preview_ = Preview::None;
        LibCore::Tween::TweenPlayer<float> previewDownloadTween_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(stampSound_));
            archive(CEREAL_NVP(confirmSound_));
            archive(CEREAL_NVP(uiSounds_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(stampSound_));
            if (version >= 0) archive(CEREAL_NVP(confirmSound_));
            if (version >= 1) archive(CEREAL_NVP(uiSounds_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::AssetUpdatePresenter, 1);
