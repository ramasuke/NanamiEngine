"""Synthesize the MagicCaster spell sound effects and install them as SoundFile assets under Assets/Audio/Magic.

    python tools/art/magic_sfx.py [--only NAME ...] [--preview PATH]

Per spell: <Spell>_Charge (played by the Cast_<Motion> prefab from the start of the cast, as long as the wind-up),
<Spell>_Release (MagicSpellData.castSound_, at the cast point on every client) and whatever the spell leaves behind
(_Impact / _Sigil / _Burst / _Rise / _Crumble / _Hit, played by those effect prefabs through GamePlay::Sound::SpawnSound).
Everything is generated here (numpy + scipy, mp3 via lameenc: MPEG-1 Layer III, 192 kbps, 48 kHz, stereo like the
other SE), so there is no licence to track. Deterministic: every sound has its own seed. An existing .mp3.meta keeps
its guid; volume_ is 255 (0 would be silent) and loudness is set by the waveform peak instead.
"""
from __future__ import annotations

import argparse
import math
import sys
import uuid
from pathlib import Path

import lameenc
import numpy as np
from scipy.signal import butter, fftconvolve, lfilter, sosfilt

REPO = Path(__file__).resolve().parents[2]
OUT_DIR = REPO / 'Assets/Audio/Magic'
SR = 48000
TAU = 2 * math.pi


# ================================================================ primitives
def n_of(secs):
    return int(round(secs * SR))


def tt(secs):
    return np.arange(n_of(secs)) / SR


def pad(x, n):
    return x[:n] if len(x) >= n else np.concatenate([x, np.zeros(n - len(x))])


def at(buf, x, start_secs, gain=1.0):
    """Mix x into buf starting at start_secs (in place, clipped to buf)."""
    i = n_of(start_secs)
    if i >= len(buf):
        return buf
    m = min(len(x), len(buf) - i)
    buf[i:i + m] += x[:m] * gain
    return buf


def curve(secs, points, kind='lin'):
    """Piecewise curve over [0, secs]: points = [(t, v), ...]; kind 'exp' interpolates in log space."""
    t = tt(secs)
    xs = [p[0] for p in points]
    vs = [p[1] for p in points]
    if kind == 'exp':
        return np.exp(np.interp(t, xs, np.log(np.maximum(vs, 1e-6))))
    return np.interp(t, xs, vs)


def exp_decay(secs, tau, attack=0.002):
    t = tt(secs)
    return np.minimum(1.0, t / max(attack, 1e-4)) * np.exp(-t / tau)


def white(secs, rng):
    return rng.standard_normal(n_of(secs))


def brown(secs, rng):
    x = np.cumsum(rng.standard_normal(n_of(secs)))
    x = hp(x, 25)
    return x / (np.max(np.abs(x)) + 1e-9)


def _sos(kind, f, order=2):
    if kind == 'bp':
        lo, hi = f
        return butter(order, [max(lo, 20), min(hi, SR * 0.45)], 'bandpass', fs=SR, output='sos')
    return butter(order, min(max(f, 20), SR * 0.45), 'low' if kind == 'lp' else 'high', fs=SR, output='sos')


def lp(x, f, order=2):
    return sosfilt(_sos('lp', f, order), x)


def hp(x, f, order=2):
    return sosfilt(_sos('hp', f, order), x)


def bp(x, lo, hi, order=2):
    return sosfilt(_sos('bp', (lo, hi), order), x)


def _biquad(kind, fc, q):
    w0 = TAU * min(max(fc, 20.0), SR * 0.45) / SR
    alpha = math.sin(w0) / (2 * q)
    c = math.cos(w0)
    if kind == 'bp':
        b = [alpha, 0.0, -alpha]
    elif kind == 'lp':
        b = [(1 - c) / 2, 1 - c, (1 - c) / 2]
    else:
        b = [(1 + c) / 2, -(1 + c), (1 + c) / 2]
    a = [1 + alpha, -2 * c, 1 - alpha]
    return np.array(b) / a[0], np.array(a) / a[0]


def sweep(x, fc, q=2.0, kind='bp', block=96):
    """Time-varying biquad; fc is an array as long as x (or a constant)."""
    fc = np.broadcast_to(fc, x.shape)
    y = np.empty_like(x)
    zi = np.zeros(2)
    for i in range(0, len(x), block):
        b, a = _biquad(kind, float(fc[i]), q)
        y[i:i + block], zi = lfilter(b, a, x[i:i + block], zi=zi)
    return y


def osc(freq, secs, shape='sine', harmonics=10):
    """freq: constant or per-sample array (Hz)."""
    n = n_of(secs)
    f = np.broadcast_to(np.asarray(freq, dtype=float), (n,)) if np.ndim(freq) == 0 else pad(np.asarray(freq, float), n)
    ph = TAU * np.cumsum(f) / SR
    if shape == 'sine':
        return np.sin(ph)
    out = np.zeros(n)
    for k in range(1, harmonics + 1):
        if shape == 'square' and k % 2 == 0:
            continue
        mask = (f * k) < SR * 0.45
        amp = 1.0 / k if shape in ('saw', 'square') else (1.0 / (k * k) if k % 2 else 0.0)
        out += np.where(mask, np.sin(k * ph) * amp, 0.0)
    return out / (1.2 if shape == 'saw' else 1.0)


