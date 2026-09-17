"""Generate the action-instruction tutorial card sprites used by
Assets/Prefab/UI/ActionInstructTutorial/ActionInstructTutorialUI.prefab (GamePlay::Ui::SwordManActionInstructTutorial).

    python tools/art/tutorial_card.py [--out-dir Assets/Art/UI/ActionTutorial] [--preview PATH]

The card is the "callout" design: it sits just right of the control guide's focused row and points at it
with a tail, so the課題 text and the input to press read as one thing. Sprites share the guide's palette
(tools/art/control_guide.py): dark teal panel, gold while a課題 is running, teal once it is cleared.
Writes a SpriteFile .png.meta for each (an existing .meta keeps its GUID).
The prefab layout the sprites are sized for is printed at the end (GEOMETRY), relative to the card centre.

Requires Pillow + numpy.
"""
import argparse
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT))
from tools.art.control_guide import Canvas, GOLD, TEAL, blur, cov, hexc, sd_rbox  # noqa: E402
from tools.scene import sprite_meta  # noqa: E402

CARD_W, CARD_H = 400, 104
TAIL_W, TAIL_H = 18, 26
ACCENT_W, ACCENT_INSET_Y = 4, 4
CHECK = 30

PANEL = hexc('#0b181e')
BORDER = hexc('#38d6c4')

# ipam.ttf .meta size_。TextRenderer は localScale で縮めて使う
LABEL_FONT_SIZE = 60
STEP_SIZE, TITLE_SIZE, BODY_SIZE, CLEAR_SIZE = 18, 30, 18, 30
TEXT_LEFT = -CARD_W / 2 + 22


def render_panel():
    c = Canvas(CARD_W, CARD_H)
    X, Y = c.X, c.Y
    d = sd_rbox(X, Y, 0.5, 0.5, CARD_W - 0.5, CARD_H - 0.5, 5)
    c.over(PANEL, cov(d) * 0.88)
    c.over(BORDER, cov(d) * (1 - cov(d + 1.2)) * 0.3)
    # 上端のハイライトで板に厚みを出す
    c.over(hexc('#dff6f2'), cov(sd_rbox(X, Y, 6, 1.0, CARD_W - 6, 2.0, 0.5)) * 0.1)
    return c.resolve()


def render_accent(color):
    h = CARD_H - ACCENT_INSET_Y * 2
    c = Canvas(ACCENT_W + 6, h)
    X, Y = c.X, c.Y
    bar = cov(sd_rbox(X, Y, 3, 0.5, 3 + ACCENT_W, h - 0.5, 0.8))
    c.over(color, np.clip(blur(bar, 2.2) * 1.6, 0, 1) * 0.35)
    c.over(color, bar)
    return c.resolve()


def render_tail():
    c = Canvas(TAIL_W, TAIL_H)
    X, Y = c.X, c.Y
    # 左を向いた三角形。x が右へ進むほど許される半分の高さが増える
    half = X / TAIL_W * (TAIL_H / 2)
    c.over(PANEL, cov(np.abs(Y - TAIL_H / 2) - half) * 0.88)
    return c.resolve()


def render_check(color):
    c = Canvas(CHECK, CHECK)
    X, Y = c.X, c.Y
    pts = ((CHECK * 0.16, CHECK * 0.52), (CHECK * 0.40, CHECK * 0.76), (CHECK * 0.84, CHECK * 0.22))
    mark = np.zeros_like(X)
    for (ax, ay), (bx, by) in zip(pts[:-1], pts[1:]):
        vx, vy = bx - ax, by - ay
        length2 = vx * vx + vy * vy
        t = np.clip(((X - ax) * vx + (Y - ay) * vy) / length2, 0, 1)
        mark = np.maximum(mark, cov(np.hypot(X - (ax + vx * t), Y - (ay + vy * t)) - 2.1))
    c.over(color, np.clip(blur(mark, 2.5) * 1.5, 0, 1) * 0.45)
    c.over(color, mark)
    return c.resolve()


SPRITES = {
    'TutorialCard_Panel': render_panel,
    'TutorialCard_Accent': lambda: render_accent(GOLD),
    'TutorialCard_AccentCleared': lambda: render_accent(TEAL),
    'TutorialCard_Tail': render_tail,
    'TutorialCard_Check': lambda: render_check(TEAL),
}

GEOMETRY = {
    'Card root localPos (guide 右隣、フォーカス行の高さに追従)': (16 + 300 + 26 + CARD_W / 2, 540),
    'Panel localPos': (0, 0),
    'Tail localPos': (-CARD_W / 2 - TAIL_W / 2 + 1, 0),
    'Accent localPos': (-CARD_W / 2 + ACCENT_W / 2 + 3, 0),
    'StepText localPos': (TEXT_LEFT, -CARD_H / 2 + 14),
    'TitleText localPos': (TEXT_LEFT, -CARD_H / 2 + 36),
    'BodyText localPos': (TEXT_LEFT, -CARD_H / 2 + 70),
    'ClearMark localPos': (TEXT_LEFT + CHECK / 2 - 2, 6),
    'ClearText localPos': (TEXT_LEFT + CHECK + 12, -9),
    'StepText / BodyText localScale': STEP_SIZE / LABEL_FONT_SIZE,
    'TitleText / ClearText localScale': TITLE_SIZE / LABEL_FONT_SIZE,
}


def render_preview(sprites, path):
    bg = Image.new('RGBA', (CARD_W + 120, CARD_H + 80), (74, 92, 60, 255))
    x, y = 80, 40
    bg.alpha_composite(sprites['TutorialCard_Tail'], (x - TAIL_W + 1, y + (CARD_H - TAIL_H) // 2))
    bg.alpha_composite(sprites['TutorialCard_Panel'], (x, y))
    accent = sprites['TutorialCard_Accent']
    bg.alpha_composite(accent, (x, y + ACCENT_INSET_Y))
    bg.alpha_composite(sprites['TutorialCard_Check'], (x + 22, y + CARD_H // 2 - 8))
    bg.convert('RGB').save(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out-dir', default='Assets/Art/UI/ActionTutorial')
    ap.add_argument('--preview', help='also write a composite of the card parts to this PNG path')
    args = ap.parse_args()

    out_dir = Path(args.out_dir)
    if not out_dir.is_absolute():
        out_dir = REPO_ROOT / out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    sprites = {}
    for name, fn in SPRITES.items():
        sprites[name] = fn()
        png = out_dir / f"{name}.png"
        sprites[name].save(png)
        meta = out_dir / f"{name}.png.meta"
        guid = sprite_meta.read_meta(meta)["guid"] if meta.exists() else sprite_meta.mint_guid()
        sprite_meta.write_meta(meta, name, guid, sprite_meta.content_path_for(name, out_dir, REPO_ROOT))
        print(f"{name:28s} {sprites[name].size[0]}x{sprites[name].size[1]}  {guid}")

    print("GEOMETRY (ActionInstructTutorialUI.prefab, card 中心からの相対座標):")
    for k, v in GEOMETRY.items():
        print(f"  {k} = {v}")

    if args.preview:
        render_preview(sprites, args.preview)
        print(f"preview -> {args.preview}")


if __name__ == '__main__':
    main()
