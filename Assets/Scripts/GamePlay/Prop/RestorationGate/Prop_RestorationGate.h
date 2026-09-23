#pragma once
#include <memory>

#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../../../Core/Game/Story/Story_Facility.h"

namespace GamePlay::Prop
{
    /**
     * @brief facility_ が直っているかどうかで、壊れた見た目と直った見た目の GameObject を切り替える。
     *        StoryProgress が変わるとその場で切り替わる。
     *        掲示板の「復興」で選ばれている間は下見として、直った見た目を出して previewCamera_ へ寄せる
     */
    class RestorationGate final : public Component::ComponentBase,
                                  public LifeCycleCallback::IStartable
    {
    public:
        /** @brief シーンにある facility の門。無ければ nullptr */
        [[nodiscard]] static std::shared_ptr<RestorationGate> Find(GameCore::Story::Facility facility);

        void BeginPreview();
        void EndPreview();

    private:
        void OnStart() override;
        void Apply() const;

        // NOTE: tools.scene で設定できるよう Story::Facility を int で持つ
        [[serialize(0)]] int facility_ = 0;
        [[serialize(0)]] FIELD(GameObject::IGameObject) brokenObject_;
        [[serialize(0)]] FIELD(GameObject::IGameObject) restoredObject_;
        [[serialize(1)]] FIELD(CineMachine::CineMachineVirtualCamera) previewCamera_;
        [[serialize(1)]] int previewPriority_ = 100;

        bool isPreviewing_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(facility_));
            archive(CEREAL_NVP(brokenObject_));
            archive(CEREAL_NVP(restoredObject_));
            archive(CEREAL_NVP(previewCamera_));
            archive(CEREAL_NVP(previewPriority_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(facility_));
            if (version >= 0) archive(CEREAL_NVP(brokenObject_));
            if (version >= 0) archive(CEREAL_NVP(restoredObject_));
            if (version >= 1) archive(CEREAL_NVP(previewCamera_));
            if (version >= 1) archive(CEREAL_NVP(previewPriority_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::RestorationGate, 1);