def chirp(f0, f1, secs, kind='exp'):
    return osc(curve(secs, [(0, f0), (secs, f1)], kind), secs)


def fm_bell(f, secs, ratio=1.4, index=2.5, tau=0.8):
    t = tt(secs)
    env = np.exp(-t / tau)
    mod = index * env * np.sin(TAU * f * ratio * t)
    return np.sin(TAU * f * t + mod) * env * np.minimum(1.0, t / 0.002)


def pings(secs, rng, count, f_lo, f_hi, tau, t_lo=0.0, t_hi=None, amp=(0.4, 1.0)):
    buf = np.zeros(n_of(secs))
    t_hi = secs if t_hi is None else t_hi
    for _ in range(count):
        f = f_lo * (f_hi / f_lo) ** rng.random()
        d = min(secs, tau * 6)
        p = osc(f, d) * exp_decay(d, tau, 0.001) * (amp[0] + (amp[1] - amp[0]) * rng.random())
        at(buf, p, t_lo + (t_hi - t_lo) * rng.random())
    return buf


def grains(secs, rng, times, lo, hi, dur=(0.004, 0.02), amp=(0.3, 1.0)):
    """Short windowed band-passed noise bursts (crackle, gravel, debris)."""
    buf = np.zeros(n_of(secs))
    for ts in times:
        d = dur[0] + (dur[1] - dur[0]) * rng.random()
        g = rng.standard_normal(max(8, n_of(d))) * np.hanning(max(8, n_of(d)))
        at(buf, g, ts, amp[0] + (amp[1] - amp[0]) * rng.random())
    return bp(buf, lo, hi)


def poisson_times(secs, rng, rate_curve):
    """Event times whose rate (per second) follows rate_curve(t)."""
    times, t = [], 0.0
    while t < secs:
        r = max(1e-3, rate_curve(t))
        t += rng.exponential(1.0 / r)
        if t < secs:
            times.append(t)
    return times


def reverb(x, rng, rt60=0.8, mix=0.25, predelay=0.012, bright=6000):
    """Mono -> stereo with a decorrelated exponential-noise tail. The dry end gets a short fade so a layer that is
    still sounding when the recipe's buffer ends does not click."""
    x = x.copy()
    k = min(len(x), n_of(0.03))
    x[len(x) - k:] *= np.linspace(1, 0, k)
    n_ir = n_of(rt60 * 1.1)
    t = np.arange(n_ir) / SR
    decay = np.exp(-6.9 * t / rt60)
    out = []
    for _ in range(2):
        ir = lp(rng.standard_normal(n_ir) * decay, bright)
        ir = np.concatenate([np.zeros(n_of(predelay)), ir])
        ir /= np.sqrt(np.sum(ir ** 2)) + 1e-9
        wet = fftconvolve(x, ir)[:len(x) + n_ir]
        out.append(pad(x, len(wet)) + wet * mix)
    return np.stack(out)


def finish(stereo, peak_db, fade_in=0.002, fade_out=0.05, tail_to=None):
    if stereo.ndim == 1:
        stereo = np.stack([stereo, stereo])
    if tail_to is not None:
        stereo = stereo[:, :n_of(tail_to)]
    n = stereo.shape[1]
    ramp = np.ones(n)
    fi, fo = n_of(fade_in), n_of(fade_out)
    if fi:
        ramp[:fi] = np.linspace(0, 1, fi)
    if fo:
        ramp[-fo:] *= np.linspace(1, 0, fo)
    stereo = stereo * ramp
    peak = np.max(np.abs(stereo)) + 1e-9
    return stereo * (10 ** (peak_db / 20) / peak)


def trim_tail(stereo, floor_db=-50.0, keep=0.04):
    """Cut the reverb tail once it has fallen below floor_db of the peak, with a short fade."""
    level = np.abs(stereo).max(axis=0)
    loud = np.nonzero(level > level.max() * 10 ** (floor_db / 20))[0]
    end = min(stereo.shape[1], (loud[-1] if len(loud) else 0) + n_of(keep))
    out = stereo[:, :end].copy()
    k = min(end, n_of(keep))
    out[:, end - k:] *= np.linspace(1, 0, k)
    return out


def sat(x, drive=2.0):
    return np.tanh(x * drive) / np.tanh(drive)


def norm(x):
    return x / (np.max(np.abs(x)) + 1e-9)


# ================================================================ recipes (each returns a stereo array)
def magic_bolt_charge(rng):
    d = 0.42
    rise = curve(d, [(0, 0), (d * 0.85, 1), (d, 0.6)]) ** 1.5
    spark = sweep(white(d, rng), curve(d, [(0, 2000), (d, 7000)], 'exp'), 3.0) * rise
    tone = (osc(curve(d, [(0, 700), (d, 1700)], 'exp'), d) * 0.7 + osc(curve(d, [(0, 1400), (d, 3400)], 'exp'), d) * 0.2)
    tone *= rise * (0.7 + 0.3 * np.sin(TAU * 32 * tt(d)))
    return finish(reverb(norm(spark) * 0.5 + tone * 0.45, rng, 0.4, 0.2), -12, fade_out=0.03)


