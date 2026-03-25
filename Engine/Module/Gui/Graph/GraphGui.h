#pragma once
#include <memory>

#include "vec2.hpp"
#include "../../../Core/Object/IObject.h"
#include "../ImGui/ImGuiHelper.h"
#include "NodeDrawResult/NodeDrawResult.h"
#include "NodeOption/NodeOption.h"

namespace NanamiEngine::Module::Gui::Graph
{
    void DrawGrid(ImDrawList* drawList,
                  const ImVec2& offset,
                  const ImVec2& windowSize,
                  float gridStep = 64.0f,
                  ImU32 color = IM_COL32(60, 60, 60, 255));

    /**
     * @note Nodeの描画, NodeをDragの位置移動, NodeをクリックのInspector表示を行う。
     */
    NodeDrawResult DrawNode(ImVec2 offset, glm::vec2& nodePosition, ImDrawList* drawList, const std::weak_ptr<Object::IObject>& drawObject, const NodeOption& option, const Guid& nodeGuid);
    
    /**
     * @note Draw()なのに返り値がboolなのが意味が通っていないが、ImGuiが直前に生成したImGuiItemを即時判定することによって作成していく思想に沿っているため。
     * @param drawList 描画するDrawList
     * @param position Portの中心位置
     * @param color 描画するPortの色
     */
    bool DrawInputPort (ImDrawList* drawList, const ImVec2& position, ImU32 color);
    
    /**
     * @note Draw()なのに返り値がboolなのが意味が通っていないが、ImGuiが直前に生成したImGuiItemを即時判定することによって作成していく思想に沿っているため。 
     * @param drawList 描画するDrawList
     * @param position Portの中心位置
     * @param color 描画するPortの色
     */
    bool DrawOutputPort(ImDrawList* drawList, const ImVec2& position, ImU32 color);
}
