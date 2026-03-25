#pragma once
#include <assert.h>

#include "cereal/cereal.hpp"

namespace GameCore::StatusParameter
{
    //NOTE: 値オブジェクト推奨
    struct Health final
    {
        explicit Health(int value = 0);
        void OnDrawGui();
        [[nodiscard]] int Value() const { return value_; }
        
        float operator/(const Health& rhs) const;
        auto operator<=>(const Health&) const = default;
        
    private:
        int value_;
        
#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(CEREAL_NVP(value_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(CEREAL_NVP(value_));
        }
#pragma endregion
    };
}
