#pragma once
#include <functional>
#include <memory>

#include "Engine/Core/Coroutine/Task/Task.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "Prop_FloatingStoneShot.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Prop
{
    /**
     * @brief 島の心臓の浮遊石(子にオーラの ParticleSystem)。ステージでは飛び去り、拠点の島では空から戻ってはまる。
     *        演出はシーンが抜けたら (この石が破棄されたら) 止まる
     */
    class FloatingStone final : public Component::ComponentBase
    {
    public:
        /** @brief 石と子のパーティクルを出す/隠す */
        void SetVisible(bool isVisible);

        /** @brief 石が震えて浮き上がり、空へ飛び去る。終わると石は隠れたまま */
        Coroutine::Task<void> PlayDepartAsync(std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar);

        /**
         * @brief 石が空から飛んできて島の底にはまる。石のシーン上の位置がはまった位置
         * @param canStart 演出を始めてよいか。ロード画面が明けるまで false を返す (その裏で終わってしまうため)
         * @param onDocked はまった瞬間 (スキップ・シーンを抜けたときはその場) に一度だけ呼ぶ
         */
        Coroutine::Task<void> PlayReturnAsync(
            std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar, std::function<bool()> canStart, std::function<void()> onDocked);

    private:
        [[nodiscard]] bool IsCanceled() const { return DestroyCancellationToken().IsCancellationRequested(); }

        /** LookAt で石を追うカメラ。シーンに置いた位置から動かない */
        [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera) camera_;
        /** 飛んでいる石に重ねる光の尾 */
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) flightParticle_;
        /** ステージでは抜け出す瞬間、拠点の島でははまる瞬間に出す */
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) burstParticle_;
        [[serialize(0)]] DepartShot departShot_;
        [[serialize(0)]] ReturnShot returnShot_;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(camera_));
            archive(CEREAL_NVP(flightParticle_));
            archive(CEREAL_NVP(burstParticle_));
            archive(CEREAL_NVP(departShot_));
            archive(CEREAL_NVP(returnShot_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            if (version >= 0) archive(CEREAL_NVP(camera_));
            if (version >= 0) archive(CEREAL_NVP(flightParticle_));
            if (version >= 0) archive(CEREAL_NVP(burstParticle_));
            if (version >= 0) archive(CEREAL_NVP(departShot_));
            if (version >= 0) archive(CEREAL_NVP(returnShot_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(GamePlay::Prop::FloatingStone, 0);
