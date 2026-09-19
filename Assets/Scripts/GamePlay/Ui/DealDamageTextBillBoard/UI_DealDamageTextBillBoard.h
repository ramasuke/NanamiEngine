#pragma once
#include "../../../../../Engine/Module/Color/Color32.h"
#include "../../../../../Engine/Module/Component/ComponentBase.h"

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace GamePlay::Ui
{
    class DealDamageTextBillBoard final : public Component::ComponentBase,
                                          public LifeCycleCallback::IAwakable,
                                          public LifeCycleCallback::IUpdatable
    {
    public:
        enum class Emphasis
        {
            Normal,
            BreakablePart,
            WeakPointStun,
        };

        void Play(int value, Emphasis emphasis);

    private:
        void OnAwake () override;
        void OnUpdate() override;

        float riseTime_   = 0.5f;
        float fallTime_   = 0.3f;
        float riseAmount_ = 1.0f;
        float fallAmount_ = 0.8f;

        Color32 breakablePartColor_ = Color32(255, 215, 0);
        Color32 weakPointStunColor_ = Color32(255, 40, 40);
        float   emphasisScaleRate_  = 1.5f;

        bool      isPlaying_   = false;
        float     elapsedTime_ = 0.0f;
        glm::vec3 startPos_    = {};

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(riseTime_));
            archive(CEREAL_NVP(fallTime_));
            archive(CEREAL_NVP(riseAmount_));
            archive(CEREAL_NVP(fallAmount_));
            archive(CEREAL_NVP(breakablePartColor_));
            archive(CEREAL_NVP(weakPointStunColor_));
            archive(CEREAL_NVP(emphasisScaleRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(riseTime_));
            if (version >= 0) archive(CEREAL_NVP(fallTime_));
            if (version >= 0) archive(CEREAL_NVP(riseAmount_));
            if (version >= 0) archive(CEREAL_NVP(fallAmount_));
            if (version >= 1) archive(CEREAL_NVP(breakablePartColor_));
            if (version >= 1) archive(CEREAL_NVP(weakPointStunColor_));
            if (version >= 1) archive(CEREAL_NVP(emphasisScaleRate_));
        }
#pragma endregion
    };

    /** @brief 当たった部位から強調色を決めて、position にダメージ表記を出す */
    void SpawnDealDamageText(Asset::PrefabGameObjectFile& prefab,
                             const glm::vec3& position,
                             int value,
                             const std::shared_ptr<GameObject::IGameObject>& hitPart,
                             GameObject::IGameObject& targetObject,
                             bool isChargedAttack);
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::DealDamageTextBillBoard, 1)
