#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "cereal/types/vector.hpp"
#include "../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../Engine/Module/NanamiUI/Slider/NanamiUi_Slider.h"
#include "../../../../../Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../../Data/Character/Data_CharacterData.h"
#include "../StageSelect/Difficulty/StageDifficultyPips.h"
#include "ItemCell/Ui_PauseMenuItemCell.h"
#include "Row/Ui_PauseMenuRow.h"

namespace GameCore::PlayerAvatar
{
    class ItemPouch;
}

namespace GameCore::PlayerAvatar::SwordMan
{
    class SwordManAvatarStatus;
}

namespace GamePlay::Ui
{
    /**
     * @brief 冒険者の手帳(＠メニュー)の見た目。左の頁に目次、右の頁にステータスを出す。
     * 開閉は book_ ごと SetEnable で切り替えるので、開くたびに目次の強調を当て直すこと。
     */
    class PauseMenuUi final : public Component::ComponentBase
    {
    public:
        void BuildRows();
        void ShowCharacter() const;
        void SetVisible(bool isVisible) const;
        void HighlightRow(std::size_t index) const;
        /** @brief 開いている間は毎フレーム呼ぶ。持ち物はポーチが変わったときだけ作り直す */
        void Present(const GameCore::PlayerAvatar::SwordMan::SwordManAvatarStatus& status);

    private:
        void PresentItems(const GameCore::PlayerAvatar::ItemPouch& pouch);

        [[serialize(0)]] FIELD(GameObject::IGameObject) book_;
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) rowPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) rowsRoot_;

        [[serialize(0)]] FIELD(Asset::CharacterData) character_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) nameText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) readingText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) taglineText_;
        // 腕っぷし / しぶとさ / 身軽さ の順に並べる
        [[serialize(0)]] std::vector<FIELD(StageDifficultyPips)> statPips_;

        [[serialize(0)]] FIELD(NanamiUi::Slider) healthBar_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) healthText_;
        [[serialize(0)]] FIELD(NanamiUi::Slider) staminaBar_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) staminaText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) moneyText_;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) itemCellPrefab_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) itemsRoot_;
        [[serialize(0)]] int maxItemCells_ = 6;

        std::vector<std::weak_ptr<PauseMenuRow>> rows_;
        std::vector<std::weak_ptr<PauseMenuItemCell>> itemCells_;
        std::uint32_t presentedPouchRevision_ = 0;
        bool hasPresentedItems_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(book_));
            archive(CEREAL_NVP(rowPrefab_));
            archive(CEREAL_NVP(rowsRoot_));
            archive(CEREAL_NVP(character_));
            archive(CEREAL_NVP(nameText_));
            archive(CEREAL_NVP(readingText_));
            archive(CEREAL_NVP(taglineText_));
            archive(CEREAL_NVP(statPips_));
            archive(CEREAL_NVP(healthBar_));
            archive(CEREAL_NVP(healthText_));
            archive(CEREAL_NVP(staminaBar_));
            archive(CEREAL_NVP(staminaText_));
            archive(CEREAL_NVP(moneyText_));
            archive(CEREAL_NVP(itemCellPrefab_));
            archive(CEREAL_NVP(itemsRoot_));
            archive(CEREAL_NVP(maxItemCells_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(book_));
            if (version >= 0) archive(CEREAL_NVP(rowPrefab_));
            if (version >= 0) archive(CEREAL_NVP(rowsRoot_));
            if (version >= 0) archive(CEREAL_NVP(character_));
            if (version >= 0) archive(CEREAL_NVP(nameText_));
            if (version >= 0) archive(CEREAL_NVP(readingText_));
            if (version >= 0) archive(CEREAL_NVP(taglineText_));
            if (version >= 0) archive(CEREAL_NVP(statPips_));
            if (version >= 0) archive(CEREAL_NVP(healthBar_));
            if (version >= 0) archive(CEREAL_NVP(healthText_));
            if (version >= 0) archive(CEREAL_NVP(staminaBar_));
            if (version >= 0) archive(CEREAL_NVP(staminaText_));
            if (version >= 0) archive(CEREAL_NVP(moneyText_));
            if (version >= 0) archive(CEREAL_NVP(itemCellPrefab_));
            if (version >= 0) archive(CEREAL_NVP(itemsRoot_));
            if (version >= 0) archive(CEREAL_NVP(maxItemCells_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::PauseMenuUi, 0)
