#pragma once
#include <string>

#include "cereal/types/string.hpp"
#include "vec2.hpp"
#include "Engine/Module/ScriptableObject/ScriptableObject.h"
#include "../../Scripts/Core/Game/Scene/Main/Type/MainSceneType.h"
#include "Libs/LibCore/cereal/glm/GlmHelper.h"

namespace NanamiEngine::Module::Asset
{
    constexpr auto LOADING_ROUTE_DATA_EXTENSION_LABEL = ".loadingRoute";

    /**
     * @brief ロード画面の航路図で、飛行船がどこからどこへ飛ぶか。
     *
     * 座標はロード画面の 1920x1080 の画面座標(カメラを動かす前)。
     * 航路は 3 次ベジェで、飛行船は弧長で進捗ぶん進む。
     * isHover_ のときは航路を引かず、hoverCenter_ の上空を楕円で回り続ける(行き先の島が地図に無い遷移用)
     */
    class LoadingRouteData final : public ScriptableObject
    {
    public:
        explicit LoadingRouteData(const std::string& contentPath = "");

        /** @brief from → to の遷移に使えるか。isFromAnywhere_ なら出発地を問わない */
        [[nodiscard]] bool Matches(GameCore::Scene::Main::SceneType from, bool hasFrom, GameCore::Scene::Main::SceneType to) const;
        /** @brief 出発地まで一致する航路を、どこからでも使える航路より優先するための点数 */
        [[nodiscard]] int MatchScore() const { return isFromAnywhere_ ? 1 : 2; }

        [[nodiscard]] GameCore::Scene::Main::SceneType ToScene() const { return toScene_; }
        [[nodiscard]] bool             IsHover        () const { return isHover_;        }
        [[nodiscard]] const glm::vec2& HoverCenter    () const { return hoverCenter_;    }
        [[nodiscard]] const glm::vec2& HoverRadius    () const { return hoverRadius_;    }
        [[nodiscard]] float            HoverLapSecs   () const { return hoverLapSecs_;   }
        [[nodiscard]] const glm::vec2& P0             () const { return p0_;             }
        [[nodiscard]] const glm::vec2& P1             () const { return p1_;             }
        [[nodiscard]] const glm::vec2& P2             () const { return p2_;             }
        [[nodiscard]] const glm::vec2& P3             () const { return p3_;             }
        [[nodiscard]] const std::string& KickerText   () const { return kickerText_;     }
        [[nodiscard]] const std::string& TitleText    () const { return titleText_;      }
        [[nodiscard]] const std::string& StatusText   () const { return statusText_;     }
        [[nodiscard]] const std::string& FromCaption  () const { return fromCaption_;    }
        [[nodiscard]] const glm::vec2&   FromCaptionPosition() const { return fromCaptionPosition_; }
        [[nodiscard]] const std::string& ToCaption    () const { return toCaption_;      }
        [[nodiscard]] const glm::vec2&   ToCaptionPosition  () const { return toCaptionPosition_;   }
        [[nodiscard]] bool             HasDestCircle  () const { return hasDestCircle_;  }
        [[nodiscard]] const glm::vec2& DestCirclePosition() const { return destCirclePosition_; }
        [[nodiscard]] float            DestCircleScale() const { return destCircleScale_; }
        [[nodiscard]] const glm::vec2& StampPosition  () const { return stampPosition_;  }
        [[nodiscard]] float            CloudDirection () const { return cloudDirection_; }
        [[nodiscard]] bool             HasNetworkStep () const { return hasNetworkStep_; }