def magic_bolt_release(rng):
    d = 0.35
    zap = np.tanh(3 * chirp(2600, 320, d)) * exp_decay(d, 0.07, 0.001)
    click = hp(white(0.006, rng), 3000)
    body = sweep(white(d, rng), curve(d, [(0, 5000), (0.15, 900)], 'exp'), 2.0) * exp_decay(d, 0.05)
    x = at(zap * 0.7 + norm(body) * 0.35, click, 0, 0.6)
    return finish(reverb(x, rng, 0.35, 0.18), -3)


def magic_bolt_impact(rng):
    d = 0.4
    pop = chirp(900, 180, d) * exp_decay(d, 0.04, 0.001)
    burst = bp(white(d, rng), 2500, 9000) * exp_decay(d, 0.05, 0.001)
    sparkle = pings(d, rng, 7, 3000, 7000, 0.04, 0.01, 0.2)
    return finish(reverb(pop * 0.8 + norm(burst) * 0.5 + sparkle * 0.35, rng, 0.4, 0.22), -5)


def _crackle(d, rng, rate, lo=1200, hi=7000, dur=(0.002, 0.008)):
    return grains(d, rng, poisson_times(d, rng, rate), lo, hi, dur)


def fire_ball_charge(rng):
    d = 0.88
    rise = curve(d, [(0, 0.05), (d * 0.9, 1), (d, 0.8)]) ** 1.3
    roar = sweep(brown(d, rng), curve(d, [(0, 250), (d, 1400)], 'exp'), 0.9, 'lp') * rise
    crack = _crackle(d, rng, lambda t: 25 + 140 * t / d) * rise
    hiss = bp(white(d, rng), 2000, 6000) * rise ** 2 * 0.15
    return finish(reverb(norm(roar) * 0.8 + norm(crack) * 0.45 + hiss, rng, 0.5, 0.2), -6, fade_in=0.05)


def fire_ball_release(rng):
    d = 0.6
    env = curve(d, [(0, 0), (0.05, 1), (d, 0)]) ** 1.5
    whoosh = sweep(white(d, rng), curve(d, [(0, 300), (0.08, 1800), (d, 500)], 'exp'), 1.4) * env
    thump = osc(curve(d, [(0, 110), (0.15, 55)], 'exp'), d) * exp_decay(d, 0.09)
    crack = _crackle(d, rng, lambda t: 90 * math.exp(-t / 0.25)) * 0.5
    return finish(reverb(norm(whoosh) * 0.8 + thump * 0.6 + norm(crack) * 0.35, rng, 0.6, 0.22), -3)


def _explosion(d, rng, boom_f=(60, 32), boom_tau=0.3, noise_tau=0.25, lp_start=5000, lp_end=250, debris=90):
    boom = osc(curve(d, [(0, boom_f[0]), (0.4, boom_f[1])], 'exp'), d) * exp_decay(d, boom_tau, 0.003)
    body = sweep(white(d, rng), curve(d, [(0, lp_start), (d, lp_end)], 'exp'), 0.8, 'lp') * exp_decay(d, noise_tau, 0.002)
    crunch = sat(bp(white(d, rng), 150, 1200) * exp_decay(d, noise_tau * 0.5, 0.001), 3.0)
    rubble = _crackle(d, rng, lambda t: debris * math.exp(-t / (d * 0.35)) if t > 0.05 else 0.1, 600, 5000,
                      (0.003, 0.02))
    return sat(boom * 1.0 + norm(body) * 0.8 + norm(crunch) * 0.35 + norm(rubble) * 0.3, 1.6)


def fire_ball_impact(rng):
    d = 1.2
    return finish(reverb(_explosion(d, rng), rng, 1.1, 0.25), -1, fade_out=0.2)


def violet_flame_charge(rng):
    d = 0.98
    swell = curve(d, [(0, 0), (d * 0.85, 1), (d, 0.7)]) ** 1.4
    t = tt(d)
    vib = 1 + 0.012 * np.sin(TAU * 5 * t)
    ghost = (osc(220 * vib, d) + osc(233 * vib, d) * 0.8 + osc(311 * vib, d) * 0.6 + osc(466 * vib, d) * 0.25) * swell
    air = sweep(white(d, rng), 1300 + 700 * np.sin(TAU * 3.5 * t), 3.0) * swell * (0.6 + 0.4 * np.sin(TAU * 4 * t))
    return finish(reverb(norm(ghost) * 0.55 + norm(air) * 0.45, rng, 1.0, 0.35), -8, fade_in=0.08)


