#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Color/Color32.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"

namespace GamePlay::Weather
{
    // NOTE: シーンにいる間ずっと線形フォグを掛け、抜けるときに切る
    class SceneFog final : public Component::ComponentBase,
                           public LifeCycleCallback::IStartable,
                           public LifeCycleCallback::IUpdatable
    {
    public:
        // NOTE: true の間は自分でフォグを掛けない (使う側が掛ける)
        void SetDrivenExternally(const bool driven) { drivenExternally_ = driven; }

        [[nodiscard]] NanamiEngine::Color32 FogColor() const { return fogColor_; }
        [[nodiscard]] float FogStart() const { return fogStart_; }
        [[nodiscard]] float FogEnd  () const { return fogEnd_; }

    private:
        bool drivenExternally_ = false;

        void OnStart  () override;
        // NOTE: インスペクタで変えた値がすぐ見えるよう毎フレーム掛け直す
        void OnUpdate () override;
        void OnDestroy() override;

        void Apply() const;

        [[serialize(0)]] NanamiEngine::Color32 fogColor_ = NanamiEngine::Color32(196, 214, 232);
        [[serialize(0)]] float fogStart_ = 250.0f;
        [[serialize(0)]] float fogEnd_ = 3200.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(fogColor_));
            archive(CEREAL_NVP(fogStart_));
            archive(CEREAL_NVP(fogEnd_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(fogColor_));
            if (version >= 0) archive(CEREAL_NVP(fogStart_));
            if (version >= 0) archive(CEREAL_NVP(fogEnd_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Weather::SceneFog, 0);
