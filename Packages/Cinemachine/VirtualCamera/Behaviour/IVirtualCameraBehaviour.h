
#pragma once
#include "fwd.hpp"

namespace NanamiEngine::CineMachine
{
    class CinemachineCameraBrain;

    class IVirtualCameraBehaviour
    {
    public:
        virtual ~IVirtualCameraBehaviour() = default;
        virtual void MainCameraCallback() { }
        
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
