#include "ParameterGroup.h"

#include "../AnimationParameter.h"
#include "../../ImGui/Helper/ImGuiHelper.h"

using namespace NanamiEngine::Module::BlackBoard;

void ParameterGroup::SubscribeToAllParameters()
{
    subscriptions_.unsubscribe();
    subscriptions_ = rxcpp::composite_subscription();

    for (const auto& param : conditionParameters_)
    {
        param->OnChanged().subscribe(subscriptions_, [this](LibCore::Rx::unit)
        {
            NanamiEngine::Core::Network::ByteBuffer buffer;
            const auto count = static_cast<uint32_t>(conditionParameters_.size());
            buffer.WriteRaw(count);
            for (const auto& p : conditionParameters_)
                p->WriteValueTo(buffer);
            paramChangedSubject_.get_subscriber().on_next(std::move(buffer));
        });
    }
}

rxcpp::observable<NanamiEngine::Core::Network::ByteBuffer>
ParameterGroup::OnParameterChanged() const
{
    return paramChangedSubject_.get_observable();
}

void ParameterGroup::ApplyFromBuffer(
    const NanamiEngine::Core::Network::ByteBuffer& buffer, size_t& offset)
{
    const uint32_t count = buffer.ReadRaw<uint32_t>(offset);
    for (size_t i = 0; i < count && i < conditionParameters_.size(); ++i)
        conditionParameters_[i]->ReadValueFrom(buffer, offset);
}

void ParameterGroup::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("parameters", conditionParameters_, [this]
    {
        if (ImGui::Button("Add"))
        {
            ImGui::OpenPopup("AddParameterPopup");
        }

        if (ImGui::BeginPopup("AddParameterPopup"))
        {
            static int selectedType = 0;
            const char* types[] = { "Bool", "Int", "Float" };

            ImGui::Text("Select Parameter Type:");
            ImGui::Combo("Type", &selectedType, types, IM_ARRAYSIZE(types));

            if (ImGui::Button("Confirm"))
            {
                switch (selectedType)
                {
                case 0:
                    conditionParameters_.push_back(std::make_shared<AnimationTree::AnimationParameter<bool>>());
                    break;
                case 1:
                    conditionParameters_.push_back(std::make_shared<AnimationTree::AnimationParameter<int>>());
                    break;
                case 2:
                    conditionParameters_.push_back(std::make_shared<AnimationTree::AnimationParameter<float>>());
                    break;
                default:
                    break;
                }

                SubscribeToAllParameters();
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    });
}