    private:
        [[serialize(0)]] bool                             isFromAnywhere_ = true;
        [[serialize(0)]] GameCore::Scene::Main::SceneType fromScene_ = GameCore::Scene::Main::SceneType::MainIsland;
        [[serialize(0)]] GameCore::Scene::Main::SceneType toScene_   = GameCore::Scene::Main::SceneType::GrassLand;
        [[serialize(0)]] bool        isHover_      = false;
        [[serialize(0)]] glm::vec2   hoverCenter_  = glm::vec2(495.0f, 600.0f);
        [[serialize(0)]] glm::vec2   hoverRadius_  = glm::vec2(120.0f, 60.0f);
        [[serialize(0)]] float       hoverLapSecs_ = 7.0f;
        [[serialize(0)]] glm::vec2   p0_ = glm::vec2(600.0f, 592.0f);
        [[serialize(0)]] glm::vec2   p1_ = glm::vec2(810.0f, 300.0f);
        [[serialize(0)]] glm::vec2   p2_ = glm::vec2(1140.0f, 645.0f);
        [[serialize(0)]] glm::vec2   p3_ = glm::vec2(1290.0f, 412.0f);
        [[serialize(0)]] std::string kickerText_;
        [[serialize(0)]] std::string titleText_;
        [[serialize(0)]] std::string statusText_ = "航行中…";
        [[serialize(0)]] std::string fromCaption_;
        [[serialize(0)]] glm::vec2   fromCaptionPosition_ = glm::vec2(0.0f, 0.0f);
        [[serialize(0)]] std::string toCaption_;
        [[serialize(0)]] glm::vec2   toCaptionPosition_   = glm::vec2(0.0f, 0.0f);
        [[serialize(0)]] bool        hasDestCircle_ = true;
        [[serialize(0)]] glm::vec2   destCirclePosition_ = glm::vec2(1395.0f, 372.0f);
        [[serialize(0)]] float       destCircleScale_ = 1.0f;
        [[serialize(0)]] glm::vec2   stampPosition_ = glm::vec2(1272.0f, 300.0f);
        /** 雲の流れる向き。-1 で左へ、+1 で右へ(飛行船の進む向きの逆) */
        [[serialize(0)]] float       cloudDirection_ = -1.0f;
        /** マルチプレイの接続を挟むシーンか。ロード画面の進捗の配分が変わる */
        [[serialize(0)]] bool        hasNetworkStep_ = false;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ScriptableObject>(this));
            archive(CEREAL_NVP(isFromAnywhere_));
            archive(CEREAL_NVP(fromScene_));
            archive(CEREAL_NVP(toScene_));
            archive(CEREAL_NVP(isHover_));
            archive(CEREAL_NVP(hoverCenter_));
            archive(CEREAL_NVP(hoverRadius_));
            archive(CEREAL_NVP(hoverLapSecs_));
            archive(CEREAL_NVP(p0_));
            archive(CEREAL_NVP(p1_));
            archive(CEREAL_NVP(p2_));
            archive(CEREAL_NVP(p3_));
            archive(CEREAL_NVP(kickerText_));
            archive(CEREAL_NVP(titleText_));
            archive(CEREAL_NVP(statusText_));
            archive(CEREAL_NVP(fromCaption_));
            archive(CEREAL_NVP(fromCaptionPosition_));
            archive(CEREAL_NVP(toCaption_));
            archive(CEREAL_NVP(toCaptionPosition_));
            archive(CEREAL_NVP(hasDestCircle_));
            archive(CEREAL_NVP(destCirclePosition_));
            archive(CEREAL_NVP(destCircleScale_));
            archive(CEREAL_NVP(stampPosition_));
            archive(CEREAL_NVP(cloudDirection_));
            archive(CEREAL_NVP(hasNetworkStep_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ScriptableObject>(this));
            if (version >= 0) archive(CEREAL_NVP(isFromAnywhere_));
            if (version >= 0) archive(CEREAL_NVP(fromScene_));
            if (version >= 0) archive(CEREAL_NVP(toScene_));
            if (version >= 0) archive(CEREAL_NVP(isHover_));
            if (version >= 0) archive(CEREAL_NVP(hoverCenter_));
            if (version >= 0) archive(CEREAL_NVP(hoverRadius_));
            if (version >= 0) archive(CEREAL_NVP(hoverLapSecs_));
            if (version >= 0) archive(CEREAL_NVP(p0_));
            if (version >= 0) archive(CEREAL_NVP(p1_));
            if (version >= 0) archive(CEREAL_NVP(p2_));
            if (version >= 0) archive(CEREAL_NVP(p3_));
            if (version >= 0) archive(CEREAL_NVP(kickerText_));
            if (version >= 0) archive(CEREAL_NVP(titleText_));
            if (version >= 0) archive(CEREAL_NVP(statusText_));
            if (version >= 0) archive(CEREAL_NVP(fromCaption_));
            if (version >= 0) archive(CEREAL_NVP(fromCaptionPosition_));
            if (version >= 0) archive(CEREAL_NVP(toCaption_));
            if (version >= 0) archive(CEREAL_NVP(toCaptionPosition_));
            if (version >= 0) archive(CEREAL_NVP(hasDestCircle_));
            if (version >= 0) archive(CEREAL_NVP(destCirclePosition_));
            if (version >= 0) archive(CEREAL_NVP(destCircleScale_));
            if (version >= 0) archive(CEREAL_NVP(stampPosition_));
            if (version >= 0) archive(CEREAL_NVP(cloudDirection_));
            if (version >= 0) archive(CEREAL_NVP(hasNetworkStep_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Asset::LoadingRouteData, 0);
