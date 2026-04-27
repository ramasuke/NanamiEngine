#pragma once
#include "../../../Core/Object/IObject.h"

namespace NanamiEngine::Module::GameObject
{
    class ComponentGroup;
}

namespace NanamiEngine::Module::GameObject
{
    class Transform;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject : public Object::IObject
    {
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<IObject>(this));  
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<IObject>(this));
        }
        
        virtual ~IGameObject() = default;
        virtual void InitGameObject(const std::weak_ptr<IGameObject>& parent, const std::shared_ptr<IGameObject>& ownPtr) = 0;
        virtual void InitForCopied (const std::shared_ptr<IGameObject>& ownPtr, bool isActive, std::string name, ComponentGroup components, Transform transform) = 0;
        virtual std::shared_ptr<IGameObject> CopyForInstantiate() = 0;
        [[nodiscard]] virtual const std::string& Name        () const = 0;
        [[nodiscard]] virtual Transform&         TransformRef() = 0;
        [[nodiscard]] virtual ComponentGroup&    Components  () = 0;
        /** @brief GameObjectの全ての機能の有効無効を切り替える */
        virtual void SetEnable(bool enable) = 0;
        virtual void OnDrawTreeGui() = 0;
        virtual void OnDestroy() const {}
        
        ///WARNING: EngineApiなのでEngine内部コードでしか使用しないで下さい。
        virtual void ImplementDestroy() {}
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::GameObject::IGameObject, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::GameObject::IGameObject);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Object::IObject, NanamiEngine::Module::GameObject::IGameObject);