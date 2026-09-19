#pragma once
#include <memory>
#include <string>

#include "MagicCastMotion.h"
#include "MagicCastTarget.h"
#include "../../../../../Engine/Module/Guid/Guid.h"
#include "../../../../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::Asset
{
    class SpriteFile;
    class SoundFile;
    class PrefabGameObjectFile;
}

namespace GameCore::Magic
{
    class IMagicCaster;

    // 魔法1つ分。ステート・ステータス・HUD はこれだけを見て、中で何が起きるかは知らない
    class IMagicSpell
    {
    public:
        virtual ~IMagicSpell() = default;
        [[nodiscard]] virtual const std::string&                 DisplayName           () const = 0;
        [[nodiscard]] virtual std::shared_ptr<Asset::SpriteFile> IconSprite            () const = 0;
        [[nodiscard]] virtual std::shared_ptr<Asset::SoundFile>  CastSound             () const = 0;
        [[nodiscard]] virtual float                              ManaCost              () const = 0;
        [[nodiscard]] virtual float                              Cooldown_secs         () const = 0;
        /** @brief Cast State に入ってから撃つまでの秒数 */
        [[nodiscard]] virtual float                              CastFireTime_secs     () const = 0;
        /** @brief Cast State 全体の長さ */
        [[nodiscard]] virtual float                              CastTotalDuration_secs() const = 0;
        [[nodiscard]] virtual MagicCastMotion                    CastMotion            () const = 0;
        /** @brief Cast State の頭から撃ち手の足元に出す演出。モーションの手の動きに合わせて作ってある。無ければ nullptr */
        [[nodiscard]] virtual std::shared_ptr<Asset::PrefabGameObjectFile> CastEffectPrefab() const = 0;
        /** @brief RPC で魔法を指す識別子 */
        [[nodiscard]] virtual const Guid&                        SpellGuid             () const = 0;
        [[nodiscard]] virtual MagicCastTarget Aim(const IMagicCaster& caster) const = 0;
        virtual void Execute(const IMagicCaster& caster, const MagicCastTarget& target) const = 0;
    };
}
