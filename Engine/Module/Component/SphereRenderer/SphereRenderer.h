#pragma once
#include "../ComponentBase.h"

#include "../../Color/Color32.h"

namespace NanamiEngine::Module::Component
{
	class SphereRenderer final : public ComponentBase,
								 public LifeCycleCallback::IRenderable
	{
	private:
		void OnRender() override;

		[[serialize(0)]] float   radius_ = 0.2f;
		[[serialize(0)]] int	 divNum_ = 16;
		[[serialize(0)]] Color32 color_;
		[[serialize(0)]] Color32 edgeColor_;
		[[serialize(0)]] bool	 fill_ = false;
#pragma region Serialization Function
public:
void OnDrawGui() {
    LibCore::ImGuiHelper::OnDrawInputField("radius_", radius_);
    LibCore::ImGuiHelper::OnDrawInputField("divNum_", divNum_);
    LibCore::ImGuiHelper::OnDrawInputField("color_", color_);
    LibCore::ImGuiHelper::OnDrawInputField("edgeColor_", edgeColor_);
    LibCore::ImGuiHelper::OnDrawInputField("fill_", fill_);
}

template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    archive(CEREAL_NVP(radius_));
    archive(CEREAL_NVP(divNum_));
    archive(CEREAL_NVP(color_));
    archive(CEREAL_NVP(edgeColor_));
    archive(CEREAL_NVP(fill_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IRenderable>(this));
    if (version >= 0) archive(CEREAL_NVP(radius_));
    if (version >= 0) archive(CEREAL_NVP(divNum_));
    if (version >= 0) archive(CEREAL_NVP(color_));
    if (version >= 0) archive(CEREAL_NVP(edgeColor_));
    if (version >= 0) archive(CEREAL_NVP(fill_));
}
#pragma endregion
};
}
#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::SphereRenderer, 0);
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Component::SphereRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Component::ComponentBase, NanamiEngine::Module::Component::SphereRenderer);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IRenderable, NanamiEngine::Module::Component::SphereRenderer);
#pragma endregion