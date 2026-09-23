#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/Component/ImageRenderer/Animation/ImageAnimationRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"

namespace GamePlay::Ui
{
    /**
     * @brief OS カーソルの代わりにマウス位置へ出すカーソル。ゲームビルドでだけ表示する。
     * 三人称カメラがマウスを中央に留めている間と、ウィンドウが非アクティブの間は隠す
     */
    class GameCursor final : public Component::ComponentBase,
                             public LifeCycleCallback::IStartable,
                             public LifeCycleCallback::IUpdatable
    {
    private:
        void OnStart() override;
        void OnUpdate() override;

        [[nodiscard]] bool ShouldShow(int mouseX, int mouseY) const;
        void SetVisible(bool visible);
        void SetPressed(bool pressed);

        [[serialize(0)]] FIELD(GameObject::IGameObject) visualRoot_;
        [[serialize(0)]] FIELD(NanamiUi::ImageAnimationRenderer) idle_;
        [[serialize(0)]] FIELD(NanamiUi::ImageAnimationRenderer) press_;
        // クリックのアニメを出している時間
        [[serialize(0)]] float pressDuration_secs_ = 0.24f;

        bool isVisible_ = false;
        bool wasMouseDown_ = false;
        float pressRemaining_secs_ = 0.0f;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(visualRoot_));
            archive(CEREAL_NVP(idle_));
            archive(CEREAL_NVP(press_));
            archive(CEREAL_NVP(pressDuration_secs_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(visualRoot_));
            if (version >= 0) archive(CEREAL_NVP(idle_));
            if (version >= 0) archive(CEREAL_NVP(press_));
            if (version >= 0) archive(CEREAL_NVP(pressDuration_secs_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::GameCursor, 0);
