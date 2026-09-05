#pragma once
#include <memory>
#include <cstddef>

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;

    // ヒエラルキーツリー上で、あるノードの子リスト中の「挿入位置」を表す薄いドラッグ&ドロップ
    // 受け皿を1つ描画する。各 IGameObject::OnDrawTreeGui 実装は、子を描画するループの中で
    // 子の直前 + 最後の子の後の計 (children.size() + 1) 回、これを呼び出す想定。
    //
    // parent: 挿入先の親（parent->Transform() の children_ に挿入される）
    // insertIndex: 挿入位置（ドロップ時点、ドラッグ中オブジェクト除去前の parent の子リストに
    //              おけるインデックス。0=先頭、children.size()=末尾）
    void DrawSiblingInsertionDropZone(const std::shared_ptr<IGameObject>& parent, std::size_t insertIndex);
}
