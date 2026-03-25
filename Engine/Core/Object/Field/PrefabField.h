#pragma once
#include <string>
#include "Field.h"
#include "IInitializablePrefabField.h"
#include "../../../Module/GameObject/Interface/IGameObject.h"
#include "../../../Module/GameObject/Transform/Transform.h"

namespace NanamiEngine
{
    #define PREFAB_FIELD(TYPE) std::shared_ptr<NanamiEngine::Core::Object::PrefabObjectField<TYPE>>
}

namespace NanamiEngine::Core::Object
{
    template<typename T>
    class PrefabObjectField final : protected Field<T>, public IInitializablePrefabObjectField
    {
    public:
        PrefabObjectField() = default;

        explicit PrefabObjectField(std::string objectName = "")
            : objectName_(std::move(objectName))
        {
            
        }

        void Init(GameObject::Transform& root) override
        {
            if (const auto target = root.CatchChild(objectName_))
            {
                *this = target->Components().Catch<T>();
            }
        }

        void OnDrawGui() override
        {
            Field<T>::OnDrawGui();
            ImGuiHelper::OnDrawInputField("objectName_", objectName_);
        }
        
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(this->context_);
            archive(objectName_);
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(this->context_);
            archive(objectName_);
            Application::ApplicationBase::ApplicationLifeCycle().AddCallback(std::weak_ptr(this->context_));
        }
        
    private:
        std::string objectName_;
    };
}
