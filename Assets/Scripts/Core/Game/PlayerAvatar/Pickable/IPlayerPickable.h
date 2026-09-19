#pragma once

namespace GameCore::PlayerAvatar
{
    class IPlayerAvatarStatus;
}

namespace GameCore::PlayerAvatar
{
    /**
     * @brief プレイヤーが拾える物。いつ誰が拾うかはプレイヤー側の PickupArea が決める
     */
    class IPlayerPickable
    {
    public:
        virtual ~IPlayerPickable() = default;
        /** @brief 出てきた直後や、picker の持ち物に入りきらない間は false */
        [[nodiscard]] virtual bool CanPickUp(const IPlayerAvatarStatus& picker) const = 0;
        /** @brief 拾われた。中身を拾った人のステータスへ渡し、自分を片付ける */
        virtual void OnPickUp(IPlayerAvatarStatus& pickerStatus) = 0;
    };
}
