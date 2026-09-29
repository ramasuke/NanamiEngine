#pragma once
#include "../../../Core/Game/PlayerAvatar/InputAction/PlayerAvatarInputDevice.h"
#include "../../../Core/Game/PlayerAvatar/Item/ItemPouch.h"
#include "../../../Core/Game/PlayerAvatar/State/Transition/PlayerAvatarControlAcceptance.h"

namespace GamePlay::Ui
{
    /// State が宣言したアイテムの操作
    struct ItemBarDeclaration final
    {
        GameCore::PlayerAvatar::PlayerAvatarControlAcceptance acceptance = GameCore::PlayerAvatar::PlayerAvatarControlAcceptance::None;
        /// isShown / isUsable は acceptance が Accept のときだけ意味を持つ
        bool isShown  = false;
        bool isUsable = false;
    };

    // アイテム欄が見るアバター。アバターの種類ごとの違い(State の宣言の読み方)はこの実装に閉じ込める
    class IItemBarSource
    {
    public:
        virtual ~IItemBarSource() = default;
        /** @return アバターが消えていたら nullptr */
        [[nodiscard]] virtual GameCore::PlayerAvatar::ItemPouch* Pouch() const = 0;
        [[nodiscard]] virtual GameCore::PlayerAvatar::PlayerAvatarInputDevice CurrentDevice() const = 0;
        [[nodiscard]] virtual ItemBarDeclaration Declaration() const = 0;
    };
}
