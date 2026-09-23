#include "CinemachineCameraBrain.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "../../../Engine/Core/Application/Configuration/DebugDraw/ApplicationConfiguration_DebugDraw.h"
#include "../../../Engine/Core/Application/Time/Time.h"
#include "../../../Engine/Core/Application/Window/Main/Game/GameWindow.h"
#include "../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../Engine/Module/Physics/Engine_Physics_Physics.h"
#include "../../../Engine/Module/Physics/Layer/Engine_Physics_PhysicsLayer.h"
#include "../../../Engine/Module/Serialization/Engine_Module_SerializationRegistration.h"

CineMachine::CinemachineCameraBrain* CineMachine::CinemachineCameraBrain::cameraBrain_ = nullptr;

CineMachine::CinemachineCameraBrain::~CinemachineCameraBrain()
{
    // NOTE: RemoveComponent は OnDestroy を呼ばないので、デストラクタでも解除する
    if (cameraBrain_ == this)
        cameraBrain_ = nullptr;
}

void CineMachine::CinemachineCameraBrain::OnAwake()
{
    
}

void CineMachine::CinemachineCameraBrain::OnStart()
{
    
}

void CineMachine::CinemachineCameraBrain::OnLateUpdate()
{
    // アクティブでないカメラも姿勢を更新しておく。切り替えた瞬間の補間の行き先が古い姿勢にならないように
    // virtualCameras_はシリアライズもされるため、同じカメラが重複していても1フレームに1回だけ回す
    std::vector<const CineMachineVirtualCamera*> updatedCameras;
    updatedCameras.reserve(virtualCameras_.size());
    for (const auto& virtualCamera : virtualCameras_)
    {
        const auto camera = virtualCamera.get();
        if (!camera || std::ranges::find(updatedCameras, camera.get()) != updatedCameras.end())
            continue;

        updatedCameras.push_back(camera.get());
        camera->UpdateBehaviours();
    }

    if (!currentVirtualCamera_)
        return;

    const glm::vec3 targetPos = currentVirtualCamera_->Transform().GetWorldPos();
    const glm::quat targetRot = currentVirtualCamera_->Transform().GetWorldRot();
    const float     targetFov = currentVirtualCamera_->Fov();

    if (!hasSmoothedPose_)
    {
        smoothedPos_ = Transform().GetWorldPos();
        smoothedRot_ = Transform().GetWorldRot();
        smoothedFov_ = targetFov;
        hasSmoothedPose_ = true;
    }

    const float dt = Time::DeltaTime();

    // アクティブなVirtualCameraが切り替わったら、その時点の姿勢から新しいカメラへ補間を始める。
    // 最初のカメラ(シーン開始時)は補間しない
    const CineMachineVirtualCamera* targetCamera = currentVirtualCamera_.get().get();
    if (targetCamera != blendTargetCamera_)
    {
        if (blendTargetCamera_ != nullptr && cameraBlendDuration_secs_ > 0.0f)
        {
            isBlending_   = true;
            blendElapsed_ = 0.0f;
            blendFromPos_ = smoothedPos_;
            blendFromRot_ = smoothedRot_;
            blendFromFov_ = smoothedFov_;
        }
        blendTargetCamera_ = targetCamera;
    }

    if (isBlending_)
    {
        blendElapsed_ += dt;
        const float rate = std::clamp(blendElapsed_ / cameraBlendDuration_secs_, 0.0f, 1.0f);
        const float t    = rate * rate * (3.0f - 2.0f * rate);
        // 行き先は毎フレームのtargetなので、移動中のカメラにも追従したまま合流する
        smoothedPos_ = glm::mix(blendFromPos_, targetPos, t);
        smoothedRot_ = glm::slerp(blendFromRot_, targetRot, t);
        smoothedFov_ = glm::mix(blendFromFov_, targetFov, t);
        if (rate >= 1.0f)
            isBlending_ = false;
    }
    else if (currentVirtualCamera_->WantsImmediateApply())
    {
        // lerp/slerpを完全にスキップしてVirtualCameraのTransformを即時適用する。
        smoothedPos_ = targetPos;
        smoothedRot_ = targetRot;
        smoothedFov_ = targetFov;
    }
    else
    {
        // 補完の開始点は揺れを含まないsmoothedPos_/smoothedRot_にする。
        smoothedPos_ = glm::mix(smoothedPos_, targetPos, 1.0f - std::exp(-positionLerpSpeed_secs_ * dt));
        smoothedRot_ = glm::slerp(smoothedRot_, targetRot, 1.0f - std::exp(-rotationSlerpSpeed_secs_ * dt));
        smoothedFov_ = glm::mix(smoothedFov_, targetFov, 1.0f - std::exp(-fovLerpSpeed_secs_ * dt));
    }
    Transform().SetWorldPos(smoothedPos_);
    Transform().SetWorldRot(smoothedRot_);

    // アクティブなVirtualCameraが切り替わったことをビヘイビアに通知する(NoiseCameraBehaviourのフェードイン等)。
    if (currentVirtualCamera_.get().get() != liveCamera_)
    {
        liveCamera_ = currentVirtualCamera_.get().get();
        currentVirtualCamera_->OnBecameLive();
    }

    // ShakeCameraBehaviourなど、補完後にオフセットを加えるビヘイビアのコールバック。
    currentVirtualCamera_->MainCameraCallback();

    // コールバック後の最終Transformでカメラをセットアップする。
    const glm::vec3 finalPos = Transform().GetWorldPos();
    const glm::quat finalRot = Transform().GetWorldRot();
    const glm::vec3 forward   = finalRot * glm::vec3(0, 0, 1);

    appliedFov_  = smoothedFov_;
    appliedNear_ = CalculateSafeNear(finalPos, finalRot);

    SetupCamera_Perspective(appliedFov_ * DX_PI_F / 180.0f);
    SetCameraNearFar(appliedNear_, cameraFar_);
    SetCameraPositionAndTarget_UpVecY(
        {finalPos.x, finalPos.y, finalPos.z},
        {finalPos.x + forward.x, finalPos.y + forward.y, finalPos.z + forward.z}
    );
}

