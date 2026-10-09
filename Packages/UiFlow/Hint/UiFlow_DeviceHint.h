#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>

#include "../Device/UiFlow_InputDevice.h"
#include "../../../Engine/Core/Object/Field/Field.h"
#include "../../../Engine/Module/Asset/Sprite/SpriteFile.h"
#include "../../../Engine/Module/Component/ComponentBase.h"
#include "../../../Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "../../../Engine/Module/LifeCycleCallback/Update/IUpdatable.h"

namespace NanamiEngine::Module::NanamiUi
{
    class IInteractivableRenderer;
}

namespace NanamiEngine::UiFlow
{
    // NOTE: 操作ヒントの札を、いま使われている入力機器のものへ差し替える。札の Renderer と同じ GameObject に付ける
    // NOTE: その機器の Sprite が未設定なら、いまの絵のままにする
    class NANAMI_API DeviceHint final : public Component::ComponentBase,
                                        public LifeCycleCallback::IStartable,
                                        public LifeCycleCallback::IUpdatable
    {
    private:
        void OnStart () override;
        void OnUpdate() override;

        void Apply(InputDeviceKind device);

        [[serialize(0)]] FIELD(Asset::SpriteFile) keyboardSprite_;
        [[serialize(0)]] FIELD(Asset::SpriteFile) gamepadSprite_;

        std::weak_ptr<Module::NanamiUi::IInteractivableRenderer> renderer_;
        InputDeviceKind applied_ = InputDeviceKind::KeyboardMouse;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(keyboardSprite_));
            archive(CEREAL_NVP(gamepadSprite_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(keyboardSprite_));
            if (version >= 0) archive(CEREAL_NVP(gamepadSprite_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::UiFlow::DeviceHint, 0);
