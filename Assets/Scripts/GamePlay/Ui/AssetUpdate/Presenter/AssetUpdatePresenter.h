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
#include "../Ui_AssetUpdateTag.h"
#include "../../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    /**
     * @brief タイトル画面のアセット更新の進行役。起動と同時に配信の更新を確かめ、状態に合わせて荷札を出す。
     * A (Enter) / B (Esc) か、ヒントの札のクリックで答える。
     *   更新あり      A 受け取る → 落として入れる / B あとで → 札を引っ込める (ゲームはまだ始められない)
     *   失敗          A もう一度 / B あとで
     *   本体が古い    B 閉じる
     *   入れ終えた    A 再起動する (予約してウィンドウを閉じる)
     * 確認中・最新・未インストール (エディタ)・オフラインでは何も出さない。
     * 実装を差し替えるときは CreateAssetUpdater だけを変える。
     */
    class AssetUpdatePresenter final : public Component::ComponentBase,
                                       public LifeCycleCallback::IStartable,
                                       public LifeCycleCallback::IUpdatable
    {
    public:
        /**
         * @brief ゲームを始めてよいか。だめなら、今の状態の荷札を出し直して false を返す。
         * 確認中やダウンロード中の押下は黙って受け流す
         */
        [[nodiscard]] bool TryStartGame();

    private:
        struct Keys
        {
            bool confirm = false;
            bool cancel  = false;
        };

        /** @brief エディタで見た目を確かめるための、偽の状態 */
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
        [[nodiscard]] static Keys ReadKeys();
        void OnStateChanged(AssetUpdater::AssetUpdateState state);
        /** @brief 答えを待つ状態 (更新あり・失敗・本体が古い・入れ終えた) の札を出す */
        void ShowPromptFor(AssetUpdater::AssetUpdateState state);
        void ShowProgress();
        void Confirm();
        void Cancel();
        void Relaunch() const;
        [[nodiscard]] AssetUpdateParcel Parcel() const;
        void PlaySound(const FIELD(Asset::SoundFile)& sound) const;
        void UpdatePreview();
        void PlayPreviewDownload();

        [[serialize(0)]] FIELD(Asset::SoundFile) stampSound_;
        [[serialize(0)]] FIELD(Asset::SoundFile) confirmSound_;
        [[serialize(1)]] FIELD(Asset::UiSoundBankData) uiSounds_;

        std::shared_ptr<AssetUpdateTagUi> view_;
        std::unique_ptr<AssetUpdater::AssetUpdateTask> task_;
        AssetUpdater::AssetUpdateState shownState_ = AssetUpdater::AssetUpdateState::Idle;
        Keys previousKeys_;
        bool canRelaunch_ = false;

        Preview preview_ = Preview::None;
        /** 偽の受け取りの進み 0..1 */
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
