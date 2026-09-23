#pragma once
#include "../ComponentBase.h"
#include "../../LifeCycleCallback/Update/IUpdatable.h"

namespace NanamiEngine::Module::Component
{
	class Rotator final : public ComponentBase,
						  public LifeCycleCallback::IUpdatable
	{
	public:
		void SetRotateSpeedDegPerSec(const float speed) { rotateSpeedDegPerSec_ = speed; }
		[[nodiscard]] float GetRotateSpeedDegPerSec() const { return rotateSpeedDegPerSec_; }

	private:
		void OnUpdate() override;

		glm::vec3 rotateAxis_          = glm::vec3(0.0f, 1.0f, 0.0f);
		float     rotateSpeedDegPerSec_ = 5.0f;

#pragma region Serialization Function
public:
void OnDrawGui() override;

		template<class Archive>
void save(Archive& archive, const std::uint32_t version) const {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
    archive(CEREAL_NVP(rotateAxis_));
    archive(CEREAL_NVP(rotateSpeedDegPerSec_));
}

template<class Archive>
void load(Archive& archive, const std::uint32_t version) {
    archive(cereal::base_class<ComponentBase>(this));
    archive(cereal::base_class<LifeCycleCallback::IUpdatable>(this));
    if (version >= 0) archive(CEREAL_NVP(rotateAxis_));
    if (version >= 0) archive(CEREAL_NVP(rotateSpeedDegPerSec_));
}
#pragma endregion
	};
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::Rotator, 0);
