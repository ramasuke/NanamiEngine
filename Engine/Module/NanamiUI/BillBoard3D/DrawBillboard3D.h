#pragma once
#include "../../../Core/Object/Field/Field.h"
#include "../../Asset/Sprite/SpriteFile.h"
#include "../../Component/ComponentBase.h"

namespace NanamiEngine::Module::NanamiUi
{
    class Billboard3D final : public Component::ComponentBase,
                              public LifeCycleCallback::IInitRenderable,
                              public LifeCycleCallback::IUserInterfaceRenderable
    {
    public:
        
        
    private:
        void InitRenderer() override;
        void OnUserInterfaceRender() override;
        [[nodiscard]] int GetRenderOrder() const override { return renderOrder_; }
        

        [[serialize(0)]] FIELD(Asset::SpriteFile) sprite_;
        [[serialize(0)]] int renderOrder_ = 0;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;
        
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(renderOrder_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(renderOrder_));
        }
#pragma endregion
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiUi::Billboard3D, 0);
CEREAL_REGISTER_TYPE(NanamiUi::Billboard3D);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Component::ComponentBase, NanamiUi::Billboard3D);
#pragma endregion