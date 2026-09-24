#include "Enemy_Behaviour_Action_SwapModel.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/Component/ModelRenderer/ModelRenderer.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

namespace
{
    using NanamiEngine::Module::Asset::Mv1File;
    using NanamiEngine::Module::Component::ModelRenderer;

    Coroutine::Task<void> SwapAsync(const std::weak_ptr<ModelRenderer> renderer, const std::shared_ptr<Mv1File> model)
    {
        // NOTE: 未ロードのまま SetMv1File すると同期読み込みで止まるので、非同期ロードの完了を待つ
        while (!model->IsLoadCompleted())
        {
            if (renderer.expired())
                co_return;

            co_await Coroutine::WaitYield();
        }

        if (const auto locked = renderer.lock())
            locked->SetMv1File(model);
    }
}

namespace GameCore::Npc::Enemy::Behaviour
{
    TickStatus Action::SwapModel::DoTick(const TickContext& context)
    {
        if (!target_ || !model_)
            return TickStatus::Failure;

        const auto renderer = target_->Components().Catch<ModelRenderer>();
        if (renderer.expired())
            return TickStatus::Failure;

        // NOTE: Sequence は毎Tick先頭からやり直すので Running は返さず、裏で差し替える。同期しない(序章の演出用)
        Coroutine::StartCoroutine(SwapAsync(renderer, model_.get()));
        return TickStatus::Success;
    }

    void Action::SwapModel::DoDrawGui()
    {
        ImGuiHelper::OnDrawInputField("target_", target_);
        ImGuiHelper::OnDrawInputField("model_", model_);
    }
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::SwapModel);
CEREAL_REGISTER_POLYMORPHIC_RELATION(GameCore::Npc::Enemy::Behaviour::ActionBase, GameCore::Npc::Enemy::Behaviour::Action::SwapModel);
#pragma endregion
