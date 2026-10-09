#pragma once
#include <functional>
#include <memory>

#include "Engine/Core/Coroutine/Task/Task.h"
#include "Engine/Core/Object/Field/Field.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Component/ComponentBase.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"
#include "Prop_DepartShot.h"
#include "Prop_ReturnShot.h"

namespace GameCore
{
    class IPlayerAvatar;
}

namespace GamePlay::Prop
{
    // NOTE: 島の心臓の浮遊石。飛び去る/空から戻ってはまる演出を持つ
    class FloatingStone final : public Component::ComponentBase
    {
    public:
        // NOTE: 石と子のパーティクルを出す/隠す
        void SetVisible(bool isVisible);

        // NOTE: 石が震えて浮き上がり、空へ飛び去る。終わると石は隠れたまま
        Coroutine::Task<void> PlayDepartAsync(std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar);

        // NOTE: 石が空から飛んできて島の底にはまる。石のシーン上の位置がはまった位置
        // NOTE: canStart が true を返すまで演出を始めない。onDocked ははまった瞬間 (中断時はその場) に一度だけ呼ぶ
        Coroutine::Task<void> PlayReturnAsync(
            std::weak_ptr<GameCore::IPlayerAvatar> playerAvatar, std::function<bool()> canStart, std::function<void()> onDocked);

    private:
        [[nodiscard]] bool IsCanceled() const { return DestroyCancellationToken().IsCancellationRequested(); }

        // NOTE: LookAt で石を追うカメラ。シーンに置いた位置から動かない
        [[serialize(0)]] FIELD(CineMachine::CineMachineVirtualCamera) camera_;
        // NOTE: 飛んでいる石に重ねる光の尾
        [[serialize(0)]] FIELD(Asset::PrefabGameObjectFile) flightParticle_;
        // NOTE: ステージでは抜け出す瞬間、拠点の島でははまる瞬間に出す
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
