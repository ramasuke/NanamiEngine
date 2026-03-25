#pragma once
#include "cereal/cereal.hpp"

namespace NanamiEngine::Module::AnimationTree
{
    class AnimationSyncBase
    {
    public:
        virtual ~AnimationSyncBase() = default;
        void Init(const int& animationModelHandle);
        virtual void UpdateSync(const int& animationModelHandle) = 0;
        virtual void DoDrawGui() = 0;

    protected:
        /** サンドボックスパターン */
        [[nodiscard]] const int& BoneIndex() const { return boneIndex_; }
        
    private:
        [[serialize(0)]] std::string boneName_;
        int boneIndex_ = -1;
        
#pragma region Serialization Function
    public:
        void OnDrawGui();
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const {
            archive(CEREAL_NVP(boneName_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version) {
            archive(CEREAL_NVP(boneName_));
        }
#pragma endregion
    };
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::AnimationTree::AnimationSyncBase, 0)
