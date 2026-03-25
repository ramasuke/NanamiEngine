#pragma once
#include "../../Application/LifeCycle/ApplicationLifeCycle.h"
#include "Context/FieldContext.h"

namespace NanamiEngine
{
#define FIELD(TYPE) NanamiEngine::Core::Object::Field<TYPE>
}

namespace NanamiEngine::Core::Object
{
    template <typename T>
    class Field
    {
    public:
        Field() : context_(std::make_shared<FieldContext<T>>())
        {
            Application::ApplicationBase::ApplicationLifeCycle().AddCallback(std::weak_ptr(context_));
        }
        Field(const std::weak_ptr<T> context) : context_(std::make_shared<FieldContext<T>>())
        {
            context_->set(context.lock());
        }
        
        [[nodiscard]] std::shared_ptr<T> get() const { return context_.get()->get(); }
        std::shared_ptr<T> operator->() const { return context_.get()->get(); }
        Field& operator=(const std::shared_ptr<T> content)
        {
            context_->set(content);
            return *this;
        } 

        explicit operator bool() const { return static_cast<bool>(*context_); }

        void InitForPrompty()
        {
            if (context_)
            {
                context_->Init();
            }
        }

        virtual void OnDrawGui()
        {
            context_->OnDrawGui();
        }

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(context_);
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(context_);
            Application::ApplicationBase::ApplicationLifeCycle().AddCallback(std::weak_ptr(context_));
        }

    protected:
        std::shared_ptr<FieldContext<T>> context_;
    };
}
