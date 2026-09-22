#pragma once
#include "../../../Core/Object/IObject.h"
#include "../cereal/include/cereal/cereal.hpp"

namespace NanamiEngine::Module::LifeCycleCallback
{
    class IUpdatable : public virtual Object::IObject
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
        // DiscardUpdatableBase が旧データのノードを読むためだけの器
        class DiscardedUpdatable final : public IUpdatable
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

    /**
     * @brief IUpdatableの継承をやめたクラスが、旧データに残るIUpdatableのノードを読み捨てる
     * @details cerealはクラスバージョンを型ごとに初出の1回しか書かないため、ノードを読まずに飛ばすと
     *          後から読む別コンポーネントのIUpdatableがバージョンを見つけられずに失敗する
     */
    template <class Archive>
    void DiscardUpdatableBase(Archive& archive)
    {
        Detail::DiscardedUpdatable discarded;
        archive(cereal::base_class<IUpdatable>(&discarded));
    }
}
CEREAL_CLASS_VERSION(NanamiEngine::Module::LifeCycleCallback::IUpdatable, 0);
