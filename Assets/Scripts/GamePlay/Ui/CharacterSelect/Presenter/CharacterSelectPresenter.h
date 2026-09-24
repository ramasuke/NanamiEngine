#pragma once
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "../Model/CharacterSelectModel.h"
#include "../../../Sound/UiSoundBank.h"

namespace GamePlay::Ui
{
    class CharacterSelectUi;
}

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Prop
{
    class CharacterPodium;
}

namespace GamePlay::Ui
{
    class CharacterSelectPresenter final : public Component::ComponentBase,
                                           public LifeCycleCallback::IStartable,
                                           public LifeCycleCallback::IUpdatable
    {
    public:
        /**
         * @brief 展示台を渡す。OnStart の前後どちらでもよく、両方揃った時点で開く
         * @details Instantiate の中で OnStart まで走るので、生成直後の Bind は OnStart の後になる
         */
        void Bind(const std::weak_ptr<Prop::CharacterPodium>& podium);

    private:
        void OnStart  () override;
        void OnUpdate () override;
        void OnDestroy() override;

        void Open();
        [[nodiscard]] bool IsAnotherOpen() const;
        /** @brief 開く前に捨てる。アバターや展示台のカメラには触らない */
        void Discard();
        void Confirm();
        /** @param didSwitch 差し替えた後は新しいアバターが操作可能な状態で出来ているので、元のアバターは触らない */
        void Close(bool didSwitch);

        std::shared_ptr<CharacterSelectUi> view_;
        std::unique_ptr<CharacterSelectModel> model_;
        std::weak_ptr<GameCore::IPlayerAvatar> suspendedAvatar_;
        std::weak_ptr<Prop::CharacterPodium> podium_;

        bool wasPrevPressed_    = false;
        bool wasNextPressed_    = false;
        bool wasConfirmPressed_ = false;
        bool wasCancelPressed_  = false;
        bool isClosed_ = false;
        bool hasStarted_ = false;
        // 会話のたびに二重に生えるのを防ぐ
        bool isOpen_ = false;

        [[serialize(1)]] FIELD(Asset::UiSoundBankData) uiSounds_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<typename Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            archive(CEREAL_NVP(uiSounds_));
        }

        template<typename Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<Component::ComponentBase>(this));
            if (version >= 1) archive(CEREAL_NVP(uiSounds_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::CharacterSelectPresenter, 1);
