#pragma once
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Libs/LibCore/Tween/Player/TweenPlayer.h"

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

        // ダメージ量で文字の大きさを変える。間は log で補間する
        int   minScaleDamage_ = 10;
        int   maxScaleDamage_ = 300;
        float minScale_       = 0.8f;
        float maxScale_       = 2.0f;

        // heavyDamage_ 以上は色を変え、一瞬大きく出してから縮める
        int     heavyDamage_   = 150;
        Color32 heavyColor_    = Color32(255, 140, 0);
        float   popScaleRate_  = 1.6f;
        float   popTime_secs_  = 0.15f;

        [[nodiscard]] float ScaleForDamage(int value) const;

        // startPos_ からの高さ。上がってから少し落ちる
        LibCore::Tween::TweenPlayer<float> heightTween_;
        glm::vec3 startPos_ = {};

        // baseScale_ に掛ける倍率
        LibCore::Tween::TweenPlayer<float> popTween_;
        glm::vec3 baseScale_ = glm::vec3(1.0f);

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
            archive(CEREAL_NVP(minScaleDamage_));
            archive(CEREAL_NVP(maxScaleDamage_));
            archive(CEREAL_NVP(minScale_));
            archive(CEREAL_NVP(maxScale_));
            archive(CEREAL_NVP(heavyDamage_));
            archive(CEREAL_NVP(heavyColor_));
            archive(CEREAL_NVP(popScaleRate_));
            archive(CEREAL_NVP(popTime_secs_));
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
            if (version >= 2) archive(CEREAL_NVP(minScaleDamage_));
            if (version >= 2) archive(CEREAL_NVP(maxScaleDamage_));
            if (version >= 2) archive(CEREAL_NVP(minScale_));
            if (version >= 2) archive(CEREAL_NVP(maxScale_));
            if (version >= 2) archive(CEREAL_NVP(heavyDamage_));
            if (version >= 2) archive(CEREAL_NVP(heavyColor_));
            if (version >= 2) archive(CEREAL_NVP(popScaleRate_));
            if (version >= 2) archive(CEREAL_NVP(popTime_secs_));
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

CEREAL_CLASS_VERSION(GamePlay::Ui::DealDamageTextBillBoard, 2);
