"""大砲の効果音を合成し、Assets/Audio/Physics に SoundFile アセットとして入れる。

    python tools/art/cannon_sfx.py

CannonBall_Explosion: 大砲の玉が爆発したとき (ExplosionParticle.prefab の SpawnSound)。
                      火薬の鋭い破裂音 + 低い衝撃 + 飛び散る破片。魔法の爆発 (ExplosionBlast_Burst) より短く乾いた音
波形の部品・mp3 エンコーダ・.meta のひな形は tools/art/magic_sfx.py のものを使うので、形式は他の SE と揃う
(MPEG-1 Layer III, 192 kbps, 48 kHz, stereo。volume_ 255、音量は波形のピークで決める)。
"""
from __future__ import annotations

import sys
import uuid
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import magic_sfx as m  # noqa: E402

OUT_DIR = m.REPO / 'Assets/Audio/Physics'


def cannon_ball_explosion(rng):
    d = 1.4
    x = m._explosion(d, rng, boom_f=(75, 34), boom_tau=0.28, noise_tau=0.18, lp_start=7000, lp_end=260, debris=110)
    # 火薬の破裂の立ち上がり
    crack = m.hp(m.white(d, rng), 1500) * m.exp_decay(d, 0.012, 0.0005)
    # 鉄片と木片が散らばる
    shrapnel = m.pings(d, rng, 6, 1800, 4200, 0.035, 0.04, 0.5, (0.2, 0.6))
    x = m.sat(x + m.norm(crack) * 0.55 + shrapnel * 0.25, 1.4)
    return m.finish(m.reverb(x, rng, 1.2, 0.28, predelay=0.02, bright=4500), -1, fade_out=0.25)


SOUNDS = {
    'CannonBall_Explosion': cannon_ball_explosion,
}


def write_sound(name, stereo):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / f'{name}.mp3'
    path.write_bytes(m.encode_mp3(stereo))
    meta = Path(str(path) + '.meta')
    if not meta.exists():
        content = str(path.relative_to(m.REPO)).replace('/', '\\').replace('\\', '\\\\')
        text = m.META.format(path=content, guid=str(uuid.uuid4()).upper()).replace('\n', '\r\n')
        meta.write_bytes(text.encode('utf-8'))
    return path


def main():
    for i, (name, recipe) in enumerate(SOUNDS.items()):
        stereo = m.trim_tail(recipe(np.random.default_rng(3000 + i)))
        path = write_sound(name, stereo)
        print(f'{name:22s} {stereo.shape[1] / m.SR:5.2f}s  {path.stat().st_size // 1024:3d} KB')


if __name__ == '__main__':
    main()
