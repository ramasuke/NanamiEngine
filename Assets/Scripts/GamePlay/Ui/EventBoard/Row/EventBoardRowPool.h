#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../Model/BoardListCursor.h"

namespace GamePlay::Ui
{
    /**
     * @brief 掲示板・店の一覧の行。行の prefab を root の下へ縦に並べて持つ。
     * スクロール部品は無いので、行は表示窓の分だけ作り、窓がずれたら中身を貼り替えて使い回す。
     */
    template<typename RowT>
    class EventBoardRowPool final
    {
    public:
        /** @brief 行を count 枚作る。作り済みなら何もしない */
        void Build(
            const FIELD(Asset::PrefabGameObjectFile)& rowPrefab,
            const FIELD(GameObject::IGameObject)& rowsRoot,
            const size_t count,
            const float rowSpacing_px)
        {
            if (IsBuilt() || !rowPrefab || !rowsRoot)
                return;

            const auto rowsObject = rowsRoot.get();
            for (size_t i = 0; i < count; ++i)
            {
                const auto rowObject = Scene::GameObject::Instantiate(*rowPrefab.get(), rowsObject).lock();
                if (!rowObject)
                    continue;

                rowObject->Transform().SetLocalPos(glm::vec3(0.0f, static_cast<float>(i) * rowSpacing_px, 0.0f));
                rows_.push_back(rowObject->Components().Catch<RowT>());
            }
        }

        [[nodiscard]] bool   IsBuilt() const { return !rows_.empty(); }
        [[nodiscard]] size_t Size   () const { return rows_.size(); }

        /** @brief 生きている行ごとに onRow(行, 表示窓の中での番号) を呼ぶ */
        template<typename F>
        void ForEach(F&& onRow) const
        {
            for (size_t i = 0; i < rows_.size(); ++i)
            {
                if (const auto row = rows_[i].lock())
                    onRow(*row, i);
            }
        }

        /** @brief 行がクリックされたら、その行の表示窓の中での番号を渡す */
        void SubscribeOnClick(std::function<void(size_t)> onClick) const
        {
            ForEach([&onClick](RowT& row, const size_t i)
            {
                row.SubscribeOnClickSelectButton([onClick, i]
                {
                    onClick(i);
                });
            });
        }

        /**
         * @brief 表示窓に入っている分を bindRow(行, 項目) で行へ貼り、上下に続きがあれば矢印を出す
         * @return 貼った行の数
         */
        template<typename EntryT, typename BindRowT>
        size_t Bind(
            const std::vector<EntryT>& entries,
            const BoardListCursor& cursor,
            const FIELD(Component::ImageRenderer)& moreAboveMark,
            const FIELD(Component::ImageRenderer)& moreBelowMark,
            BindRowT&& bindRow) const
        {
            const size_t first = cursor.FirstVisibleIndex();
            size_t shownRows = 0;
            ForEach([&](RowT& row, const size_t i)
            {
                const size_t index = first + i;
                if (index >= entries.size())
                    return;

                bindRow(row, entries[index]);
                row.SetHighlighted(index == cursor.SelectedIndex());
                ++shownRows;
            });

            if (const auto mark = moreAboveMark.get())
                mark->SetEnable(first > 0);
            if (const auto mark = moreBelowMark.get())
                mark->SetEnable(first + rows_.size() < entries.size());
            return shownRows;
        }

        /** @brief 項目をそのまま RowT::Bind に渡す */
        template<typename EntryT>
        size_t Bind(
            const std::vector<EntryT>& entries,
            const BoardListCursor& cursor,
            const FIELD(Component::ImageRenderer)& moreAboveMark,
            const FIELD(Component::ImageRenderer)& moreBelowMark) const
        {
            return Bind(entries, cursor, moreAboveMark, moreBelowMark, [](RowT& row, const EntryT& entry) { row.Bind(entry); });
        }

    private:
        std::vector<std::weak_ptr<RowT>> rows_;
    };
}
