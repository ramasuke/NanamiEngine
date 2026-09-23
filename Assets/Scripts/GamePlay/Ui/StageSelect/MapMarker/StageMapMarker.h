#pragma once
#include "vec2.hpp"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/LifeCycleCallback/Awake/IAwakable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"

namespace GamePlay::Ui
{
    // プレハブ内に1個だけ配置し、選択中のステージが変わるたびMoveTo()で使い回す
    class StageMapMarker final : public Component::ComponentBase,
                                 public LifeCycleCallback::IAwakable,
                                 public LifeCycleCallback::IUpdatable
    {
    public:
        // 移動先を新しい浮遊(bobbing)の基準位置としても採用する
        void MoveTo(const glm::vec2& position);
        void SetCleared(bool isCleared);

    private:
        void OnAwake() override;
        void OnUpdate() override;

        FIELD(Component::ImageRenderer) imageRenderer_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) challengeSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) clearedSprite_;
        [[serialize(0)]] float bobAmplitude_ = 8.0f;
        [[serialize(0)]] float bobSpeed_ = 3.0f;

        float bobTime_ = 0.0f;
        glm::vec3 basePos_ = glm::vec3(0.0f);

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(challengeSprite_));
            archive(CEREAL_NVP(clearedSprite_));
            archive(CEREAL_NVP(bobAmplitude_));
            archive(CEREAL_NVP(bobSpeed_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(challengeSprite_));
            if (version >= 0) archive(CEREAL_NVP(clearedSprite_));
            if (version >= 0) archive(CEREAL_NVP(bobAmplitude_));
            if (version >= 0) archive(CEREAL_NVP(bobSpeed_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::StageMapMarker, 0);
