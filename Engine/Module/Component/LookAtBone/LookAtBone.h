#pragma once
#include <optional>
#include <string>
#include <vector>

#include "vec3.hpp"
#include "../ComponentBase.h"
#include "../Animator/IAnimationPoseModifier.h"
#include "../cereal/include/cereal/types/string.hpp"
#include "../cereal/include/cereal/types/vector.hpp"

namespace NanamiEngine::Module::Component
{
    /**
     * @brief Animator がアニメーションを適用した後に首・頭のボーンを回し、ターゲットの方を見させる
     * @note  bones_ は親から子の順に並べる。回転量は weight の比で各ボーンに配分する
     */
    class LookAtBone final : public ComponentBase,
                             public IAnimationPoseModifier
    {
    public:
        struct BoneWeight
        {
            std::string boneName;
            float       weight = 0.5f;

            template<class Archive>
            void serialize(Archive& archive)
            {
                archive(CEREAL_NVP(boneName));
                archive(CEREAL_NVP(weight));
            }
        };

        /** @brief 見るワールド座標。nullopt で正面に戻る */
        void SetTarget(const std::optional<glm::vec3>& worldPos) { target_ = worldPos; }
        /** @brief 足元から最後のボーン(頭)までの高さ。一度も姿勢を計算していなければ nullopt */
        [[nodiscard]] std::optional<float> HeadHeight() const { return headHeight_; }

    private:
        void OnModifyPose(int modelHandle) override;
        void ResolveBoneIndices(int modelHandle);
        void ResetUserMatrices(int modelHandle) const;
        void UpdateAngles(const glm::vec3& headWorldPos);

        [[serialize(0)]] std::vector<BoneWeight> bones_ = { { "mixamorig:Neck", 0.4f }, { "mixamorig:Head", 0.6f } };
        [[serialize(0)]] float maxYawDeg_       = 70.0f;
        [[serialize(0)]] float maxPitchDeg_     = 25.0f;
        // これより後ろにいる相手は見ようとせず正面に戻る
        [[serialize(0)]] float giveUpYawDeg_    = 110.0f;
        [[serialize(0)]] float followSharpness_ = 6.0f;

        std::optional<glm::vec3> target_;
        std::optional<float>     headHeight_;
        float yaw_   = 0.0f;
        float pitch_ = 0.0f;
        std::vector<int> boneIndices_;
        int boneIndicesModelHandle_ = -1;
        bool boneIndicesDirty_ = true;

#pragma region Serialization Function
    public:
        void OnDrawGui() override;

        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(bones_));
            archive(CEREAL_NVP(maxYawDeg_));
            archive(CEREAL_NVP(maxPitchDeg_));
            archive(CEREAL_NVP(giveUpYawDeg_));
            archive(CEREAL_NVP(followSharpness_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ComponentBase>(this));
            archive(CEREAL_NVP(bones_));
            archive(CEREAL_NVP(maxYawDeg_));
            archive(CEREAL_NVP(maxPitchDeg_));
            archive(CEREAL_NVP(giveUpYawDeg_));
            archive(CEREAL_NVP(followSharpness_));
        }
#pragma endregion
    };
}

CEREAL_CLASS_VERSION(NanamiEngine::Module::Component::LookAtBone, 0);
