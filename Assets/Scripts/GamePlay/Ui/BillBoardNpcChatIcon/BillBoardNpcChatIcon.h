#pragma once
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/NanamiUI/BillBoard3D/BillboardAnimation3D.h"
#include "Engine/Module/NanamiUI/BillBoard3D/DrawBillboard3D.h"

namespace GamePlay::Ui
{
    class BillBoardNpcChatIcon final : public Component::ComponentBase,
                                       public LifeCycleCallback::IUpdatable
    {
    public:
        void Show(
            bool chattableIcon,
            bool chattingIcon,
            bool surpriseIcon);
        void Hide();
        void OnChattable();
        void OnExitChattable();

    private:
        enum class IconMotion
        {
            Surprise,   // ゆっくり上下 + 定期的にコトッと傾いた直後に枠を光が走る
            Chattable,  // 下向きに弾む
            Chatting,   // 呼吸するように拡大縮小
        };

        // アイコンごとの演出状態（シリアライズしない）
        struct IconState
        {
            bool      isCaptured     = false;
            bool      wasEnabled     = false;
            float     shownTime_secs = 0.0f;
            glm::vec3 basePos        = {};
            glm::vec3 baseScale      = {};
            float     baseAngle      = 0.0f;
            std::weak_ptr<NanamiUi::Billboard3D> billboard;
        };

        void OnUpdate() override;
        static void UpdateIcon(
            const std::shared_ptr<GameObject::IGameObject>& object,
            const std::shared_ptr<NanamiUi::BillboardAnimation3D>& rimGlow,
            IconState& state,
            IconMotion motion);

        bool isShow_ = true;
        IconState chattableState_;
        IconState chattingState_;
        IconState surpriseState_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) chattableIcon_; 
        [[serialize(0)]] FIELD(GameObject::IGameObject) chattingIcon_;
        [[serialize(1)]] FIELD(GameObject::IGameObject) surpriseIcon_;
        [[serialize(2)]] FIELD(NanamiUi::BillboardAnimation3D) surpriseRimGlow_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(chattableIcon_));
            archive(CEREAL_NVP(chattingIcon_));
            archive(CEREAL_NVP(surpriseIcon_));
            archive(CEREAL_NVP(surpriseRimGlow_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(chattableIcon_));
            if (version >= 0) archive(CEREAL_NVP(chattingIcon_));
            if (version >= 1) archive(CEREAL_NVP(surpriseIcon_));
            if (version >= 2) archive(CEREAL_NVP(surpriseRimGlow_));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::BillBoardNpcChatIcon, 2)
