#pragma once
#include <string>
#include <vector>

#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/Sprite/SpriteFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/NanamiUI/Button/NanamiUi_Button.h"
#include "Engine/Module/NanamiUI/TextRenderer/TextRenderer.h"
#include "../../../Network/Relay/RelayRoom.h"

namespace GamePlay::Ui
{
    /**
     * @brief ステージ選択の「部屋」の行。◀▶ で 相席する / 部屋を作る / 番号で入る を切り替え、
     *        番号で入るときは数字の枠を出す。入力と状態は StageSelectPresenter が持ち、ここは見た目だけ。
     * @note 行き方ごとの言葉・操作ヒント・桁数はすべてインスペクタの値
     */
    class StageSelectRoomUi final : public Component::ComponentBase
    {
    public:
        /** @brief 番号の桁数。枠の数もこれに合わせて prefab を組んである */
        [[nodiscard]] int CodeLength() const { return codeLength_; }

        /**
         * @brief いまの行き方と番号を書く
         * @param cursor 番号を入れている桁(番号で入る以外は負)
         * @param isCodeReady 番号が桁数ぶん揃っているか(ヒントの出し分けに使う)
         */
        void ShowRoom(Network::RelayRoom::Mode mode, const std::string& code, int cursor, bool isCodeReady);

        [[nodiscard]] NanamiEngine::R4::Observable<NanamiUi::MouseState> OnLeftArrowClicked() const { return leftArrowButton_->OnClick(); }
        [[nodiscard]] NanamiEngine::R4::Observable<NanamiUi::MouseState> OnRightArrowClicked() const { return rightArrowButton_->OnClick(); }

    private:
        void ShowCode(const std::string& code, int cursor);
        [[nodiscard]] const std::string& TextFor(const std::vector<std::string>& texts, Network::RelayRoom::Mode mode) const;

        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) modeText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) noteText_;
        [[serialize(0)]] FIELD(NanamiUi::TextRenderer) hintText_;
        [[serialize(0)]] FIELD(NanamiUi::Button) leftArrowButton_;
        [[serialize(0)]] FIELD(NanamiUi::Button) rightArrowButton_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) digitsRoot_;
        [[serialize(0)]] std::vector<FIELD(Component::ImageRenderer)> digitBoxes_;
        [[serialize(0)]] std::vector<FIELD(NanamiUi::TextRenderer)> digitTexts_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) digitSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) digitFocusSprite_;
        [[serialize(0)]] int codeLength_ = 6;
        // 行き方ごとの言葉。RelayRoom::Mode の順 (相席する / 部屋を作る / 番号で入る)
        [[serialize(0)]] std::vector<std::string> modeNames_;
        [[serialize(0)]] std::vector<std::string> modeNotes_;
        // 操作ヒント。行き方ごと + 番号がまだ揃っていないとき
        [[serialize(0)]] std::vector<std::string> modeHints_;
        [[serialize(0)]] std::string codeIncompleteHint_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(modeText_));
            archive(CEREAL_NVP(noteText_));
            archive(CEREAL_NVP(hintText_));
            archive(CEREAL_NVP(leftArrowButton_));
            archive(CEREAL_NVP(rightArrowButton_));
            archive(CEREAL_NVP(digitsRoot_));
            archive(CEREAL_NVP(digitBoxes_));
            archive(CEREAL_NVP(digitTexts_));
            archive(CEREAL_NVP(digitSprite_));
            archive(CEREAL_NVP(digitFocusSprite_));
            archive(CEREAL_NVP(codeLength_));
            archive(CEREAL_NVP(modeNames_));
            archive(CEREAL_NVP(modeNotes_));
            archive(CEREAL_NVP(modeHints_));
            archive(CEREAL_NVP(codeIncompleteHint_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(modeText_));
            if (version >= 0) archive(CEREAL_NVP(noteText_));
            if (version >= 0) archive(CEREAL_NVP(hintText_));
            if (version >= 0) archive(CEREAL_NVP(leftArrowButton_));
            if (version >= 0) archive(CEREAL_NVP(rightArrowButton_));
            if (version >= 0) archive(CEREAL_NVP(digitsRoot_));
            if (version >= 0) archive(CEREAL_NVP(digitBoxes_));
            if (version >= 0) archive(CEREAL_NVP(digitTexts_));
            if (version >= 0) archive(CEREAL_NVP(digitSprite_));
            if (version >= 0) archive(CEREAL_NVP(digitFocusSprite_));
            if (version >= 0) archive(CEREAL_NVP(codeLength_));
            if (version >= 0) archive(CEREAL_NVP(modeNames_));
            if (version >= 0) archive(CEREAL_NVP(modeNotes_));
            if (version >= 0) archive(CEREAL_NVP(modeHints_));
            if (version >= 0) archive(CEREAL_NVP(codeIncompleteHint_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::StageSelectRoomUi, 0);