def violet_flame_release(rng):
    d = 0.7
    env = curve(d, [(0, 0), (0.06, 1), (d, 0)]) ** 1.4
    whoosh = sweep(white(d, rng), curve(d, [(0, 200), (0.1, 1300), (d, 350)], 'exp'), 1.2) * env
    fall = (chirp(520, 260, d) + chirp(540, 268, d) * 0.8) * exp_decay(d, 0.25, 0.01)
    return finish(reverb(norm(whoosh) * 0.75 + norm(fall) * 0.4, rng, 0.9, 0.3), -3)


def violet_flame_impact(rng):
    d = 1.0
    whump = osc(curve(d, [(0, 95), (0.3, 40)], 'exp'), d) * exp_decay(d, 0.18)
    dark = lp(white(d, rng), 900) * exp_decay(d, 0.15, 0.002)
    t = tt(d)
    shimmer = sum(osc(f * (1 + 0.02 * np.sin(TAU * 9 * t + k)), d) for k, f in enumerate((1180, 1250, 1590, 2010)))
    shimmer *= exp_decay(d, 0.35, 0.02)
    return finish(reverb(whump * 0.8 + norm(dark) * 0.5 + norm(shimmer) * 0.35, rng, 1.2, 0.35), -3, fade_out=0.2)


def wind_cutter_charge(rng):
    d = 0.5
    t = tt(d)
    rise = curve(d, [(0, 0.05), (d * 0.9, 1), (d, 0.8)])
    fc = curve(d, [(0, 600), (d, 2600)], 'exp') * (1 + 0.35 * np.sin(TAU * 7 * t))
    wind = sweep(white(d, rng), fc, 4.0) * rise
    return finish(reverb(norm(wind), rng, 0.5, 0.25), -8, fade_in=0.04)


def wind_cutter_release(rng):
    d = 0.45
    env = curve(d, [(0, 0), (0.03, 1), (0.12, 0.7), (d, 0)]) ** 1.3
    swish = sweep(white(d, rng), curve(d, [(0, 1000), (0.12, 6500), (d, 2500)], 'exp'), 2.5) * env
    ring = (osc(3400, d) + osc(5100, d) * 0.6) * exp_decay(d, 0.09, 0.005)
    return finish(reverb(norm(swish) * 0.85 + ring * 0.12, rng, 0.45, 0.2), -3)


def wind_cutter_impact(rng):
    d = 0.5
    slice_ = bp(white(d, rng), 4000, 11000) * exp_decay(d, 0.02, 0.001)
    swish = sweep(white(d, rng), curve(d, [(0, 5000), (d, 1500)], 'exp'), 2.0) * exp_decay(d, 0.08, 0.004)
    gust = bp(white(d, rng), 500, 1600) * curve(d, [(0, 0), (0.05, 1), (d, 0)]) ** 2
    return finish(reverb(norm(slice_) * 0.6 + norm(swish) * 0.5 + norm(gust) * 0.4, rng, 0.5, 0.22), -4)


def explosion_blast_charge(rng):
    d = 1.33
    swell = curve(d, [(0, 0.05), (1.1, 1), (1.25, 0.25), (d, 0.1)]) ** 1.3
    roar = sweep(brown(d, rng), curve(d, [(0, 200), (1.1, 1000), (d, 1600)], 'exp'), 0.9, 'lp') * swell
    crack = _crackle(d, rng, lambda t: 20 + 160 * min(t, 1.1) / 1.1) * swell
    suck = osc(curve(d, [(0, 200), (1.1, 200), (d, 900)], 'exp'), d) * curve(d, [(0, 0), (1.08, 0), (1.2, 1), (d, 0.6)])
    return finish(reverb(norm(roar) * 0.75 + norm(crack) * 0.45 + suck * 0.3, rng, 0.6, 0.25), -6, fade_in=0.05)


def explosion_blast_release(rng):
    d = 0.6
    thud = osc(curve(d, [(0, 90), (0.2, 50)], 'exp'), d) * exp_decay(d, 0.12)
    rush = bp(white(d, rng), 400, 2200) * exp_decay(d, 0.1, 0.003)
    crack = _crackle(d, rng, lambda t: 120 * math.exp(-t / 0.15)) * 0.5
    return finish(reverb(thud * 0.8 + norm(rush) * 0.6 + norm(crack) * 0.35, rng, 0.7, 0.25), -3)


def explosion_blast_sigil(rng):
    d = 0.6
    t = tt(d)
    hum = (osc(110, d) + osc(165, d) * 0.6) * curve(d, [(0, 0), (0.15, 1), (d, 0.8)]) * (0.7 + 0.3 * np.sin(TAU * 8 * t))
    ticks = np.zeros(n_of(d))
    for ts in (0.05, 0.22, 0.34, 0.43, 0.5, 0.55):
        at(ticks, osc(1500, 0.05) * exp_decay(0.05, 0.012, 0.001), ts)
    return finish(reverb(norm(hum) * 0.6 + ticks * 0.35, rng, 0.6, 0.25), -8, fade_in=0.02)


def explosion_blast_burst(rng):
    d = 2.0
    x = _explosion(d, rng, boom_f=(55, 28), boom_tau=0.5, noise_tau=0.4, lp_start=6000, lp_end=200, debris=120)
    return finish(reverb(x, rng, 1.8, 0.3), -1, fade_out=0.35)


