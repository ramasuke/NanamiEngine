#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../glm/fwd.hpp"

namespace NanamiEngine::CineMachine
{
    class CinemachineCameraBrain;

    // NOTE: VirtualCameraが1フレーム内でBehaviourを回す順番。入力・オフセット計算 → 位置 → 向き
    enum class VirtualCameraStage
    {
        Driver,
        Body,
        Aim,
    };

    class NANAMI_API IVirtualCameraBehaviour
    {
    public:
        virtual ~IVirtualCameraBehaviour() = default;
        // NOTE: 毎フレーム Stage() の順に呼ばれる
        virtual void OnCameraUpdate() { }
        [[nodiscard]] virtual VirtualCameraStage Stage() const { return VirtualCameraStage::Body; }
        virtual void MainCameraCallback() { }
        // NOTE: このカメラがアクティブに切り替わったフレームに呼ばれる
        virtual void OnBecameLive() { }
        virtual bool WantsImmediateApply() const { return false; }

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
        }
    };
}
