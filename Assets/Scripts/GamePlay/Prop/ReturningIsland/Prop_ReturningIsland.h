#pragma once
#include <functional>
#include <memory>

#include "Engine/Core/Coroutine/Task/Task.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../FloatingStone/Prop_IslandReturnShot.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Prop
{
    // NOTE: 雲の下から戻ってくる島と階段。シーン上の位置が戻った位置
    class ReturningIsland final : public Component::ComponentBase
    {
    public:
        // NOTE: 戻る前の島と階段を隠し、コライダーごと雲の下へ退避させる。シーンに入ったときに一度だけ呼ぶ
        void Sink();

        // NOTE: 戻った島と階段を出し、橋の出口をふさぐ壁をどける(シーンでは島と階段を隠してある)
        void Show();

        // NOTE: Sink で退避させた島が雲の下からせり上がり、階段が手前から1段ずつ架かる
        // NOTE: canStart が true を返すまで演出を始めない。onReturned は戻りきった瞬間 (中断時はその場) に一度だけ呼ぶ
        Coroutine::Task<void> PlayReturnAsync(
            std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar, std::function<bool()> canStart, std::function<void()> onReturned);

    private:
        [[nodiscard]] bool IsCanceled() const { return DestroyCancellationToken().IsCancellationRequested(); }

        // NOTE: 子が1段ずつの足場。子の並び順に架かる
        [[serialize(0)]] FIELD(GameObject::IGameObject) stairs_;
        // NOTE: LookAt で島を追うカメラ。シーンに置いた位置から動かない
        [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera) camera_;
        // NOTE: カメラが見る所。島の子(一緒に上がってくる物)にする
        [[serialize(0)]] FIELD(GameObject::IGameObject) focus_;
        [[serialize(0)]] IslandReturnShot shot_;
        // NOTE: 戻るまで島の縁の橋の出口をふさぐ壁。戻ったら雲の下へどける
        [[serialize(1)]] FIELD(GameObject::IGameObject) blockers_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(stairs_));
            archive(CEREAL_NVP(camera_));
            archive(CEREAL_NVP(focus_));
            archive(CEREAL_NVP(shot_));
            archive(CEREAL_NVP(blockers_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(stairs_));
            if (version >= 0) archive(CEREAL_NVP(camera_));
            if (version >= 0) archive(CEREAL_NVP(focus_));
            if (version >= 0) archive(CEREAL_NVP(shot_));
            if (version >= 1) archive(CEREAL_NVP(blockers_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::ReturningIsland, 1);
