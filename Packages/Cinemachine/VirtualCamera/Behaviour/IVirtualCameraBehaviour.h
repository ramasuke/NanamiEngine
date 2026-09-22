#pragma once
#include "../glm/fwd.hpp"

namespace NanamiEngine::CineMachine
{
    class CinemachineCameraBrain;

    // VirtualCameraが1フレーム内でBehaviourを回す順番。入力・オフセット計算 → 位置 → 向き
    enum class VirtualCameraStage
    {
        Driver,
        Body,
        Aim,
    };

    class IVirtualCameraBehaviour
    {
    public:
        virtual ~IVirtualCameraBehaviour() = default;
        // Brainが毎フレーム、VirtualCamera経由でStage()の順に呼ぶ
        virtual void OnCameraUpdate() { }
        [[nodiscard]] virtual VirtualCameraStage Stage() const { return VirtualCameraStage::Body; }
        virtual void MainCameraCallback() { }
        // このVirtualCameraがBrainのアクティブカメラに切り替わったフレームに呼ばれる。
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
