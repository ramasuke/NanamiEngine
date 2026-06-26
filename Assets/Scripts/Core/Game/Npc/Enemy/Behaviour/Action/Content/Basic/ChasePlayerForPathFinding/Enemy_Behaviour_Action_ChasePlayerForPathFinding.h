#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "vec3.hpp"
#include "../../../Enemy_Behaviour_ActionBase.h"
#include "../../../../../../../../../Editor/Npc/Enemy/Behaviour/Action/Enemy_Behaviour_ActionFactory.h"
#include "../../../../../../../../../../Data/HeightGridMap/Data_HeightGridMap.h"
#include "../../../../../../../../../../../Engine/Core/Object/Field/Field.h"
#include "cereal/types/base_class.hpp"
#include "cereal/types/polymorphic.hpp"

namespace GameCore::Npc::Enemy::Behaviour::Action
{
    /** HeightGridMap�̊i�q��񂩂�X���̓o�~�ۂ𔻒肵�AA* �o�H�T����ʃX���b�h�Ŏ��s���ăv���C���[�֌������A�N�V�����B
     * NOTE:
     * - 1�t���[����: �T���X���b�h���N�����邾���ňړ����� Failure ��Ԃ��B
     * - 2�t���[���ڈȍ~: �O�t���[���܂łɊ��������o�H�ňړ�����B
     *     �ړ��ł���� SetLinearVelocity �ňړ����� Success�A�ł��Ȃ���� Failure�B
     * �T���͊������邽�тɌ��݈ʒu���n�_�Ƃ��ĒǐՂ������B
     */
    class ChasePlayerForPathFinding final : public ActionBase
    {
    public:
        ChasePlayerForPathFinding() = default;
        ~ChasePlayerForPathFinding() override;

    private:
        TickStatus DoTick(const TickContext& context) override;
        void       DoDrawGui() override;

        // ���[�J�[�X���b�h�Ŏ��s�����o�H�T���{�́Bgrid �ƈʒu�݂̂��Q�Ƃ��鏃���Ȋ֐��B
        // �n�_�Z������ړI�n�܂Ői�ނׂ��Z�����ׂẴ��[���h���W��Ԃ��B
        static std::vector<glm::vec3> FindPath(
            const Asset::HeightGridMap& grid,
            const glm::vec3& start, const glm::vec3& goal,
            int maxCellRange, float maxClimbAngleDeg);

        static bool HasLineOfSight(
            const Asset::HeightGridMap& grid,
            int x0, int z0, int x1, int z1,
            float maxSlopeTan, float orthoDist);

        static constexpr int kDirX[8] = {  1, -1,  0,  0,  1,  1, -1, -1 };
        static constexpr int kDirZ[8] = {  0,  0,  1, -1,  1, -1,  1, -1 };

        [[serialize(0)]] FIELD(NanamiEngine::Module::Asset::HeightGridMap) heightGridMap_;
        [[serialize(0)]] float moveSpeed_           = 3.0f;  // �ړ����x
        [[serialize(0)]] float rotateSpeed_         = 360.0f;// �ړ������ւ̐���Y����]���x(�x/�b)
        [[serialize(0)]] int   maxPathCellRange_    = 16;    // �T������Z���̍ő�͈�
        [[serialize(0)]] float maxClimbAngleDeg_    = 45.0f; // �z������ő�X�Ίp(�x)
        [[serialize(0)]] float searchIntervalSec_   = 1.0f;  // �o�H�T���𑖂点��Ԋu(�b)
        [[serialize(0)]] int   animationNumber_     = -1;    // �ړ����ɍĐ�����A�j���[�V�����ԍ�(-1�Ŗ���)

        [[serialize(0)]] float rotateToleranceDeg_ = 5.0f;  // �ړ������̊p�x(�x)�ȉ��Ȃ��]���Ȃ�(���ʂȉ�]�h�~)

        std::atomic_bool       isSearching_{false}; // �T���X���b�h�����s����
        std::atomic_bool       isReady_{false};     // �V�����T�����ʂ𗘗p�\��
        bool                   hasPath_ = false;    // �ړ��Ɏg����o�H��ێ����Ă��邩
        float                  searchTimer_ = 0.0f; // ���̒T�����N������܂ł̎c�莞��(�b)
        std::vector<glm::vec3> cachedPath_;         // ���C���X���b�h���ړ��Ɏg���o�H
        std::vector<glm::vec3> resultPath_;         // ���[�J�[�X���b�h���i�[�����o�H
        std::mutex             mutex_;
        std::thread            pathThread_;

#pragma region Serialization Function
    public:
        template<class Archive>
        void save(Archive& archive, const std::uint32_t version) const
        {
            archive(cereal::base_class<ActionBase>(this));
            archive(CEREAL_NVP(heightGridMap_));
            archive(CEREAL_NVP(moveSpeed_));
            archive(CEREAL_NVP(rotateSpeed_));
            archive(CEREAL_NVP(maxPathCellRange_));
            archive(CEREAL_NVP(maxClimbAngleDeg_));
            archive(CEREAL_NVP(searchIntervalSec_));
            archive(CEREAL_NVP(animationNumber_));
            archive(CEREAL_NVP(rotateToleranceDeg_));
        }

        template<class Archive>
        void load(Archive& archive, const std::uint32_t version)
        {
            archive(cereal::base_class<ActionBase>(this));
            if (version >= 0) archive(CEREAL_NVP(heightGridMap_));
            if (version >= 0) archive(CEREAL_NVP(moveSpeed_));
            if (version >= 0) archive(CEREAL_NVP(rotateSpeed_));
            if (version >= 0) archive(CEREAL_NVP(maxPathCellRange_));
            if (version >= 0) archive(CEREAL_NVP(maxClimbAngleDeg_));
            if (version >= 2) archive(CEREAL_NVP(searchIntervalSec_));
            if (version >= 0) archive(CEREAL_NVP(animationNumber_));
            if (version >= 1) archive(CEREAL_NVP(rotateToleranceDeg_));
        }
#pragma endregion

    };

    REGISTER_ENEMY_ACTION_WITH_NAME(ChasePlayerForPathFinding, "Basic::ChasePlayerForPathFinding")
}

CEREAL_CLASS_VERSION(GameCore::Npc::Enemy::Behaviour::Action::ChasePlayerForPathFinding, 2)
CEREAL_REGISTER_TYPE(GameCore::Npc::Enemy::Behaviour::Action::ChasePlayerForPathFinding)
CEREAL_REGISTER_POLYMORPHIC_RELATION(
    GameCore::Npc::Enemy::Behaviour::ActionBase,
    GameCore::Npc::Enemy::Behaviour::Action::ChasePlayerForPathFinding
)
