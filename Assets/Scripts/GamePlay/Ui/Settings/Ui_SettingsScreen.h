#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "vec3.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/BlendImageRenderer/BlendImageRenderer.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"
#include "Row/Ui_SettingsRow.h"
#include "Tab/Ui_SettingsTab.h"

namespace GamePlay::Ui
{
    /**
     * @brief 設定画面の見た目 (モンハンライズのオプションに倣った並び)。
     * 左にカテゴリの札、右の黒漆のパネルに「項目 ─ 値」の一覧、左下に説明、右下に操作ヒント。
     * 札と行は prefab から生成し、行は表示窓の分だけ作って中身を流し込む。
     */
    class SettingsScreenUi final : public Component::ComponentBase,
                                   public LifeCycleCallback::IUpdatable
    {
    public:
        void BuildTabs(const std::vector<std::string>& names);
        void SetTabSelection(size_t index) const;
        void SetCategoryName(const std::string& name) const;

        /** @brief 行の数 (表示窓の大きさ) */
        [[nodiscard]] size_t VisibleRowCount();
        /** @brief slot 番目の行に中身を出す。label が空なら行ごと隠す */
        void SetRow(size_t slot, const std::string& label, const std::string& value, bool isSelected);
        /** @brief 一覧があふれているときだけスクロールバーを出す */
        void SetScroll(size_t firstVisibleIndex, size_t count);
        void SetDescription(const std::string& text) const;

        void PlayEnter();

    private:
        void OnUpdate() override;
        void EnsureRows();

        [[serialize(0)]] FIELD(GameObject::IGameObject) visualRoot_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) tabPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) tabsRoot_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) rowsRoot_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) categoryText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) descriptionText_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) scrollTrack_;
        [[serialize(0)]] FIELD(NanamiUi::BlendImageRenderer) scrollThumb_;

        [[serialize(0)]] float tabPitch_px_         = 74.0f;
        [[serialize(0)]] float rowPitch_px_         = 60.0f;
        [[serialize(0)]] int   maxVisibleRows_      = 6;
        /** つまみが動ける縦の幅 (つまみの上端の移動量) */
        [[serialize(0)]] float scrollTravel_px_     = 300.0f;
        [[serialize(0)]] float enterSlide_px_       = 24.0f;
        [[serialize(0)]] float enterDuration_secs_  = 0.18f;

        std::vector<std::weak_ptr<SettingsTabUi>> tabs_;
        std::vector<std::weak_ptr<SettingsRowUi>> rows_;
        glm::vec3 visualBasePos_ = glm::vec3(0.0f);
        glm::vec3 thumbBasePos_  = glm::vec3(0.0f);
        bool isThumbBaseTaken_ = false;
        LibCore::Tween::TweenPlayer<float> enterTween_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(visualRoot_));
            archive(CEREAL_NVP(tabPrefab_));
            archive(CEREAL_NVP(tabsRoot_));
            archive(CEREAL_NVP(rowPrefab_));
            archive(CEREAL_NVP(rowsRoot_));
            archive(CEREAL_NVP(categoryText_));
            archive(CEREAL_NVP(descriptionText_));
            archive(CEREAL_NVP(scrollTrack_));
            archive(CEREAL_NVP(scrollThumb_));
            archive(CEREAL_NVP(tabPitch_px_));
            archive(CEREAL_NVP(rowPitch_px_));
            archive(CEREAL_NVP(maxVisibleRows_));
            archive(CEREAL_NVP(scrollTravel_px_));
            archive(CEREAL_NVP(enterSlide_px_));
            archive(CEREAL_NVP(enterDuration_secs_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(visualRoot_));
            if (version >= 0) archive(CEREAL_NVP(tabPrefab_));
            if (version >= 0) archive(CEREAL_NVP(tabsRoot_));
            if (version >= 0) archive(CEREAL_NVP(rowPrefab_));
            if (version >= 0) archive(CEREAL_NVP(rowsRoot_));
            if (version >= 0) archive(CEREAL_NVP(categoryText_));
            if (version >= 0) archive(CEREAL_NVP(descriptionText_));
            if (version >= 0) archive(CEREAL_NVP(scrollTrack_));
            if (version >= 0) archive(CEREAL_NVP(scrollThumb_));
            if (version >= 0) archive(CEREAL_NVP(tabPitch_px_));
            if (version >= 0) archive(CEREAL_NVP(rowPitch_px_));
            if (version >= 0) archive(CEREAL_NVP(maxVisibleRows_));
            if (version >= 0) archive(CEREAL_NVP(scrollTravel_px_));
            if (version >= 0) archive(CEREAL_NVP(enterSlide_px_));
            if (version >= 0) archive(CEREAL_NVP(enterDuration_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::SettingsScreenUi, 0);
