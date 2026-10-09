#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../Namespace/EngineNamespace.h"
#include "vec2.hpp"
#include <string>

#include "../../../Core/Object/IObject.h"
#include "../../../../Packages/R4/R4.h"

namespace NanamiEngine::Module::AnimationTree
{
    constexpr auto NODE_SIZE = ImVec2(120.0f, 60.0f);

    // NOTE: 特定クリップの再生進捗
    struct NANAMI_API ClipProgress final
    {
        float duringSecs    = 0.0f; // 再生経過秒
        float durationSecs   = 0.0f; // クリップ全長（秒）
        float normalizedTime = 0.0f; // duringSecs / durationSecs を [0,1] にクランプした値
    };

    class NANAMI_API IAnimationNode : public virtual Object::IObject
    {
    public:
        struct NANAMI_API UpdateCallbackContext
        {
            UpdateCallbackContext(const float duringSecs, const float duringDeltaTimeSecs, const float timeScale)
                : during_secs_(duringSecs),
                  duringDeltaTime_secs_(duringDeltaTimeSecs),
                  timeScale_(timeScale)
            {
            }

            const float during_secs_;
            const float duringDeltaTime_secs_;
            const float timeScale_;
        };
        
        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
        }

        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
        }
        
        virtual ~IAnimationNode() override = default;
        virtual void  InitForGamePlay    (int modelHandle   ) = 0;
        
        virtual void  OnUpdateBlendRate(float blendRate   ) = 0;
        virtual void  OnUpdateAnimation(int modelHandle, float timeScale) = 0;
        virtual void  OnExitNode       (int modelHandle   ) = 0;
        
        virtual R4::Observable<UpdateCallbackContext> OnUpdated() = 0;
        [[nodiscard]] virtual glm::vec2 Position()         const = 0;
        // NOTE: グラフエディタ上の位置（ノード左上）。表示専用でゲームプレイには影響しない
        virtual void SetPosition(const glm::vec2& position) = 0;
        [[nodiscard]] virtual float GetAnimDuration_secs() const = 0;

        // NOTE: グラフエディタのノード見出しに出す名前
        [[nodiscard]] virtual std::string GraphNodeName() const = 0;
        // NOTE: グラフエディタのノード本文に出す補足（1 行）。空なら何も出さない
        [[nodiscard]] virtual std::string GraphNodeDetail() const { return {}; }
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::IAnimationNode, 0);