def quake_blast_charge(rng):
    d = 1.05
    swell = curve(d, [(0, 0.1), (d, 1)]) ** 1.2
    rumble = lp(brown(d, rng), 160) * (0.8 + 0.2 * np.sin(TAU * 11 * tt(d))) * swell
    gravel = grains(d, rng, poisson_times(d, rng, lambda t: 30 + 150 * t / d), 900, 3200, (0.004, 0.015)) * swell
    return finish(reverb(norm(rumble) * 0.85 + norm(gravel) * 0.35, rng, 0.6, 0.2), -7, fade_in=0.06)


def quake_blast_release(rng):
    d = 1.2
    thud = osc(curve(d, [(0, 70), (0.3, 38)], 'exp'), d) * exp_decay(d, 0.3, 0.002)
    body = lp(white(d, rng), 300) * exp_decay(d, 0.12, 0.001)
    crack = bp(white(d, rng), 1800, 5500) * exp_decay(d, 0.03, 0.001)
    tail = lp(brown(d, rng), 120) * curve(d, [(0, 0), (0.08, 1), (d, 0)])
    x = sat(thud * 1.0 + norm(body) * 0.7 + norm(crack) * 0.3 + norm(tail) * 0.5, 1.8)
    return finish(reverb(x, rng, 0.9, 0.2), -1, fade_out=0.2)


def quake_blast_sigil(rng):
    d = 0.8
    rumble = lp(brown(d, rng), 110) * (0.6 + 0.4 * np.abs(np.sin(TAU * 7 * tt(d)))) * curve(d, [(0, 0), (d, 1)])
    clicks = grains(d, rng, poisson_times(d, rng, lambda t: 10 + 40 * t / d), 1500, 5000, (0.002, 0.006))
    return finish(reverb(norm(rumble) * 0.8 + norm(clicks) * 0.2, rng, 0.5, 0.2), -6, fade_in=0.05)


def quake_blast_burst(rng):
    d = 1.8
    x = _explosion(d, rng, boom_f=(65, 30), boom_tau=0.4, noise_tau=0.25, lp_start=3500, lp_end=180, debris=60)
    crash = grains(d, rng, poisson_times(0.5, rng, lambda t: 140), 800, 4000, (0.01, 0.05))
    falls = grains(d, rng, [0.5 + t for t in poisson_times(1.1, rng, lambda t: 25 * math.exp(-t / 0.6))],
                   1200, 5000, (0.003, 0.012))
    return finish(reverb(x + norm(crash) * 0.5 + norm(falls) * 0.25, rng, 1.4, 0.25), -1, fade_out=0.3)


def rock_wall_charge(rng):
    d = 0.68
    swell = curve(d, [(0, 0.1), (d, 1)])
    rumble = lp(brown(d, rng), 180) * swell
    scrape = sweep(white(d, rng), curve(d, [(0, 500), (d, 1400)], 'exp'), 3.0) * swell ** 2
    gravel = grains(d, rng, poisson_times(d, rng, lambda t: 60), 1000, 3500, (0.004, 0.012)) * swell
    return finish(reverb(norm(rumble) * 0.7 + norm(scrape) * 0.3 + norm(gravel) * 0.3, rng, 0.5, 0.2), -8, fade_in=0.04)


def rock_wall_release(rng):
    d = 0.4
    swoosh = sweep(white(d, rng), curve(d, [(0, 300), (0.08, 1500), (d, 500)], 'exp'), 1.5) * \
        curve(d, [(0, 0), (0.04, 1), (d, 0)])
    thud = osc(80, d) * exp_decay(d, 0.07)
    return finish(reverb(norm(swoosh) * 0.7 + thud * 0.5, rng, 0.5, 0.2), -4)


def rock_wall_rise(rng):
    d = 1.0
    grind_env = curve(d, [(0, 0), (0.03, 1), (0.35, 0.8), (d, 0)])
    grind = (lp(brown(d, rng), 220) * 0.8 + norm(grains(d, rng, poisson_times(d, rng, lambda t: 220 * math.exp(-t / 0.4)),
                                                         400, 2600, (0.005, 0.02))) * 0.5) * grind_env
    thud = osc(curve(d, [(0, 75), (0.2, 45)], 'exp'), d) * exp_decay(d, 0.18, 0.003)
    return finish(reverb(sat(norm(grind) * 0.8 + thud * 0.9, 1.5), rng, 0.8, 0.22), -2, fade_out=0.15)


def rock_wall_crumble(rng):
    d = 1.3
    rubble = grains(d, rng, poisson_times(d, rng, lambda t: 260 * math.exp(-t / 0.45)), 500, 4500, (0.006, 0.04))
    rumble = lp(brown(d, rng), 150) * curve(d, [(0, 0), (0.05, 1), (d, 0)])
    thump = osc(60, d) * exp_decay(d, 0.15)
    return finish(reverb(norm(rubble) * 0.7 + norm(rumble) * 0.5 + thump * 0.5, rng, 0.9, 0.22), -3, fade_out=0.2)


