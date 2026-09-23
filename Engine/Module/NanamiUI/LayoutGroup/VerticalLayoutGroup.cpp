#include "VerticalLayoutGroup.h"
#include <algorithm>
#include <vector>
#include "LayoutElement.h"
#include "../../GameObject/ComponentGroup/ComponentGroup.h"
#include "../../GameObject/Transform/Transform.h"
#include "../../GameObject/Interface/IGameObject.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::NanamiUi
{
    namespace
    {
        bool HasEnabledComponent(GameObject::IGameObject& gameObject)
        {
            const auto components = gameObject.Components().Catches<Component::ComponentBase>();
            if (components.empty())
                return true;

            return std::ranges::any_of(components, [](const std::weak_ptr<Component::ComponentBase>& component)
            {
                const auto locked = component.lock();
                return locked && locked->IsEnable();
            });
        }

        float MainAxisRateOf(GameObject::IGameObject& gameObject)
        {
            const auto layoutElement = gameObject.Components().Catch<LayoutElement>().lock();
            return layoutElement ? layoutElement->MainAxisRate() : 1.0f;
        }
    }

    void VerticalLayoutGroup::OnLateUpdate()
    {
        if (!IsEnable())
            return;

        std::vector<std::shared_ptr<GameObject::IGameObject>> children = Transform().GetChildren();
        if (ignoreDisabledChildren_)
            std::erase_if(children, [](const std::shared_ptr<GameObject::IGameObject>& child) { return !HasEnabledComponent(*child); });

        if (children.empty())
            return;
        if (reverseArrangement_)
            std::ranges::reverse(children);

        const float offsetX = ToCrossAlignFactor(childAlignment_) * cellSize_.x;
        const float pitch   = cellSize_.y + spacing_;

        // 割合 r の子は p*r の枠を占め、その枠の中央に置く。全員 r=1 なら i*p に一致する
        std::vector<float> alongs;
        alongs.reserve(children.size());
        float filled = 0.0f;
        for (const auto& child : children)
        {
            const float rate = MainAxisRateOf(*child);
            alongs.push_back(filled + pitch * (rate - 1.0f) * 0.5f);
            filled += pitch * rate;
        }

        const float centerShift = centerOnOrigin_ ? (filled - spacing_ - cellSize_.y) * 0.5f : 0.0f;
        const float direction   = stackUpward_ ? -1.0f : 1.0f;

        for (std::size_t i = 0; i < children.size(); ++i)
        {
            auto& childTransform = children[i]->Transform();
            const glm::vec3& current = childTransform.GetLocalPos();
            childTransform.SetLocalPos(glm::vec3(
                offsetX,
                (alongs[i] - centerShift) * direction,
                current.z
            ));
        }
    }

    void VerticalLayoutGroup::OnDrawGui()
    {
        float cellSize[2] = { cellSize_.x, cellSize_.y };
        if (ImGui::InputFloat2("cellSize_", cellSize))
        {
            cellSize_.x = cellSize[0];
            cellSize_.y = cellSize[1];
        }
        ImGuiHelper::OnDrawInputField("spacing_", spacing_);
        ImGuiHelper::OnDrawEnumField("childAlignment_", childAlignment_, LAYOUT_CROSS_ALIGNS, ToString);
        ImGuiHelper::OnDrawInputField("reverseArrangement_", reverseArrangement_);
        ImGuiHelper::OnDrawInputField("ignoreDisabledChildren_", ignoreDisabledChildren_);
        ImGuiHelper::OnDrawInputField("stackUpward_", stackUpward_);
        ImGuiHelper::OnDrawInputField("centerOnOrigin_", centerOnOrigin_);
    }
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::NanamiUi::VerticalLayoutGroup);
#pragma endregion
