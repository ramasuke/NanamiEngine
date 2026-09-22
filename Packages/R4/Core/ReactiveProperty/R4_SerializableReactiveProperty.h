#pragma once
#include "cereal/cereal.hpp"

#include "R4_ReactiveProperty.h"
#include "../../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"

namespace NanamiEngine::R4
{
    ///NOTE: cereal で保存でき、インスペクタで編集できる ReactiveProperty（R3 の SerializableReactiveProperty<T>）
    ///      保存形式は {"value": ...}（旧 LibCore::Rx::SerializableSubject と同じなので既存データをそのまま読める）
    template <typename T>
    class SerializableReactiveProperty final : public ReactiveProperty<T>
    {
    public:
        explicit SerializableReactiveProperty(T value = T()) : ReactiveProperty<T>(std::move(value)) { }

        void OnDrawGui()
        {
            T edited = this->Value();
            LibCore::ImGuiHelper::OnDrawInputField("value", edited);
            this->Value(edited);

            if (ImGui::Button("onNext"))
                this->ForceNotify();
        }

        template <class Archive>
        void save(Archive& archive) const
        {
            const T value = this->Value();
            archive(cereal::make_nvp("value", value));
        }

        template <class Archive>
        void load(Archive& archive)
        {
            T value = this->Value();
            archive(cereal::make_nvp("value", value));
            this->Value(value);
        }
    };
}