def _electric_buzz(d, f=100):
    t = tt(d)
    return np.tanh(3 * osc(f * (1 + 0.01 * np.sin(TAU * 13 * t)), d, 'saw'))


def thunder_trap_charge(rng):
    d = 0.61
    swell = curve(d, [(0, 0.05), (d * 0.9, 1), (d, 0.7)]) ** 1.5
    buzz = bp(_electric_buzz(d, 110), 150, 3000) * swell
    zaps = grains(d, rng, poisson_times(d, rng, lambda t: 15 + 120 * t / d), 1500, 9000, (0.002, 0.01)) * swell
    return finish(reverb(norm(buzz) * 0.55 + norm(zaps) * 0.5, rng, 0.4, 0.2), -8, fade_in=0.03)


def _thunder_crack(d, rng):
    crack = bp(white(d, rng), 900, 9000) * exp_decay(d, 0.035, 0.0005)
    zap = np.tanh(2 * chirp(3200, 180, d)) * exp_decay(d, 0.05, 0.001)
    return norm(crack) * 0.8 + zap * 0.35


def thunder_trap_release(rng):
    d = 0.5
    rumble = lp(brown(d, rng), 200) * curve(d, [(0, 0), (0.05, 1), (d, 0)])
    return finish(reverb(_thunder_crack(d, rng) + norm(rumble) * 0.4, rng, 0.6, 0.25), -2)


def thunder_trap_set(rng):
    d = 0.8
    env = curve(d, [(0, 0), (0.1, 1), (0.6, 0.7), (d, 0)])
    hum = bp(_electric_buzz(d, 60), 80, 1500) * env
    crackle = grains(d, rng, poisson_times(d, rng, lambda t: 25), 2000, 8000, (0.002, 0.006)) * env
    return finish(reverb(norm(hum) * 0.6 + norm(crackle) * 0.3, rng, 0.5, 0.25), -9)


def thunder_trap_strike(rng):
    d = 1.5
    burst = bp(white(d, rng), 700, 9000) * exp_decay(d, 0.05, 0.0005)
    body = bp(white(d, rng), 90, 900) * curve(d, [(0, 0), (0.08, 1), (1.0, 0.1), (d, 0)])
    body *= 0.5 + 0.5 * norm(np.abs(grains(d, rng, poisson_times(d, rng, lambda t: 40), 20, 200, (0.01, 0.05))))
    t = tt(d)
    rumble = lp(brown(d, rng), 150) * (1 + 0.3 * np.sin(TAU * 3.1 * t)) * curve(d, [(0, 0), (0.3, 1), (d, 0)])
    x = norm(burst) * 0.9 + norm(body) * 0.6 + norm(rumble) * 0.6 + np.tanh(2 * chirp(3000, 150, d)) * exp_decay(d, 0.04)
    return finish(reverb(sat(x, 1.4), rng, 1.3, 0.25), -1, fade_out=0.3)


def _chord(freqs, secs, shape='saw', detune=0.004, vib=0.0):
    t = tt(secs)
    out = np.zeros(n_of(secs))
    for f in freqs:
        for s in (-1, 1):
            out += osc(f * (1 + s * detune) * (1 + vib * np.sin(TAU * 5.2 * t)), secs, shape)
    return out


def might_charge(rng):
    d = 1.17
    rise = curve(d, [(0, 0), (d * 0.95, 1), (d, 0.9)]) ** 1.3
    t = tt(d)
    bend = curve(d, [(0, 1.0), (d, 2 ** (5 / 12))], 'exp')
    hum = sum(osc(f * bend, d, 'saw') for f in (110, 165, 220))
    hum = sweep(hum, curve(d, [(0, 300), (d, 2500)], 'exp'), 0.8, 'lp') * rise
    shimmer = pings(d, rng, 18, 2000, 6000, 0.08, d * 0.3, d) * 0.5
    return finish(reverb(norm(hum) * 0.7 + shimmer * 0.4, rng, 0.8, 0.28), -7, fade_in=0.1)


def might_release(rng):
    d = 1.2
    whoosh = sweep(white(d, rng), curve(d, [(0, 300), (0.15, 4000), (d, 1500)], 'exp'), 1.5) * \
        curve(d, [(0, 0), (0.12, 1), (d, 0)]) ** 2
    hit = lp(_chord((261.6, 329.6, 392.0, 523.3), d, 'saw', 0.003), 3000) * exp_decay(d, 0.45, 0.01)
    thump = osc(curve(d, [(0, 90), (0.2, 50)], 'exp'), d) * exp_decay(d, 0.15)
    return finish(reverb(norm(whoosh) * 0.5 + norm(hit) * 0.6 + thump * 0.6, rng, 1.2, 0.3), -2, fade_out=0.25)


def heal_charge(rng):
    d = 1.42
    swell = curve(d, [(0, 0), (d * 0.9, 1), (d, 0.85)]) ** 1.2
    pad_ = _chord((523.3, 659.3, 784.0), d, 'sine', 0.003, 0.004) * swell
    air = bp(white(d, rng), 3000, 9000) * swell * 0.1
    twinkle = pings(d, rng, 14, 2500, 7000, 0.12, 0.2, d) * 0.4
    return finish(reverb(norm(pad_) * 0.6 + air + twinkle, rng, 1.4, 0.4), -9, fade_in=0.15)