float CineMachine::CinemachineCameraBrain::CalculateSafeNear(const glm::vec3& cameraPos, const glm::quat& cameraRot) const
{
    int screenWidth, screenHeight;
    GetScreenState(&screenWidth, &screenHeight, nullptr);
    if (screenHeight <= 0)
        return cameraNear_;

    const float aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    const float tanHalfFov  = std::tan(appliedFov_ * DX_PI_F / 180.0f * 0.5f);

    const glm::vec3 forward = cameraRot * glm::vec3(0, 0, 1);
    const glm::vec3 right   = cameraRot * glm::vec3(1, 0, 0) * (tanHalfFov * aspectRatio);
    const glm::vec3 up      = cameraRot * glm::vec3(0, 1, 0) * tanHalfFov;

    // near=1 のときのNear平面の中心と四隅
    const std::array<glm::vec3, 5> nearPlanePoints = {
        forward,
        forward + right + up,
        forward + right - up,
        forward - right + up,
        forward - right - up,
    };

    const Module::Physics::LayerMask mask = nearClipLayerMask_;

    // 球の重なり判定はメッシュの三角形を枝刈りできず重いため、最近傍で打ち切れるレイで調べる
    float safeNear = cameraNear_;
    for (const glm::vec3& point : nearPlanePoints)
    {
        const float pointDistanceScale = glm::length(point);
        const Module::Physics::RaycastHit hit = Module::Physics::Raycast(cameraPos, point, cameraNear_ * pointDistanceScale, mask);
        if (hit.Hit())
            safeNear = std::min(safeNear, hit.Distance() / pointDistanceScale * nearClipMargin_);
    }

    const float lowerNear = std::min(minCameraNear_, cameraNear_);
    return std::clamp(safeNear, lowerNear, cameraNear_);
}

void CineMachine::CinemachineCameraBrain::OnDestroy()
{
    // cameraBrain_はload()で無条件に上書きされるstaticなシングルトンポインタなので、
    // 破棄されても自動的にはクリアされない。ここでクリアしないと、このBrainを持たない
    // シーン(TitleScene等)に遷移した際にInstance()がダングリングポインタを返し、
    // 呼び出し側(ShakeCameraBehaviour等)がそれを解放済みメモリとして参照してクラッシュする。
    if (cameraBrain_ == this)
        cameraBrain_ = nullptr;
}

