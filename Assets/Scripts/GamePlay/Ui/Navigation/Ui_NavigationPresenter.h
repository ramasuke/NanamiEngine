#pragma once
#include <memory>

#include "Ui_NavigationObjective.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/LifeCycleCallback/Start/IStartable.h"
#include "Engine/Module/LifeCycleCallback/Update/IUpdatable.h"
#include "Packages/R4/R4.h"
#include "../../../../Data/Navigation/Data_NavigationGuide.h"

namespace GameCore::Scene
{
    class SceneContextBase;
}

namespace GamePlay::Ui
{
    class BillBoardNpcChatIcon;

    // NOTE: 次にすることを NavigationMemory に記録する
    class NavigationPresenter final : public Component::ComponentBase,
                                      public LifeCycleCallback::IStartable,
                                      public LifeCycleCallback::IUpdatable
    {
    public:
        [[nodiscard]] static bool IsQuiet();

    private:
        void OnStart() override;
        void OnUpdate() override;
        void OnDestroy() override;
        void Evaluate(const std::shared_ptr<GameCore::Scene::SceneContextBase>& context);
        // NOTE: 驚きアイコンを出させる相手を target の会話アイコンに付け替える
        void PointSurpriseAt(const std::shared_ptr<NanamiEngine::Module::GameObject::IGameObject>& target);

        [[serialize(0)]] FIELD(Asset::NavigationGuide) guide_;

        std::weak_ptr<GameCore::Scene::SceneContextBase> lastContext_;
        std::weak_ptr<BillBoardNpcChatIcon>              surpriseIcon_;
        NanamiEngine::R4::SerialDisposable               objectivesSubscription_;
        bool isDirty_ = true;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(guide_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(guide_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Ui::NavigationPresenter, 0);
