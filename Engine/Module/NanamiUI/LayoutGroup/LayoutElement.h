#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../Component/ComponentBase.h"

namespace NanamiEngine::Module::NanamiUi
{
    // NOTE: 親 LayoutGroup の並び方向にこの子が占める枠の割合(0..1)。補間すると周りが跳ねずに詰まる
    class NANAMI_API LayoutElement final : public Component::ComponentBase
    {
    public:
        void SetMainAxisRate(float mainAxisRate);
        [[nodiscard]] float MainAxisRate() const;

    private:
        [[serialize(0)]] float mainAxisRate_ = 1.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(mainAxisRate_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(mainAxisRate_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::NanamiUi::LayoutElement, 0);
