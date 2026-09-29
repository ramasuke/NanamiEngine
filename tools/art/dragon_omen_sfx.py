"""序章でドラゴンが飛んで来るまでの「不穏」な音を合成し、SoundFile アセットとして入れる。

    python tools/art/dragon_omen_sfx.py [--only NAME ...] [--preview out.png]

鳴らすのは FirstEventDragon.enemyBehaviourData (tools/art/dragon_omen_bt.py が組む):
- DragonOmen_DistantRoar  … 姿が見える前、雲の向こうから聞こえる遠い咆哮
- DragonOmen_AlarmBell    … 島の見張りが打ち鳴らす警鐘 (教官の「鐘を鳴らせ！」と対)
- Omen_DragonApproach     … 飛来〜着地まで流す不穏な BGM (Assets/Audio/BGM、ループ)

音作りは ui_sfx.py の方針どおり低く写実的に (電子音・明るいチャイムにしない):
- 遠吠え: 同じ竜の声に聞こえるよう、既存の Dragon Roar.mp3 を読み (pip install miniaudio)、高域を落として
  こだまと長い屋外の残響に沈める。
- 警鐘: 見張り台の青銅の鐘を慌てて打つ。打つ間隔と強さを揺らし、遠くで鳴っている音にする。
- BGM: メロディーは無し。低いチェロのような持続音 + 半音でぶつかる弦のうねり + 遠い太鼓の鼓動 (2 秒に 1 回の
  二つ打ち) + 風。32 秒でぴったり繋がるように、周期的な部品だけで作り、フィルタと残響も輪にして掛ける。
  NOTE: mp3 は頭に数十 ms の無音が入るので、ループの継ぎ目は太鼓の一打で隠す (PULSE_OFFSET)。
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

SE_DIR = m.REPO / 'Assets/Audio/Enemy/FirstEventDragon'
BGM_DIR = m.REPO / 'Assets/Audio/BGM'


# ================================================================ 遠吠え
ROAR_SOURCE = m.REPO / 'Assets/Audio/Physics/Dragon Roar.mp3'


def load_mono(path):
    """mp3 を 48 kHz のモノラルで読む (pip install miniaudio)"""
    import miniaudio
    from scipy.signal import resample_poly
    # NOTE: decode_file は日本語を含むパスを開けないので、中身を読んで渡す
    decoded = miniaudio.decode(Path(path).read_bytes(), output_format=miniaudio.SampleFormat.FLOAT32)
    x = np.asarray(decoded.samples, dtype=float).reshape(-1, decoded.nchannels).mean(axis=1)
    if decoded.sample_rate != m.SR:
        g = np.gcd(m.SR, decoded.sample_rate)
        x = resample_poly(x, m.SR // g, decoded.sample_rate // g)
    return x


def distant_roar(rng):
    """同じ竜の咆哮 (Dragon Roar.mp3) を遠くで鳴ったように: 高域は空気に吸われ、雲と島に跳ね返った残響が長い"""
    voice = load_mono(ROAR_SOURCE)
    d = len(voice) / m.SR + 2.5
    far = m.hp(ui.muffle(m.norm(voice), 1100), 70)
    x = np.zeros(m.n_of(d))
    m.at(x, far, 0.0)
    # NOTE: 遠くの島の崖に返ってくる、さらにこもった二度目
    m.at(x, m.lp(far, 600), 0.42, 0.3)
    return ui.open_air(x, rng, rt60=3.0, mix=0.6, bright=800)


# ================================================================ 警鐘
def alarm_bell(rng):
    """見張り台の警鐘を慌てて打つ: 間隔と強さが揺れる十数打。遠くで鳴っているので丸く、残響が長い"""
    d = 10.0
    buf = np.zeros(m.n_of(d))
    t = 0.15
    hits = 0
    while t < 7.6:
        strength = 0.7 + 0.3 * rng.random()
        strike = ui.bronze_bell(3.0, rng, f=174.6, tau=1.8)
        clapper = ui.iron(0.12, rng, f=520, tau=0.03)
        m.at(buf, m.norm(strike) + 0.25 * m.pad(m.norm(clapper), len(strike)), t, strength)
        # NOTE: 打ち始めは速く、腕が疲れて少しずつ間が空く
        t += 0.46 + 0.05 * hits / 14 + 0.06 * rng.standard_normal()
        hits += 1
    x = ui.muffle(buf, 1500)
    return ui.open_air(x, rng, rt60=2.4, mix=0.45, bright=1100)


# ================================================================ 不穏 BGM
LOOP_SECS = 32.0
PULSE_SECS = 2.0
PULSE_OFFSET = 0.0


def loop_freq(f):
    """ループ長でちょうど整数周期になる周波数 (継ぎ目で位相が揃う)"""
    return round(f * LOOP_SECS) / LOOP_SECS


def circular(fn, x):
    """x を輪として fn (フィルタ・残響) を掛ける: 3 周並べて真ん中の 1 周を取る"""
    n = len(x)
    y = fn(np.tile(x, 3))
    return y[..., n:2 * n]


def loop_curve(points, kind='lin'):
    """1 周で元に戻る包絡 (最後の点は先頭と同じ値にしておく)"""
    return m.curve(LOOP_SECS, points, kind)


def cello_drone(rng):
    """低いチェロの持続音: D2 と少しずらした D2、下に D1 のうなり。弓の圧で明るさがゆっくり揺れる"""
    d = LOOP_SECS
    t = m.tt(d)
    vib = 1 + 0.0025 * np.sin(m.TAU * t / 4.0)
    x = (m.osc(loop_freq(73.42) * vib, d, 'saw', 24)
         + m.osc(loop_freq(73.67), d, 'saw', 24) * 0.8
         + m.osc(loop_freq(36.72), d, 'saw', 12) * 0.5)
    bow = loop_curve([(0, 260), (8, 420), (16, 300), (24, 520), (32, 260)], 'exp')
    body = circular(lambda s: m.sweep(s, np.tile(bow, 3), q=0.9, kind='lp'), x)
    rosin = circular(lambda s: m.bp(s, 700, 1800), m.white(d, rng)) * 0.04
    return m.norm(body) + rosin


def dissonant_strings(rng):
    """半音でぶつかる弦 (D3 / Eb3 / A3): 8 秒かけて膨らみ、しぼむ。2 周目の方が少し強い"""
    d = LOOP_SECS
    t = m.tt(d)
    x = np.zeros(m.n_of(d))
    for f, g in ((146.83, 1.0), (155.56, 0.8), (220.0, 0.45), (233.08, 0.3)):
        for detune in (-0.6, 0.0, 0.7):
            wob = 1 + 0.002 * np.sin(m.TAU * t / 8.0 + rng.random() * m.TAU)
            x += m.osc(loop_freq(f + detune) * wob, d, 'saw', 16) * g
    x = circular(lambda s: m.bp(s, 180, 1300), x)
    swell = loop_curve([(0, 0.05), (4, 0.05), (12, 0.8), (16, 0.15), (20, 0.05), (28, 1.0), (31, 0.2), (32, 0.05)])
    return m.norm(x) * swell


def heart_drum(rng):
    """遠くの大太鼓の鼓動: 2 秒ごとに「ドン、ドン」(2 打目は弱く)"""
    d = LOOP_SECS
    buf = np.zeros(m.n_of(d) + m.n_of(2.0))
    beat = 0
    t = PULSE_OFFSET
    while t < d:
        accent = 1.0 if beat % 4 == 0 else 0.8
        m.at(buf, m.norm(ui.drum(1.6, rng, f=46, tau=0.55)), t, accent)
        m.at(buf, m.norm(ui.drum(1.4, rng, f=44, tau=0.45)), t + 0.32, accent * 0.55)
        t += PULSE_SECS
        beat += 1
    # NOTE: 最後の打の残りは頭へ回して輪にする
    body = buf[:m.n_of(d)].copy()
    tail = buf[m.n_of(d):]
    body[:len(tail)] += tail
    return circular(lambda s: ui.muffle(s, 700), body)


def wind(rng):
    """島を渡る低い風: 帯域が 16 秒でうねる"""
    d = LOOP_SECS
    fc = loop_curve([(0, 220), (6, 480), (11, 260), (19, 560), (26, 240), (32, 220)], 'exp')
    x = circular(lambda s: m.sweep(s, np.tile(fc, 3), q=0.7), m.brown(d, rng) + 0.3 * m.white(d, rng))
    gust = loop_curve([(0, 0.4), (6, 1.0), (11, 0.5), (19, 1.0), (26, 0.4), (32, 0.4)])
    return m.norm(x) * gust


def omen_dragon_approach(rng):
    x = (cello_drone(rng) * 0.3
         + dissonant_strings(rng) * 0.4
         + m.norm(heart_drum(rng)) * 1.0
         + wind(rng) * 0.18)
    x = circular(lambda s: ui.muffle(s, 2400), m.sat(x, 1.2))
    return circular(lambda s: ui.open_air(s, rng, rt60=2.8, mix=0.35, bright=1400), x)


# ================================================================ 出力
SOUNDS = {
    'DragonOmen_DistantRoar': (distant_roar, SE_DIR, False),
    'DragonOmen_AlarmBell': (alarm_bell, SE_DIR, False),
    'Omen_DragonApproach': (omen_dragon_approach, BGM_DIR, True),
}

# NOTE: 遠くの音なので、出来事の SE (-14 dB) より控えめ。BGM は戦闘曲に切り替わったとき差が出るよう抑える
LOUDNESS = {
    'DragonOmen_DistantRoar': -17.0,
    'DragonOmen_AlarmBell': -18.0,
    'Omen_DragonApproach': -19.0,
}


def write_sound(name, stereo, out_dir):
    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / f'{name}.mp3'
    path.write_bytes(m.encode_mp3(stereo))
    meta = Path(str(path) + '.meta')
    if not meta.exists():
        content = str(path.relative_to(m.REPO)).replace('/', '\\').replace('\\', '\\\\')
        text = m.META.format(path=content, guid=str(uuid.uuid4()).upper()).replace('\n', '\r\n')
        meta.write_bytes(text.encode('utf-8'))
    return path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--only', action='append', choices=sorted(SOUNDS))
    ap.add_argument('--preview')
    args = ap.parse_args()
    rendered = {}
    for i, (name, (recipe, out_dir, loop)) in enumerate(SOUNDS.items()):
        if args.only and name not in args.only:
            continue
        stereo = recipe(np.random.default_rng(9300 + i))
        if loop:
            # NOTE: match_loudness は超低音を削るハイパスを掛けることがあり、継ぎ目が切れるので音量だけ合わせる
            stereo = stereo * 10 ** ((LOUDNESS[name] - ui.a_weighted_level(stereo)) / 20)
            stereo = ui.PEAK_CEILING * np.tanh(stereo / ui.PEAK_CEILING)
        else:
            stereo = m.trim_tail(stereo)
            stereo = ui.match_loudness(stereo, LOUDNESS[name], max_squash_db=4.0)
        rendered[name] = stereo
        path = write_sound(name, stereo, out_dir)
        mono = stereo.mean(axis=0)
        f = np.fft.rfftfreq(len(mono), 1 / m.SR)
        power = np.abs(np.fft.rfft(mono)) ** 2
        # 継ぎ目の段差を、ふだんの隣り合うサンプルの差 (最大) と比べる。0 dB 以下なら段差は目立たない
        steps = np.abs(np.diff(stereo, axis=1)).max()
        seam = 20 * np.log10(np.max(np.abs(stereo[:, :1] - stereo[:, -1:])) / steps + 1e-9)
        print(f'{name:24s} {stereo.shape[1] / m.SR:5.2f}s  {path.stat().st_size // 1024:4d} KB  '
              f'A {ui.a_weighted_level(stereo):6.1f} dB  peak {20 * np.log10(np.max(np.abs(stereo))):5.1f} dBFS  '
              f'centroid {np.sum(f * power) / np.sum(power):5.0f} Hz' + (f'  seam {seam:5.1f} dB' if loop else ''))
    if args.preview:
        m.render_preview(rendered, args.preview)


if __name__ == '__main__':
    main()
