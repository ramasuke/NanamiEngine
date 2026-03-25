#pragma once
#include "../glm/fwd.hpp"
#include "../glm/detail/func_packing_simd.inl"
#include "../glm/detail/type_quat.hpp"

namespace cereal
{
    template <class Archive>
    void serialize(Archive& archive, glm::mat4& m)
    {
        for (int i = 0; i < 4; ++i)
            archive(m[i]);
    }

    template <class Archive>
    void serialize(Archive& archive, glm::vec3& v)
    {
        archive(v.x, v.y, v.z);
    }

    template <class Archive>
    void serialize(Archive& archive, glm::quat& q)
    {
        archive(q.x, q.y, q.z, q.w);
    }

    template <class Archive>
    void serialize(Archive& archive, glm::vec4& v)
    {
        archive(v.x, v.y, v.z, v.w);
    }
    template <class Archive>
    void serialize(Archive& archive, glm::vec2& v)
    {
        archive(v.x, v.y);
    }
}
