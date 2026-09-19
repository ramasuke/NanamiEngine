#pragma once
#include <memory>

namespace NanamiEngine::Module::Asset
{
    class ItemData;
}

namespace GameCore::PlayerAvatar::Item
{
    // 拾ったアイテムを受け取る側。ポーチを持たないアバターは実装しないので、拾えずに地面へ残る
    class IItemReceiver
    {
    public:
        virtual ~IItemReceiver() = default;
        [[nodiscard]] virtual int ReceivableCount(const NanamiEngine::Module::Asset::ItemData& item) const = 0;
        /** @return 実際に入った数 */
        virtual int ReceiveItem(const std::shared_ptr<NanamiEngine::Module::Asset::ItemData>& item, int count) = 0;
    };
}
