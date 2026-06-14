#pragma once
#include "../cereal/include/cereal/cereal.hpp"

namespace GameCore
{
    class GameSettings final
    {
    public:
        GameSettings() = default;

        // ファイルから設定を読み込んだシングルトンを返す。初回呼び出し時に LocalPrefs からロードする
        static GameSettings& GetInstance();

        [[nodiscard]] float GetChatTextCharInterval_secs() const { return chatTextCharInterval_secs_; }
        [[nodiscard]] float GetChatTextSentenceInterval_secs() const { return chatTextSentenceInterval_secs_; }

        // 設定値を ImGui ウィジェットで編集するためのUI描画 (DrawLocalPrefWidget に使われる)
        void OnDrawGui();

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(
                CEREAL_NVP(chatTextCharInterval_secs_),
                CEREAL_NVP(chatTextSentenceInterval_secs_)
            );
        }

    private:
        float chatTextCharInterval_secs_     = 0.015f;
        float chatTextSentenceInterval_secs_ = 1.0f;
    };
}
