#pragma once
#include <string>

#include "../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../cereal/include/cereal/cereal.hpp"
#include "Hash/GuidHash.h"

class Guid final
{
public:
    Guid();
    explicit Guid(const std::string& guidValue);
    [[nodiscard]] Guid Clone() const;
    [[nodiscard]] const std::string& Value() const { return value_; }

    bool operator==(const Guid& other) const { return value_ == other.value_; }
    
private:
    [[serialize(0)]] std::string value_;
    
#pragma region Serialization Function
public:
void OnDrawGui() {
    LibCore::ImGuiHelper::OnDrawInputField("value_", value_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(CEREAL_NVP(value_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    if (version >= 0) archive(CEREAL_NVP(value_));
}
#pragma endregion
};
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(Guid, 0);
#pragma endregion
