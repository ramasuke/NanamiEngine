#pragma once
#include "vec2.hpp"
#include "../../../../Libs/LibCore/Rx/SerializableSubject/unit/unit.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"
#include "../../Component/ComponentBase.h"
#include "../MouseEvent/MouseState.h"
#include "../rxcpp/rx.hpp"

namespace NanamiEngine::Module::NanamiUi
{
    class IInteractivableRenderer;
}

namespace NanamiEngine::Module::NanamiUi
{
    class Button final : public Component::ComponentBase,
                         public LifeCycleCallback::IAwakable,
                         public LifeCycleCallback::IUpdatable
    {
    public:
        [[nodiscard]] rxcpp::observable<MouseState> OnClick  () const { return onClick  .get_observable(); } 
        [[nodiscard]] rxcpp::observable<Rx::unit  > OnHover  () const { return onHover  .get_observable(); }
        [[nodiscard]] rxcpp::observable<Rx::unit  > OnRelease() const { return onRelease.get_observable(); } 

    private:
        void OnAwake() override;
        void OnUpdate() override;

        [[nodiscard]] bool CheckInnerMousePointer() const;
        void TryClick();
        void TryHover();
        void TryRelease();


        [[serialize(0)]] glm::vec2 eventAreaSize_{100.0f, 100.0f};
        [[serialize(1)]] FIELD(Asset::SpriteFile) onIdleSprite_;
        [[serialize(1)]] FIELD(Asset::SpriteFile) onHoverSprite_;
        
        std::weak_ptr<IInteractivableRenderer> renderer_;
        bool isPressing_ = false;
        bool isHovering_ = false;

        rxcpp::subjects::subject<MouseState> onClick;
        rxcpp::subjects::subject<Rx::unit  > onHover;
        rxcpp::subjects::subject<Rx::unit  > onRelease;
        
#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(eventAreaSize_));
            if (version >= 1) archive(CEREAL_NVP(onIdleSprite_));
            if (version >= 1) archive(CEREAL_NVP(onHoverSprite_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(eventAreaSize_));
            if (version >= 1) archive(CEREAL_NVP(onIdleSprite_));
            if (version >= 1) archive(CEREAL_NVP(onHoverSprite_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiUi::Button, 1);
CEREAL_REGISTER_TYPE(NanamiUi::Button);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiUi::Button);
#pragma endregion