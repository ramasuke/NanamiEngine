#include "Ui_StageSelect_RoomUi.h"

#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace GamePlay::Ui
{
    namespace
    {
        const std::string EMPTY_TEXT;
    }

    void StageSelectRoomUi::ShowRoom(const Network::RelayRoom::Mode mode, const std::string& code, const int cursor,
                                     const bool isCodeReady)
    {
        const bool isJoin = mode == Network::RelayRoom::Mode::Join;

        if (const auto text = modeText_.get())
            text->SetText(TextFor(modeNames_, mode));

        // 番号で入るときだけ枠を出し、ほかは説明文を出す(同じ行に重ねてあるので、どちらか片方だけ)
        if (const auto note = noteText_.get())
            note->SetText(isJoin ? EMPTY_TEXT : TextFor(modeNotes_, mode));
        if (const auto digits = digitsRoot_.get())
            digits->SetEnable(isJoin);
        if (const auto hint = hintText_.get())
            hint->SetText(isJoin && !isCodeReady ? codeIncompleteHint_ : TextFor(modeHints_, mode));

        ShowCode(code, isJoin ? cursor : -1);
    }

    void StageSelectRoomUi::ShowCode(const std::string& code, const int cursor)
    {
        for (size_t i = 0; i < digitTexts_.size(); ++i)
        {
            if (const auto text = digitTexts_[i].get())
                text->SetText(i < code.size() ? code.substr(i, 1) : EMPTY_TEXT);

            if (i >= digitBoxes_.size())
                continue;
            if (const auto box = digitBoxes_[i].get())
                box->SetSprite(static_cast<int>(i) == cursor ? digitFocusSprite_.get() : digitSprite_.get());
        }
    }

    const std::string& StageSelectRoomUi::TextFor(const std::vector<std::string>& texts, const Network::RelayRoom::Mode mode) const
    {
        const size_t index = static_cast<size_t>(mode);
        return index < texts.size() ? texts[index] : EMPTY_TEXT;
    }

    void StageSelectRoomUi::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("modeText_", modeText_);
        ImGuiHelper::OnDrawInputField("noteText_", noteText_);
        ImGuiHelper::OnDrawInputField("hintText_", hintText_);
        ImGuiHelper::OnDrawInputField("leftArrowButton_", leftArrowButton_);
        ImGuiHelper::OnDrawInputField("rightArrowButton_", rightArrowButton_);
        ImGuiHelper::OnDrawInputField("digitsRoot_", digitsRoot_);
        ImGuiHelper::OnDrawInputField("digitBoxes_", digitBoxes_, [this]
        {
            if (ImGui::Button("Add Digit Box"))
                digitBoxes_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("digitTexts_", digitTexts_, [this]
        {
            if (ImGui::Button("Add Digit Text"))
                digitTexts_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("digitSprite_", digitSprite_);
        ImGuiHelper::OnDrawInputField("digitFocusSprite_", digitFocusSprite_);
        ImGuiHelper::OnDrawInputField("codeLength_", codeLength_);
        ImGuiHelper::OnDrawInputField("modeNames_", modeNames_, [this]
        {
            if (ImGui::Button("Add Mode Name"))
                modeNames_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("modeNotes_", modeNotes_, [this]
        {
            if (ImGui::Button("Add Mode Note"))
                modeNotes_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("modeHints_", modeHints_, [this]
        {
            if (ImGui::Button("Add Mode Hint"))
                modeHints_.emplace_back();
        });
        ImGuiHelper::OnDrawInputField("codeIncompleteHint_", codeIncompleteHint_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(GamePlay::Ui::StageSelectRoomUi);
#pragma endregion
