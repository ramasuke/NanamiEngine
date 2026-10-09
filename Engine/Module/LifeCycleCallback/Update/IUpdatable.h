#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include "../../../Core/Object/IObject.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class NANAMI_API IUpdatable : public virtual Object::IObject
    {
    public:
        virtual ~IUpdatable() = default;
        virtual void OnUpdate() = 0;

        template <class Archive>
        void save(Archive& archive, const std::uint32_t version) const { }
        template <class Archive>
        void load(Archive& archive, const std::uint32_t version)       { }
    };

    namespace Detail
    {
        // NOTE: 旧データに残る IUpdatable のノードを読み捨てるためだけの器
        class NANAMI_API DiscardedUpdatable final : public IUpdatable
        {
        public:
            void OnUpdate() override { }
            void OnDrawGui() override { }
            [[nodiscard]] const Guid& GetGuid() const override
            {
                static const Guid guid;
                return guid;
            }
        };
    }

    // NOTE: IUpdatableの継承をやめたクラスが、旧データに残るIUpdatableのノードを読み捨てる
    // NOTE: 飛ばすと初出だけに書かれたクラスバージョンが失われ、後続の読み込みが失敗する
    template <class Archive>
    void DiscardUpdatableBase(Archive& archive)
    {
        Detail::DiscardedUpdatable discarded;
        archive(cereal::base_class<IUpdatable>(&discarded));
    }
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, 0);
