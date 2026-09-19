#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "../../../../../../Engine/Core/Object/Field/Field.h"
#include "../../../../../../Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../../../../../../Engine/Module/Component/ImageRenderer/ImageRenderer.h"
#include "../../../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../Model/BoardListCursor.h"

namespace GamePlay::Ui
{
    /**
     * @brief 行の prefab を root の下へ count 枚だけ縦に並べる。
     * スクロール部品は無いので、行は表示窓の分だけ作り、窓がずれたら中身を貼り替えて使い回す。
     */
    template<typename RowT>
    std::vector<std::weak_ptr<RowT>> InstantiateEventBoardRows(
        const FIELD(Asset::PrefabGameObjectFile)& rowPrefab,
        const FIELD(GameObject::IGameObject)& rowsRoot,
        const size_t count,
        const float rowSpacing_px)
    {
        std::vector<std::weak_ptr<RowT>> rows;
        if (!rowPrefab || !rowsRoot)
            return rows;

        const auto rowsObject = rowsRoot.get();
        for (size_t i = 0; i < count; ++i)
        {
            const auto rowObject = Scene::GameObject::Instantiate(*rowPrefab.get(), rowsObject).lock();
            if (!rowObject)
                continue;

            rowObject->Transform().SetLocalPos(glm::vec3(0.0f, static_cast<float>(i) * rowSpacing_px, 0.0f));
            rows.push_back(rowObject->Components().Catch<RowT>());
        }
        return rows;
    }

    /** @brief 行がクリックされたら、その行の表示窓の中での番号を渡す */
    template<typename RowT>
    void SubscribeOnClickEventBoardRows(const std::vector<std::weak_ptr<RowT>>& rows, std::function<void(size_t)> onClick)
    {
        for (size_t i = 0; i < rows.size(); ++i)
        {
            if (const auto row = rows[i].lock())
            {
                row->SubscribeOnClickSelectButton([onClick, i]
                {
                    onClick(i);
                });
            }
        }
    }

    /** @brief 表示窓に入っている分を行へ貼り、上下に続きがあれば矢印を出す */
    template<typename RowT, typename EntryT>
    void BindEventBoardRows(
        const std::vector<std::weak_ptr<RowT>>& rows,
        const std::vector<EntryT>& entries,
        const BoardListCursor& cursor,
        const FIELD(Component::ImageRenderer)& moreAboveMark,
        const FIELD(Component::ImageRenderer)& moreBelowMark)
    {
        const size_t first = cursor.FirstVisibleIndex();
        for (size_t i = 0; i < rows.size(); ++i)
        {
            const auto row = rows[i].lock();
            if (!row)
                continue;

            const size_t index = first + i;
            if (index >= entries.size())
                continue;

            row->Bind(entries[index]);
            row->SetHighlighted(index == cursor.SelectedIndex());
        }

        if (const auto mark = moreAboveMark.get())
            mark->SetEnable(first > 0);
        if (const auto mark = moreBelowMark.get())
            mark->SetEnable(first + rows.size() < entries.size());
    }
}
