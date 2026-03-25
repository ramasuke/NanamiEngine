#pragma once
#include <cstdint>

class Guid;

namespace JPH
{
    enum class EMotionType : uint8_t;
}

namespace JPH
{
    class BodyID;
}

namespace NanamiEngine::Module::Physics
{
    class ICollider
    {
    public:
        virtual ~ICollider() = default;
        [[nodiscard]] virtual const JPH::BodyID& BodyId() const = 0;
        virtual void ChangeEmotionType(const JPH::EMotionType& type) = 0;
    };
}
