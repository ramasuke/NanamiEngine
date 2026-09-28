#pragma once
#include "../cereal/include/cereal/cereal.hpp"

namespace GameCore
{
    enum class ChatAdvanceMode : int
    {
        Manual = 0,
        Auto   = 1,
    };

    class GameSettings final
    {
    public:
        GameSettings() = default;

        static GameSettings& GetInstance();

        void Save() const;

        [[nodiscard]] float GetChatTextCharInterval_secs() const { return chatTextCharInterval_secs_; }
        [[nodiscard]] float GetChatTextSentenceInterval_secs() const { return chatTextSentenceInterval_secs_; }
        [[nodiscard]] ChatAdvanceMode GetChatAdvanceMode() const { return chatAdvanceMode_; }
        void SetChatAdvanceMode(const ChatAdvanceMode mode) { chatAdvanceMode_ = mode; }

        /** 音量は 0..VOLUME_STEPS の段階。変えるとすぐエンジンの音量に反映する */
        static constexpr int VOLUME_STEPS = 10;
        [[nodiscard]] int GetMasterVolume() const { return masterVolume_; }
        [[nodiscard]] int GetBgmVolume() const { return bgmVolume_; }
        [[nodiscard]] int GetSeVolume() const { return seVolume_; }
        void SetMasterVolume(int step);
        void SetBgmVolume(int step);
        void SetSeVolume(int step);
        void ApplyAudioVolume() const;

        void OnDrawGui();

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            // NOTE: enum は int にして書く (cereal の enum 対応を持ち込まない)
            const int chatAdvanceMode = static_cast<int>(chatAdvanceMode_);
            archive(
                CEREAL_NVP(chatTextCharInterval_secs_),
                CEREAL_NVP(chatTextSentenceInterval_secs_),
                cereal::make_nvp("chatAdvanceMode_", chatAdvanceMode),
                CEREAL_NVP(masterVolume_),
                CEREAL_NVP(bgmVolume_),
                CEREAL_NVP(seVolume_)
            );
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            if (version >= 0) archive(CEREAL_NVP(chatTextCharInterval_secs_));
            if (version >= 0) archive(CEREAL_NVP(chatTextSentenceInterval_secs_));
            int chatAdvanceMode = static_cast<int>(chatAdvanceMode_);
            if (version >= 0) archive(cereal::make_nvp("chatAdvanceMode_", chatAdvanceMode));
            chatAdvanceMode_ = chatAdvanceMode == static_cast<int>(ChatAdvanceMode::Manual) ? ChatAdvanceMode::Manual : ChatAdvanceMode::Auto;
            if (version >= 1) archive(CEREAL_NVP(masterVolume_));
            if (version >= 1) archive(CEREAL_NVP(bgmVolume_));
            if (version >= 1) archive(CEREAL_NVP(seVolume_));
        }

    private:
        float chatTextCharInterval_secs_     = 0.01875f;
        float chatTextSentenceInterval_secs_ = 1.25f;
        ChatAdvanceMode chatAdvanceMode_     = ChatAdvanceMode::Auto;
        int masterVolume_                    = VOLUME_STEPS;
        int bgmVolume_                       = VOLUME_STEPS;
        int seVolume_                        = VOLUME_STEPS;
    };
}

CEREAL_CLASS_VERSION(GameCore::GameSettings, 1);
