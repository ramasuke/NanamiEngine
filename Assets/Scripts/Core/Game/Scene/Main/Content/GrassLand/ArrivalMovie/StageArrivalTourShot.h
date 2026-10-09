#pragma once
#include <cstdint>
#include <memory>
#include <string>

#include "cereal/types/string.hpp"
#include "Engine/Core/Object/Field/Field.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"
#include "Packages/Cinemachine/VirtualCamera/CineMachineVirtualCamera.h"

namespace GameCore::Scene::GrassLand
{
    // NOTE: 到着空撮で見どころを1か所映すショット。startCamera へ切ってから endCamera へ補間する
    struct StageArrivalTourShot
    {
        // NOTE: 見どころの名前。見出しなので全角スペースで字間を空ける (「村 の 跡」)
        std::string title;
        // NOTE: 名前の下に出す一言
        std::string subtitle;
        FIELD(NanamiEngine::CineMachine::CineMachineVirtualCamera) startCamera;
        FIELD(NanamiEngine::CineMachine::CineMachineVirtualCamera) endCamera;
        int duration_msecs = 4500;

        void Init();
        [[nodiscard]] bool HasCameras() const { return startCamera && endCamera; }
        [[nodiscard]] std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> StartCamera() const { return startCamera.get(); }
        [[nodiscard]] std::shared_ptr<NanamiEngine::CineMachine::CineMachineVirtualCamera> EndCamera  () const { return endCamera  .get(); }

        void OnDrawGui();

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(CEREAL_NVP(title));
            archive(CEREAL_NVP(subtitle));
            archive(CEREAL_NVP(startCamera));
            archive(CEREAL_NVP(endCamera));
            archive(CEREAL_NVP(duration_msecs));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(CEREAL_NVP(title));
            archive(CEREAL_NVP(subtitle));
            if (version == 0)
            {
                // NOTE: 旧版のワールド座標のカメラ位置。今はシーンに置いたカメラを使うので読み捨てる
                glm::vec3 cameraStart, cameraEnd, lookAt;
                archive(CEREAL_NVP(cameraStart));
                archive(CEREAL_NVP(cameraEnd));
                archive(CEREAL_NVP(lookAt));
            }
            if (version >= 1)
            {
                archive(CEREAL_NVP(startCamera));
                archive(CEREAL_NVP(endCamera));
            }
            archive(CEREAL_NVP(duration_msecs));
        }
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(GameCore::Scene::GrassLand::StageArrivalTourShot, 1);
#pragma endregion
