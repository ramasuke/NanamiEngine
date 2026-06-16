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
void OnDrawGui() override;

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

ENGINE_REGISTER_COMPONENT(NanamiEngine::Module::Component::SphereRenderer, 0)