def heal_release(rng):
    d = 2.0
    bells = np.zeros(n_of(d))
    for i, f in enumerate((1046.5, 1318.5, 1568.0, 2093.0)):
        at(bells, fm_bell(f, d - i * 0.04, 1.41, 2.2, 0.7), i * 0.04, 1.0 - i * 0.12)
    shimmer = pings(d, rng, 26, 3000, 8000, 0.15, 0.05, 1.3) * 0.35
    pad_ = _chord((523.3, 659.3, 784.0), d, 'sine', 0.004, 0.004) * curve(d, [(0, 0), (0.1, 1), (d, 0)]) * 0.25
    return finish(reverb(norm(bells) * 0.7 + shimmer + norm(pad_) * 0.3, rng, 1.8, 0.4), -3, fade_out=0.4)


def arcane_ray_charge(rng):
    d = 0.89
    t = tt(d)
    rise = curve(d, [(0, 0.05), (d * 0.92, 1), (d, 0.8)]) ** 1.4
    f = curve(d, [(0, 250), (d, 1400)], 'exp')
    trem_rate = curve(d, [(0, 6), (d, 30)])
    trem = 0.65 + 0.35 * np.sin(TAU * np.cumsum(trem_rate) / SR)
    whine = (osc(f, d) + osc(f * 2, d) * 0.3 + osc(f * 3.01, d) * 0.15) * rise * trem
    hiss = bp(white(d, rng), 4000, 10000) * rise ** 2 * 0.2
    return finish(reverb(norm(whine) * 0.6 + hiss, rng, 0.6, 0.3), -7, fade_in=0.04)


def arcane_ray_release(rng):
    d = 1.95
    t = tt(d)
    burst = bp(white(d, rng), 300, 5000) * exp_decay(d, 0.06, 0.001)
    thump = osc(curve(d, [(0, 120), (0.1, 60)], 'exp'), d) * exp_decay(d, 0.08)
    beam = osc(110, d, 'saw') + osc(110.7, d, 'saw') * 0.8 + osc(221, d, 'saw') * 0.4
    beam = lp(beam, 1600)
    delay = (0.003 + 0.002 * np.sin(TAU * 0.7 * t)) * SR
    idx = np.clip(np.arange(len(beam)) - delay, 0, len(beam) - 1)
    beam = beam + np.interp(idx, np.arange(len(beam)), beam)
    beam *= (0.8 + 0.2 * np.sin(TAU * 12 * t)) * curve(d, [(0, 0), (0.05, 1), (d - 0.3, 0.9), (d, 0)])
    sizzle = bp(white(d, rng), 5000, 12000) * curve(d, [(0, 0), (0.05, 1), (d - 0.3, 0.8), (d, 0)]) * 0.12
    return finish(reverb(norm(burst) * 0.5 + thump * 0.5 + norm(beam) * 0.55 + sizzle, rng, 0.8, 0.25), -3, fade_out=0.1)


def arcane_ray_hit(rng):
    d = 0.2
    s = bp(white(d, rng), 3000, 8000) * exp_decay(d, 0.05, 0.001) * (0.5 + 0.5 * np.sign(np.sin(TAU * 60 * tt(d))))
    return finish(reverb(s, rng, 0.25, 0.15), -10)


def frost_breath_charge(rng):
    d = 0.81
    rise = curve(d, [(0, 0.05), (d, 1)]) ** 1.3
    icy = pings(d, rng, 22, 2200, 7000, 0.12, 0.0, d) * np.maximum(rise, 0.2)
    wind = sweep(white(d, rng), curve(d, [(0, 1500), (d, 4000)], 'exp'), 3.0) * rise * 0.3
    return finish(reverb(icy * 0.6 + norm(wind) * 0.35, rng, 0.8, 0.35), -8, fade_in=0.03)


def frost_breath_release(rng):
    d = 1.5
    t = tt(d)
    env = curve(d, [(0, 0), (0.06, 1), (d - 0.25, 0.85), (d, 0)])
    hiss = sweep(white(d, rng), 3500 + 1500 * np.sin(TAU * 0.9 * t), 1.2) * env
    body = lp(white(d, rng), 900) * env * 0.4
    tinkle = pings(d, rng, 30, 3000, 9000, 0.06, 0.05, d - 0.2) * 0.35
    return finish(reverb(norm(hiss) * 0.6 + norm(body) * 0.35 + tinkle, rng, 0.9, 0.3), -3, fade_out=0.1)


def frost_breath_hit(rng):
    d = 0.2
    click = hp(white(d, rng), 4000) * exp_decay(d, 0.008, 0.0005)
    ping = osc(4200, d) * exp_decay(d, 0.05, 0.001)
    return finish(reverb(norm(click) * 0.6 + ping * 0.4, rng, 0.3, 0.2), -10)