void CineMachine::CinemachineCameraBrain::OnDebugRender()
{
    if (currentVirtualCamera_)
    {
        if (!Core::Application::ApplicationBase::GameWindow()->IsPlayMode())
        {
            smoothedPos_ = currentVirtualCamera_->Transform().GetWorldPos();
            smoothedRot_ = currentVirtualCamera_->Transform().GetWorldRot();
            // OnUpdateが回らない編集モードでも、錐台表示が現在のFOVを映すようにする
            smoothedFov_ = currentVirtualCamera_->Fov();
            appliedFov_  = smoothedFov_;
            hasSmoothedPose_ = true;
            Transform().SetWorldPos(smoothedPos_);
            Transform().SetWorldRot(smoothedRot_);
        }
        else
        {
            Transform().SetWorldPos(Transform().GetWorldPos());
            Transform().SetWorldRot(Transform().GetWorldRot());
        }
    }

    if (Core::Application::Configuration::DebugDrawConfiguration::ShouldDrawMainCameraFrustum())
        OnDebugCameraFovRender();
}

void CineMachine::CinemachineCameraBrain::OnDebugCameraFovRender() const
{
    const glm::vec3 eye = Transform().GetWorldPos();
    const glm::quat rot = Transform().GetWorldRot();

    const glm::vec3 forward = rot * glm::vec3(0, 0, 1);
    const glm::vec3 up      = rot * glm::vec3(0, 1, 0);
    const glm::vec3 right   = rot * glm::vec3(1, 0, 0);

    int screenWidth, screenHeight;
    GetScreenState(&screenWidth, &screenHeight, nullptr);

    const float     fovRad      = appliedFov_ * DX_PI_F / 180.0f;
    constexpr float debugFar    = 80.0f;

    const float aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
    const float halfHeight  = tanf(fovRad * 0.5f) * debugFar;
    const float halfWidth   = halfHeight * aspectRatio;
    const glm::vec3 farCenter = eye + forward * debugFar;

    const glm::vec3 p1 = farCenter + up * halfHeight + right * halfWidth;
    const glm::vec3 p2 = farCenter + up * halfHeight - right * halfWidth;
    const glm::vec3 p3 = farCenter - up * halfHeight - right * halfWidth;
    const glm::vec3 p4 = farCenter - up * halfHeight + right * halfWidth;

    const int color = GetColor(255, 255, 0);

    DrawTriangle3D({eye.x, eye.y, eye.z}, {p1.x, p1.y, p1.z}, {p2.x, p2.y, p2.z}, color, false);
    DrawTriangle3D({eye.x, eye.y, eye.z}, {p2.x, p2.y, p2.z}, {p3.x, p3.y, p3.z}, color, false);
    DrawTriangle3D({eye.x, eye.y, eye.z}, {p3.x, p3.y, p3.z}, {p4.x, p4.y, p4.z}, color, false);
    DrawTriangle3D({eye.x, eye.y, eye.z}, {p4.x, p4.y, p4.z}, {p1.x, p1.y, p1.z}, color, false);
}

void CineMachine::CinemachineCameraBrain::OnDrawGui()
{
    if (currentVirtualCamera_)
    {
        ImGui::Text(("currentVirtualCameraPriority: " + std::to_string(currentVirtualCamera_->Priority().CurrentValue())).c_str());
    }
    else
    {
        ImGui::Text("currentVirtualCameraPriority: none");
    }
    ImGuiHelper::OnDrawInputField("currentVirtualCamera_", currentVirtualCamera_);

    ImGuiHelper::OnDrawInputField("virtualCameras_", virtualCameras_, [this]
    {
        if (ImGui::Button("Add"))
        {
            virtualCameras_.emplace_back();
        }
    });
    
    ImGuiHelper::OnDrawInputField("positionLerpSpeed_secs_"  , positionLerpSpeed_secs_   );
    ImGuiHelper::OnDrawInputField("rotationSlerpSpeed_secs_" , rotationSlerpSpeed_secs_  );
    ImGuiHelper::OnDrawInputField("fovLerpSpeed_secs_"       , fovLerpSpeed_secs_        );
    ImGuiHelper::OnDrawInputField("cameraBlendDuration_secs_", cameraBlendDuration_secs_ );
    ImGuiHelper::OnDrawInputField("fov_ (default)"           , fov_                      );
    ImGuiHelper::OnDrawInputField("cameraNear_"              , cameraNear_               );
    ImGuiHelper::OnDrawInputField("cameraFar_"               , cameraFar_                );
    ImGuiHelper::OnDrawInputField("minCameraNear_"           , minCameraNear_            );
    ImGuiHelper::OnDrawInputField("nearClipMargin_"          , nearClipMargin_           );
    Module::Physics::DrawLayerMaskGui("nearClipLayerMask_", nearClipLayerMask_);
    ImGui::Text(("appliedNear: " + std::to_string(appliedNear_)).c_str());
    ImGui::Text(("appliedFov: "  + std::to_string(appliedFov_)).c_str());
}

