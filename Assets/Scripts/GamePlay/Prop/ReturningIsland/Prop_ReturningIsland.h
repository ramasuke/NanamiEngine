#pragma once
#include <functional>
#include <memory>

#include "Engine/Core/Coroutine/Task/Task.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "../FloatingStone/Prop_FloatingStoneShot.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Prop
{
    /**
     * @brief 浮遊石の力で雲の下から戻ってくる島と、そこへ上る階段。シーン上の位置が戻った位置。
     *        戻るまでは Sink で雲の下へ退避させておく。演出はシーンが抜けたら (この島が破棄されたら) 止まる
     */
    class ReturningIsland final : public Component::ComponentBase
    {
    public:
        /** @brief 戻る前の島と階段を隠し、コライダーごと雲の下へ退避させる。シーンに入ったときに一度だけ呼ぶ */
        void Sink();

        /** @brief 戻った島と階段を出す(シーンでは隠してある) */
        void Show();

        /**
         * @brief Sink で退避させた島が雲の下からせり上がり、階段が手前から1段ずつ架かる
         * @param canStart 演出を始めてよいか。ロード画面が明けるまで false を返す
         * @param onReturned 戻りきった瞬間 (スキップ・シーンを抜けたときはその場) に一度だけ呼ぶ
         */
        Coroutine::Task<void> PlayReturnAsync(
            std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar, std::function<bool()> canStart, std::function<void()> onReturned);

    private:
        [[nodiscard]] bool IsCanceled() const { return DestroyCancellationToken().IsCancellationRequested(); }

        /** 子が1段ずつの足場。子の並び順に架かる */
        [[serialize(0)]] FIELD(GameObject::IGameObject) stairs_;
        /** LookAt で島を追うカメラ。シーンに置いた位置から動かない */
        [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera) camera_;
        /** カメラが見る所。島の子(一緒に上がってくる物)にする */
        [[serialize(0)]] FIELD(GameObject::IGameObject) focus_;
        [[serialize(0)]] IslandReturnShot shot_;

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
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(stairs_));
            if (version >= 0) archive(CEREAL_NVP(camera_));
            if (version >= 0) archive(CEREAL_NVP(focus_));
            if (version >= 0) archive(CEREAL_NVP(shot_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::ReturningIsland, 0);
