#include "PickupArea.h"

#include <algorithm>

#include "Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Core/Game/PlayerAvatar/IPlayerAvatar.h"
#include "../../../Core/Game/PlayerAvatar/AttackArea/PlayerAvatarAttackArea.h"
#include "../../../Core/Game/PlayerAvatar/Pickable/IPlayerPickable.h"
#include "../../../Core/Game/PlayerAvatar/Status/IPlayerAvatarStatus.h"

namespace GamePlay::PlayerAvatar
{
    void PickupArea::OnUpdate()
    {
        const auto avatarObject = Transform().GetParent();
        if (!avatarObject)
            return;

        const auto avatar = avatarObject->Components().Catch<GameCore::IPlayerAvatar>().lock();
        if (!avatar)
            return;

        // 他の画面のアバターが拾うと、この画面のプレイヤーの財布に入ってしまう
        if (!GamePlay::AttackArea<GameCore::PlayerAvatar::ITakablePlayerAttack>::IsDamageApplicableTarget(*avatarObject))
            return;

        auto& status = avatar->PlayerStatus();
        if (status.IsDeath())
            return;

        for (auto it = pickables_.begin(); it != pickables_.end();)
        {
            const auto pickable = it->lock();
            if (!pickable)
            {
                it = pickables_.erase(it);
                continue;
            }

            if (!pickable->CanPickUp(status))
            {
                ++it;
                continue;
            }

            it = pickables_.erase(it);
            pickable->OnPickUp(status);
        }
    }

    void PickupArea::OnTriggerEnter(
        const Physics::Manifold& contactManifold,
        const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        const auto pickable = gameObject->Components().Catch<IPlayerPickable>().lock();
        if (!pickable)
            return;

        const bool isKnown = std::ranges::any_of(pickables_, [&](const std::weak_ptr<IPlayerPickable>& w) { return w.lock() == pickable; });
        if (!isKnown)
            pickables_.push_back(pickable);
    }

    void PickupArea::OnTriggerExit(const std::shared_ptr<GameObject::IGameObject>& gameObject)
    {
        //TODO: ここ消せる、gameObjectがnullなのはonTriggerExitを呼び出す管理部分のengine側のバグ
        if (!gameObject)
            return;

        const auto leaving = gameObject->Components().Catch<IPlayerPickable>().lock();
        if (!leaving)
            return;

        std::erase_if(pickables_, [&](const std::weak_ptr<IPlayerPickable>& w)
        {
            const auto pickable = w.lock();
            return !pickable || pickable == leaving;
        });
    }

    void PickupArea::OnDrawGui()
    {
        ImGui::TextUnformatted("Pickup Area");
        ImGui::Text("Pickables: %d", static_cast<int>(pickables_.size()));
    }
}
