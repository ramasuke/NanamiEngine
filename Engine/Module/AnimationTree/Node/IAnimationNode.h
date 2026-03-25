#pragma once
#include "../../Namespace/EngineNamespace.h"
#include "vec2.hpp"
#include "../../../Core/Object/IObject.h"
#include "../../Gui/Graph/NodeDrawResult/NodeDrawResult.h"
#include "../rxcpp/rx.hpp"

namespace NanamiEngine::Module::AnimationTree
{
    constexpr auto NODE_SIZE = ImVec2(120.0f, 60.0f);
    ///TODO: InterfaceをVisual可能AnimationNodeとLogicAnimationNodeで分離した方が良い(インターフェースの定義が要件に見合っていないため不適切)
    ///TODO: AnimationNodeBaseクラスを作成した方が共通化可能
    class IAnimationNode : public virtual Object::IObject
    {
    public:
        struct UpdateCallbackContext
        {
            UpdateCallbackContext(const float duringSecs, const float duringDeltaTimeSecs)
                : during_secs_(duringSecs),
                  duringDeltaTime_secs_(duringDeltaTimeSecs)
            {
            }

            const float during_secs_;
            const float duringDeltaTime_secs_;
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
        virtual void  OnUpdateAnimation(int modelHandle   ) = 0;
        virtual void  OnExitNode       (int modelHandle   ) = 0;
        
        virtual rxcpp::observable<UpdateCallbackContext> OnUpdated() = 0;
        [[nodiscard]] virtual glm::vec2 Position()         const = 0;
        [[nodiscard]] virtual float GetAnimDuration_secs() const = 0;
        ///TODO: AnimationNodeBaseの場合はこちらの処理が共通か可能
        virtual Gui::Graph::NodeDrawResult OnDrawGraphEditorGui(const ImVec2& offset, ImDrawList* drawList, std::weak_ptr<IAnimationNode> ownPtr) = 0;
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::IAnimationNode, 0);