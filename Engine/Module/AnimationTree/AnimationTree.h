#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <optional>

#include "../../Core/Object/IObject.h"
#include "../Asset/Factory/AssetFactory.h"
#include "../../../Libs/LibCore/BlackBoard/Group/ParameterGroup.h"
#include "Node/EntryNode/AnimatorEntryNode.h"
#include "Node/VisualAnyStateNode/AnimationVisualAnyStateNode.h"
#include "NodePath/AnimationNodePath.h"

namespace NanamiEngine::Module::Gui::Graph
{
    class GraphEditorHost;
}

namespace NanamiEngine::Module::AnimationTree
{
    class AnimationTreeGraphDelegate;

    struct NANAMI_API AnimationStateSnapshot final
    {
        Guid  primaryGuid;
        float primaryDuringSecs  = 0.f;
        float primaryBlendRate   = 1.f;

        bool  isBlending          = false;
        Guid  secondaryGuid;
        float secondaryDuringSecs = 0.f;
        float secondaryBlendRate  = 0.f;

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(primaryGuid, primaryDuringSecs, primaryBlendRate, isBlending, secondaryGuid, secondaryDuringSecs, secondaryBlendRate);
        }
    };

    class NANAMI_API AnimationTree final : public Object::IObject
    {
    public:
        explicit AnimationTree(std::string filePath = "");
        [[nodiscard]] const Guid& GetGuid() const override { return guid_; }
        void OnSave();
        void OnUpdate(int modelHandle, float timeScale) const;
        void OnDrawGraphEditorGui(bool readOnly = false);
        void OnDrawGui() override;
        [[nodiscard]] BlackBoard::ParameterGroup& Param() const { return *additionConditionParameters_; }
        [[nodiscard]] const std::string& GetFilePath() const { return filePath_; }

        [[nodiscard]] const std::vector<std::shared_ptr<IAnimationNode>>& CurrentNodes() const { return currentNodes_; }

        // WARNING: 再生に使う前に必ず呼ぶ
        void InitForAnimator(int modelHandle);

        [[nodiscard]] AnimationStateSnapshot GetCurrentState() const;
        void ApplyRemoteState(const AnimationStateSnapshot& state, int modelHandle);

        // NOTE: 指定名クリップの再生進捗
        [[nodiscard]] std::optional<ClipProgress> GetClipProgress(const std::string& clipName) const;
        // NOTE: 現在再生中クリップの再生進捗
        [[nodiscard]] std::optional<ClipProgress> GetCurrentClipProgress() const;

    private:
        friend class AnimationTreeGraphDelegate;

        void AddCurrentNode    (const std::shared_ptr<IAnimationNode>& node);
        void AddCurrentNodePath(AnimationNodePath* nodePath, int modelHandle, float timeScale);
        void RemoveCurrentNode (const std::shared_ptr<IAnimationNode>& node, int modelHandle);

        [[nodiscard]] std::vector<std::shared_ptr<AnimationNodePath>> AllNodePaths() const;
        std::weak_ptr<IAnimationNode> FindNode(const Guid& guid);
        void CreateNode();
        
        Guid guid_;
        std::string filePath_;
        std::shared_ptr<AnimatorEntryNode> entryNode_ = std::make_shared<AnimatorEntryNode>();
        std::shared_ptr<AnimationVisualAnyStateNode> visualAnyStateNode_ = std::make_shared<AnimationVisualAnyStateNode>();
        std::unordered_map<Guid, std::shared_ptr<IAnimationNode>, GuidHash> nodes_;
        std::vector<std::shared_ptr<AnimationNodePath>> fromNodeNodePaths_;
        std::vector<std::shared_ptr<AnimationNodePath>> fromAnyStateNodeNodePaths_;
        std::vector<std::shared_ptr<IAnimationNode   >> currentNodes_;
        AnimationNodePath*                              currentNodePath_ = nullptr;
        std::shared_ptr<BlackBoard::ParameterGroup> additionConditionParameters_ = std::make_shared<BlackBoard::ParameterGroup>();

        std::shared_ptr<Gui::Graph::GraphEditorHost> graphHost_;
        std::shared_ptr<AnimationTreeGraphDelegate>  graphDelegate_;
    };
}

#pragma region SerializationMacro
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationTree, 0);
#pragma endregion
