#pragma once
#include <cereal/cereal.hpp>

#include "../../ImGui/Helper/ImGuiHelper.h"
#include "../../Libs/rxcpp/operators/rx-all.hpp"
#include "../ReadOnlyReactiveContext/ReadOnlyReactiveContext.h"

namespace LibCore::Rx
{
    template <typename T>
    class SerializableSubject final
    {
    public:
        template <class Archive>
        void serialize(Archive& ar)
        {
            ar(cereal::make_nvp("value", value_));
        }

        explicit SerializableSubject(T value = T());
        void OnDrawGui();
        const T& get() const { return value_; }
        void OnNext(T value);
        ///NOTE: 中身がshared_ptrなどで持っているので、参照ではなく実態を返しても変わらない。
        ReadOnlyReactiveContext<T> AsReadOnly() const;

    private:
        rxcpp::subjects::subject<T> subject_;
        T value_;
    };

    template <typename T>
    SerializableSubject<T>::SerializableSubject(T value)
        : subject_(), value_(value)
    {
    }

    template <typename T>
    void SerializableSubject<T>::OnDrawGui()
    {
        T beforeDraw = value_;

        ImGuiHelper::OnDrawInputField("value", value_);

        if (ImGui::Button("onNext"))
        {
            OnNext(value_);
        }
    }

    template <typename T>
    void SerializableSubject<T>::OnNext(T value)
    {
        value_ = value;
        subject_.get_subscriber().on_next(value);
    }

    template <typename T>
    ReadOnlyReactiveContext<T> SerializableSubject<T>::AsReadOnly() const
    {
        return ReadOnlyReactiveContext<T>(value_, subject_.get_observable());
    }
}
