#pragma once
#include <string>

#include "Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "Libs/cereal/include/cereal/cereal.hpp"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace GamePlay::Ui
{
    /// 訓練カードに出す1課題分の文言。押すボタン名は書かない（操作ガイドの光っている行が示す）
    class ActionInstructTutorialStep final
    {
    public:
        [[nodiscard]] const std::string& Title() const { return title_; }
        [[nodiscard]] const std::string& Body () const { return body_ ; }

    private:
        [[serialize(0)]] std::string title_;
        [[serialize(0)]] std::string body_;

#pragma region Serialization Function
    public:
        void OnDrawGui()
        {
            ImGuiHelper::OnDrawInputField("title_", title_);
            ImGuiHelper::OnDrawInputField("body_", body_);
        }

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(title_));
            archive(CEREAL_NVP(body_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(title_));
            if (version >= 0) archive(CEREAL_NVP(body_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::ActionInstructTutorialStep, 0)
