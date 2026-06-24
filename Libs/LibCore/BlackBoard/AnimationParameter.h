#pragma once
#include <memory>

#include "../ImGui/Helper/ImGuiHelper.h"
#include "../../Libs/rxcpp/operators/rx-all.hpp"
#include "../Rx/SerializableSubject/unit/unit.h"
#include "cereal/types/polymorphic.hpp"
#include "IAnimationParameter.h"
#include "../../../Engine/Core/Network/Packet/ByteBuffer/Packet_ByteBuffer.h"

namespace NanamiEngine::Module::AnimationTree
{
    template<typename T>
    class AnimationParameter final : public IAnimationParameter
    {
    public:
        [[nodiscard]] const std::string& Name() const override;
        [[nodiscard]] T Get() const;
        void            Set(T value);

        [[nodiscard]] rxcpp::observable<LibCore::Rx::unit> OnChanged() const override;
        void WriteValueTo(NanamiEngine::Core::Network::ByteBuffer& buffer) const override;
        void ReadValueFrom(const NanamiEngine::Core::Network::ByteBuffer& buffer, size_t& offset) override;

        void OnDrawGui() override;

    private:
        std::string name_;
        rxcpp::subjects::subject<LibCore::Rx::unit> changeSubject_;
        T value_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<IAnimationParameter>(this));
            archive(CEREAL_NVP(name_));
            archive(CEREAL_NVP(value_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<IAnimationParameter>(this));
            if (version >= 0) archive(CEREAL_NVP(name_));
            if (version >= 0) archive(CEREAL_NVP(value_));
        }
#pragma endregion
    };

    template <typename T>
    const std::string& AnimationParameter<T>::Name() const { return  name_; }
    template <typename T>
    T                  AnimationParameter<T>::Get()  const { return value_; }
    template <typename T>
    void               AnimationParameter<T>::Set(T value)
    {
        value_ = value;
        changeSubject_.get_subscriber().on_next(LibCore::Rx::unit{});
    }

    template <typename T>
    rxcpp::observable<LibCore::Rx::unit> AnimationParameter<T>::OnChanged() const
    {
        return changeSubject_.get_observable();
    }

    template <typename T>
    void AnimationParameter<T>::WriteValueTo(NanamiEngine::Core::Network::ByteBuffer& buffer) const
    {
        buffer.Write(value_);
    }

    template <typename T>
    void AnimationParameter<T>::ReadValueFrom(const NanamiEngine::Core::Network::ByteBuffer& buffer, size_t& offset)
    {
        value_ = buffer.Read<T>(offset);
    }

    template <typename T>
    void AnimationParameter<T>::OnDrawGui()
    {
        ImGui::Text(typeid(T).name());
        LibCore::ImGuiHelper::OnDrawInputField("name_", name_);
        LibCore::ImGuiHelper::OnDrawInputField("value_" , value_);
    }
}

namespace NanamiEngine
{
    template <class T>
    using AnimParam = std::shared_ptr<Module::AnimationTree::AnimationParameter<T>>;
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationParameter<bool>, 0);
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationParameter<int>, 0);
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationParameter<float>, 0);

CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationParameter<bool>);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationParameter<int>);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::AnimationTree::AnimationParameter<float>);

CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationParameter,
    NanamiEngine::Module::AnimationTree::AnimationParameter<bool>
);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationParameter,
    NanamiEngine::Module::AnimationTree::AnimationParameter<int>
);
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    NanamiEngine::Module::AnimationTree::IAnimationParameter,
    NanamiEngine::Module::AnimationTree::AnimationParameter<float>
);