SOUNDS = {
    'MagicBolt_Charge': magic_bolt_charge, 'MagicBolt_Release': magic_bolt_release,
    'MagicBolt_Impact': magic_bolt_impact,
    'FireBall_Charge': fire_ball_charge, 'FireBall_Release': fire_ball_release, 'FireBall_Impact': fire_ball_impact,
    'VioletFlame_Charge': violet_flame_charge, 'VioletFlame_Release': violet_flame_release,
    'VioletFlame_Impact': violet_flame_impact,
    'WindCutter_Charge': wind_cutter_charge, 'WindCutter_Release': wind_cutter_release,
    'WindCutter_Impact': wind_cutter_impact,
    'ExplosionBlast_Charge': explosion_blast_charge, 'ExplosionBlast_Release': explosion_blast_release,
    'ExplosionBlast_Sigil': explosion_blast_sigil, 'ExplosionBlast_Burst': explosion_blast_burst,
    'QuakeBlast_Charge': quake_blast_charge, 'QuakeBlast_Release': quake_blast_release,
    'QuakeBlast_Sigil': quake_blast_sigil, 'QuakeBlast_Burst': quake_blast_burst,
    'RockWall_Charge': rock_wall_charge, 'RockWall_Release': rock_wall_release, 'RockWall_Rise': rock_wall_rise,
    'RockWall_Crumble': rock_wall_crumble,
    'ThunderTrap_Charge': thunder_trap_charge, 'ThunderTrap_Release': thunder_trap_release,
    'ThunderTrap_Set': thunder_trap_set, 'ThunderTrap_Strike': thunder_trap_strike,
    'Might_Charge': might_charge, 'Might_Release': might_release,
    'Heal_Charge': heal_charge, 'Heal_Release': heal_release,
    'ArcaneRay_Charge': arcane_ray_charge, 'ArcaneRay_Release': arcane_ray_release, 'ArcaneRay_Hit': arcane_ray_hit,
    'FrostBreath_Charge': frost_breath_charge, 'FrostBreath_Release': frost_breath_release,
    'FrostBreath_Hit': frost_breath_hit,
}


# ================================================================ output
META = """{{
    "value0": {{
        "polymorphic_id": 2147483649,
        "polymorphic_name": "NanamiEngine::Module::Asset::SoundFile",
        "ptr_wrapper": {{
            "id": 2147483649,
            "data": {{
                "cereal_class_version": 0,
                "value0": {{
                    "cereal_class_version": 0
                }},
                "contentPath_": "{path}",
                "guid_": {{
                    "cereal_class_version": 0,
                    "value_": "{guid}"
                }},
                "volume_": 255
            }}
        }}
    }}
}}"""


def encode_mp3(stereo):
    pcm = (np.clip(stereo.T, -1, 1) * 32767).astype('<i2')
    enc = lameenc.Encoder()
    enc.set_bit_rate(192)
    enc.set_in_sample_rate(SR)
    enc.set_channels(2)
    enc.set_quality(2)
    return enc.encode(pcm.tobytes()) + enc.flush()


def write_sound(name, stereo):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / f'{name}.mp3'
    path.write_bytes(encode_mp3(stereo))
    meta = Path(str(path) + '.meta')
    if not meta.exists():
        content = str(path.relative_to(REPO)).replace('/', '\\').replace('\\', '\\\\')
        text = META.format(path=content, guid=str(uuid.uuid4()).upper()).replace('\n', '\r\n')
        meta.write_bytes(text.encode('utf-8'))
    return path


def render_preview(rendered, out):
    from PIL import Image, ImageDraw
    w, h = 900, 60
    img = Image.new('RGB', (w, h * len(rendered)), (18, 20, 26))
    d = ImageDraw.Draw(img)
    longest = max(s.shape[1] for s in rendered.values())
    for i, (name, s) in enumerate(rendered.items()):
        mono = np.abs(s).max(axis=0)
        cols = np.array_split(mono, max(1, int(w * s.shape[1] / longest)))
        y0 = i * h
        for x, c in enumerate(cols):
            v = float(c.max()) if len(c) else 0.0
            d.line([(x, y0 + h / 2 - v * h * 0.45), (x, y0 + h / 2 + v * h * 0.45)], fill=(120, 200, 255))
        d.text((4, y0 + 2), f'{name}  {s.shape[1] / SR:.2f}s  peak {20 * np.log10(np.abs(s).max() + 1e-9):.1f}dB',
               fill=(240, 240, 240))
    img.save(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--only', nargs='*')
    ap.add_argument('--preview')
    args = ap.parse_args()
    rendered = {}
    for i, name in enumerate(args.only or SOUNDS):
        rng = np.random.default_rng(1000 + sorted(SOUNDS).index(name))
        stereo = trim_tail(SOUNDS[name](rng))
        rendered[name] = stereo
        path = write_sound(name, stereo)
        print(f'{name:26s} {stereo.shape[1] / SR:5.2f}s  {path.stat().st_size // 1024:3d} KB')
    if args.preview:
        render_preview(rendered, args.preview)


if __name__ == '__main__':
    main()
