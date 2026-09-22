#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"

namespace GamePlay::Magic
{
    // 設置の魔法（壁・罠）のプレハブに付ける。寿命が来たら消える
    class MagicPlacement final : public Component::ComponentBase,
                                 public LifeCycleCallback::IUpdatable
    {
    public:
        /** @brief 生成直後に呼ぶ */
        void Place(float lifeTime_secs);

    private:
        void OnUpdate() override;

        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) vanishPrefab_;
        [[serialize(0)]] float vanishEffectLifeTime_secs_ = 2.0f;

        float remainingLifeTime_secs_ = 0.0f;
        bool isPlaced_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(vanishPrefab_));
            archive(CEREAL_NVP(vanishEffectLifeTime_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(vanishPrefab_));
            if (version >= 0) archive(CEREAL_NVP(vanishEffectLifeTime_secs_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Magic::MagicPlacement, 0)
