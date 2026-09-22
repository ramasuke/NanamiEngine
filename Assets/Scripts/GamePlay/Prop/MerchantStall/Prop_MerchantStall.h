#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"

namespace GamePlay::Prop
{
    /**
     * @brief 拠点の露店。店の画面を開いている間だけ、露店と店主を映すカメラへ寄せる。
     */
    class MerchantStall final : public Component::ComponentBase
    {
    public:
        void FocusCamera  () const;
        void RestoreCamera() const;

    private:
        [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera) shopCamera_;
        [[serialize(0)]] int focusPriority_ = 100;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(shopCamera_));
            archive(CEREAL_NVP(focusPriority_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(shopCamera_));
            if (version >= 0) archive(CEREAL_NVP(focusPriority_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Prop::MerchantStall, 0)