void CineMachine::CinemachineCameraBrain::ApplyVirtualCameraMatrix() const
{
    Transform().SetWorldMatrix(currentVirtualCamera_->Transform().GetWorldMatrix());
}

void CineMachine::CinemachineCameraBrain::ApplyVirtualCameraMatrix(
    const CineMachineVirtualCamera& virtualCamera) const
{
    Transform().SetWorldMatrix(virtualCamera.Transform().GetWorldMatrix());
}

void CineMachine::CinemachineCameraBrain::SnapToVirtualCamera(const CineMachineVirtualCamera& virtualCamera)
{
    // Transformだけ書き換えても、次のOnUpdateが残っているsmoothedPos_から補間し直して元へ戻すので、補間の起点ごと合わせる
    smoothedPos_ = virtualCamera.Transform().GetWorldPos();
    smoothedRot_ = virtualCamera.Transform().GetWorldRot();
    smoothedFov_ = virtualCamera.Fov();
    hasSmoothedPose_ = true;
    // スナップ先への切り替え補間は不要
    blendTargetCamera_ = &virtualCamera;
    isBlending_ = false;
    Transform().SetWorldPos(smoothedPos_);
    Transform().SetWorldRot(smoothedRot_);
}

void CineMachine::CinemachineCameraBrain::SubscribeVirtualCamera(const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera)
{
    const auto camera = virtualCamera.lock();
    if (cameraBrain_ == nullptr || !camera)
        return;

    cameraBrain_->virtualCameras_.emplace_back(virtualCamera);
    camera->Priority().Subscribe(
            [](int)
            {
                // NOTE: Brain が先に破棄された後で Priority が変わることがある
                if (cameraBrain_ == nullptr || cameraBrain_->virtualCameras_.empty())
                    return;

                const auto highestPriorityVirtualCamera
                    = *std::ranges::max_element(cameraBrain_->virtualCameras_,
                            [](auto& a, auto& b)
                            {
                                return a->Priority().CurrentValue() < b->Priority().CurrentValue();
                            });

                cameraBrain_->currentVirtualCamera_ = highestPriorityVirtualCamera;
            }).AddTo(camera.get());
}

void CineMachine::CinemachineCameraBrain::UnSubscribeVirtualCamera(
    const std::weak_ptr<CineMachineVirtualCamera>& virtualCamera)
{
    if (cameraBrain_ == nullptr)
        return;

    // assert(!cameraBrain_->virtualCameras_.empty() && "No virtual cameras registered!");

    //TODO: 明らかにバグです、修正必須。
    if (cameraBrain_->virtualCameras_.empty())
        return;
    
    std::erase_if(cameraBrain_->virtualCameras_,
        [&](const Core::Object::Field<CineMachineVirtualCamera>& camera)
        {
            return camera.get() == virtualCamera.lock();
        });

    if (cameraBrain_->virtualCameras_.empty())
        return;
    
    cameraBrain_->currentVirtualCamera_ =
        *std::ranges::max_element(
            cameraBrain_->virtualCameras_,
            [](auto& a, auto& b)
            {
                return a->Priority().CurrentValue()
                     < b->Priority().CurrentValue();
            });
}

#pragma region SerializationMacro
ENGINE_REGISTER_COMPONENT(NanamiEngine::CineMachine::CinemachineCameraBrain);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::ILateUpdatable, NanamiEngine::CineMachine::CinemachineCameraBrain);
#pragma endregion
