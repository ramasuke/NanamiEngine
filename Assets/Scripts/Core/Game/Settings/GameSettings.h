#pragma once

namespace GameCore
{
    class GameSettings final
    {
        GameSettings() = default;

    public:
        static GameSettings& GetInstance();
        [[nodiscard]] float GetChatTextCharInterval_secs() const { return chatTextCharInterval_secs_; }
        [[nodiscard]] float GetChatTextSentenceInterval_secs() const { return chatTextSentenceInterval_secs_; }
        
    private:
        [[serialize(0)]] float chatTextCharInterval_secs_     = 0.03f;
        [[serialize(0)]] float chatTextSentenceInterval_secs_ = 1.0f;
    };
}
