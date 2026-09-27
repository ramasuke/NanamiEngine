"""敵の攻撃予兆 (tools/art/attack_warning_effect.py の閃光) に合わせて鳴らす効果音を合成し、
Assets/Audio/Enemy/AttackWarning に SoundFile アセットとして入れる。

    python tools/art/attack_warning_sfx.py [--only NAME ...] [--preview out.png]

PhysicsAttack / ChargeRush の warningSound_ に入れる (予兆エフェクトと同じ Tick で鳴る)。
- AttackWarning      : 金の予兆。刃を引き抜くような短い擦れ + 鉄の硬い響き
- AttackWarningHeavy : 赤の予兆。同じ構成を低く重くし、胸に響く打撃を足す

音作りは tools/art/ui_sfx.py の方針 (低く・写実寄り、明るいチャイムにしない) に合わせる。
既存の .mp3.meta は guid を保つ。
"""
from __future__ import annotations

import argparse
import sys
import uuid
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import magic_sfx as m  # noqa: E402
import ui_sfx as ui  # noqa: E402

OUT_DIR = m.REPO / 'Assets/Audio/Enemy/AttackWarning'


def scrape(d, rng, f0, f1, q=3.0):
    """刃が鞘や金具を擦って抜ける音: 帯域ノイズの中心を上へ掃引する"""
    x = m.white(d, rng)
    fc = m.curve(d, [(0, f0), (d, f1)], 'exp')
    return m.sweep(x, fc, q=q) * m.curve(d, [(0, 0), (d * 0.25, 1), (d, 0)])


def attack_warning(rng):
    d = 0.6
    x = ui.mix_at(d, [
        (scrape(0.11, rng, 700, 2200), 0.0, 0.55),
        (ui.iron(0.5, rng, f=330, tau=0.16), 0.07, 1.0),
        (ui.iron(0.4, rng, f=505, tau=0.09), 0.075, 0.35),
        (ui.knock(0.25, rng, f=110, tau=0.06), 0.07, 0.35),
    ])
    x = ui.muffle(x, 2600)
    return m.trim_tail(m.finish(ui.open_air(x, rng, rt60=0.7, mix=0.18, bright=1800), -3.0, fade_out=0.08))


def attack_warning_heavy(rng):
    d = 0.8
    x = ui.mix_at(d, [
        (scrape(0.14, rng, 450, 1500, q=2.5), 0.0, 0.6),
        (ui.iron(0.7, rng, f=220, tau=0.24), 0.09, 1.0),
        (ui.iron(0.5, rng, f=347, tau=0.12), 0.095, 0.4),
        (ui.drum(0.5, rng, f=55, tau=0.22), 0.09, 0.7),
    ])
    x = ui.muffle(x, 2000)
    return m.trim_tail(m.finish(ui.open_air(x, rng, rt60=0.9, mix=0.2, bright=1500), -3.0, fade_out=0.1))


SOUNDS = {
    'AttackWarning': (attack_warning, -16.0),
    'AttackWarningHeavy': (attack_warning_heavy, -14.0),
}


def make(name):
    fn, loudness = SOUNDS[name]
    rng = np.random.default_rng(sum(map(ord, name)))
    return ui.match_loudness(fn(rng), loudness)


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
    ap = argparse.ArgumentParser()
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--preview')
    args = ap.parse_args()
    rendered = {}
    for name in args.only or SOUNDS:
        stereo = make(name)
        rendered[name] = stereo
        path = write_sound(name, stereo)
        mono = stereo.mean(axis=0)
        f = np.fft.rfftfreq(len(mono), 1 / m.SR)
        power = np.abs(np.fft.rfft(mono)) ** 2
        print(f'{name:24s} {stereo.shape[1] / m.SR:5.2f}s  {path.stat().st_size // 1024:3d} KB  '
              f'A {ui.a_weighted_level(stereo):6.1f} dB  peak {20 * np.log10(np.max(np.abs(stereo))):5.1f} dBFS  '
              f'centroid {np.sum(f * power) / np.sum(power):5.0f} Hz')
    if args.preview:
        m.render_preview(rendered, args.preview)


if __name__ == '__main__':
    main()
