#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "vec2.hpp"
#include "../../../../Libs/LibCore/cereal/glm/GlmHelper.h"
#include "../../../../Packages/R4/R4.h"
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"
#include "../../Component/ComponentBase.h"
#include "../MouseEvent/MouseState.h"

namespace NanamiEngine::Module::NanamiUi
{
    class IInteractivableRenderer;
}

namespace NanamiEngine::Module::NanamiUi
{
    class NANAMI_API Button final : public Component::ComponentBase,
                         public LifeCycleCallback::IAwakable,
                         public LifeCycleCallback::IUpdatable
    {
    public:
        [[nodiscard]] R4::Observable<MouseState> OnClick  () const { return onClick  .AsObservable(); } 
        [[nodiscard]] R4::Observable<R4::Unit  > OnHover  () const { return onHover  .AsObservable(); }
        [[nodiscard]] R4::Observable<R4::Unit  > OnHoverExit() const { return onHoverExit.AsObservable(); }
        [[nodiscard]] R4::Observable<R4::Unit  > OnRelease() const { return onRelease.AsObservable(); }

        // 有効な Button が直近で更新されていれば true (マウスで操作する UI が出ている)
        [[nodiscard]] static bool IsAnyActive();

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

        R4::Subject<MouseState> onClick;
        R4::Subject<R4::Unit  > onHover;
        R4::Subject<R4::Unit  > onHoverExit;
        R4::Subject<R4::Unit  > onRelease;
        
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

CEREAL_CLASS_VERSION(NanamiUi::Button, 1);
