#pragma once
#include <memory>

#include "../../../../../../Engine/Module/Component/ComponentBase.h"
#include "../../../../../../Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../Model/PauseMenuModel.h"

namespace GamePlay::PlayerAvatar::SwordMan
{
    class SwordManAvatar;
}

namespace GamePlay::Ui
{
    class PauseMenuUi;

    /**
     * @brief 冒険者の手帳(＠メニュー)の開閉と目次の操作。
     * 常駐して ＠ / START を見張り、開けるかは今の State が OpenMenu を宣言しているかで決める
     * (攻撃中・空中・大砲・会話やキャラ選択で止められている間は宣言されないので開かない)。
     */
    class PauseMenuPresenter final : public Component::ComponentBase,
                                     public LifeCycleCallback::IUpdatable
    {
    public:
        void Initialize(const std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar>& swordManAvatar);

    private:
        class OpenMenuDeclaration;

        struct Keys
        {
            bool prev    = false;
            bool next    = false;
            bool confirm = false;
            bool cancel  = false;
        };

        void OnUpdate() override;
        void UpdateOpened(GamePlay::PlayerAvatar::SwordMan::SwordManAvatar& avatar);
        void Open(GamePlay::PlayerAvatar::SwordMan::SwordManAvatar& avatar);
        void Close();
        void Confirm();
        [[nodiscard]] static Keys ReadKeys();
        [[nodiscard]] static bool IsOpenMenuDeclared(const GamePlay::PlayerAvatar::SwordMan::SwordManAvatar& avatar);

        std::weak_ptr<GamePlay::PlayerAvatar::SwordMan::SwordManAvatar> swordManAvatar_;
        std::shared_ptr<PauseMenuUi> view_;
        PauseMenuModel model_;
        Keys previousKeys_;
        bool isOpen_ = false;
        // 閉じたキー(B=ジャンプなど)を同じフレームで State に拾わせないよう、戻すのは次のフレーム
        bool isResumePending_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override {}

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
        }
#pragma endregion
    };
}

ENGINE_REGISTER_COMPONENT(GamePlay::Ui::PauseMenuPresenter, 0)
