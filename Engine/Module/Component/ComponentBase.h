#pragma once
#include <memory>

#include "../LifeCycleCallback/Render/IRenderable.h"
#include "../cereal/include/cereal/types/polymorphic.hpp"
#include "../../../Libs/LibCore/ImGui/Helper/ImGuiHelper.h"
#include "../../Core/Object/IObject.h"
#include "../GameObject/ComponentGroup/ComponentGroup.h"
#include "../GameObject/Interface/IGameObject.h"
#include "../Namespace/EngineNamespace.h"
#include "../../../Packages/R4/Core/CancellationToken/R4_CancellationToken.h"

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Component
{
    class ComponentBase : public virtual Object::IObject
    {
    public:
        ComponentBase();
        virtual ~ComponentBase() override;
        void InitComponent(const std::weak_ptr<GameObject::IGameObject>& ownerGameObject);

        [[nodiscard]] GameObject::ComponentGroup& Components() const { return Entity().lock()->Components(); }
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        [[nodiscard]] std::weak_ptr<GameObject::IGameObject> Entity() const { return gameObjectRef_; }
        [[nodiscard]] GameObject::Transform& Transform() const { return gameObjectRef_.lock()->Transform(); }
        //TODO: 現在は行っていないがRemoveComponentでもOnDestroy()を呼ぶようにする必要がある。
        //NOTE: Componentが破棄されるタイミングで呼ばれる関数
        virtual void OnDestroy() { }
        virtual void BasedOnDrawgui() { }
        virtual void OnDrawGui() override;
        //NOTE: この関数を何かしらの方法でカプセル化した方が安全
        //WARNING: エンジン開発者以外使用しないでください。 
        void ResetGuid();
        void SetEnable(bool enable);
        [[nodiscard]] bool IsEnable() const;
        //NOTE: このComponentが破棄される時にキャンセルされるトークン。
        //      購読は Subscribe(...).AddTo(this)、破棄時の後始末は DestroyCancellationToken().Register(...)
        [[nodiscard]] R4::CancellationToken DestroyCancellationToken() const { return destroyCancellationTokenSource_.Token(); }
        //WARNING: エンジン開発者以外使用しないでください。
        void ImplementCancelOnDestroy();

    private:
        Guid guid_;
        bool isEnable_ = true;
        std::weak_ptr<GameObject::IGameObject> gameObjectRef_;
        R4::CancellationTokenSource destroyCancellationTokenSource_;

    protected:
        template <typename T>
        requires std::is_base_of_v<ComponentBase, T>
        std::shared_ptr<T> RequireComponent();
        
#pragma region Serialization Function
    public:
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(guid_    ));
            archive(CEREAL_NVP(isEnable_));
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(guid_    ));
            archive(CEREAL_NVP(isEnable_));
        }
#pragma endregion
    };

    template <typename T> requires std::is_base_of_v<ComponentBase, T>
    std::shared_ptr<T> ComponentBase::RequireComponent()
    {
        return Components().RequireComponent<T>();
    }
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::ComponentBase, 0);
// NOTE: ENGINE_REGISTER_COMPONENT(T) はコンポーネントの .cpp に書き、CEREAL_CLASS_VERSION(T, V) はヘッダに残す
// WARNING: 旧形式の ENGINE_REGISTER_COMPONENT(T, V) もビルドは通るが、ヘッダに書くと include 先すべてで保存・読み込みコードが生成される
#define ENGINE_REGISTER_COMPONENT_TYPE_(TYPE)                                                   \
    CEREAL_REGISTER_TYPE(TYPE);                                                                 \
    CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, TYPE);
#define ENGINE_REGISTER_COMPONENT_WITH_VERSION_(TYPE, VERSION)                                  \
    CEREAL_CLASS_VERSION(TYPE, VERSION);                                                        \
    ENGINE_REGISTER_COMPONENT_TYPE_(TYPE)
#define ENGINE_REGISTER_COMPONENT_EXPAND_(x) x
#define ENGINE_REGISTER_COMPONENT_SELECT_(_1, _2, NAME, ...) NAME
#define ENGINE_REGISTER_COMPONENT(...)                                                          \
    ENGINE_REGISTER_COMPONENT_EXPAND_(ENGINE_REGISTER_COMPONENT_SELECT_(__VA_ARGS__,            \
        ENGINE_REGISTER_COMPONENT_WITH_VERSION_, ENGINE_REGISTER_COMPONENT_TYPE_, )(__VA_ARGS__))