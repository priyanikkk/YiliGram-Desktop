#!/usr/bin/env python3
"""Generates the YiliGram Desktop application icons from tools/yiligram/logo.png.

    python3 -m venv .venv && .venv/bin/pip install numpy scipy scikit-image pillow
    .venv/bin/python tools/yiligram/gen_icons.py
"""
import os
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage as ndi

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ART = os.path.join(ROOT, "Telegram", "Resources", "art")
LOGO = os.path.join(ROOT, "tools", "yiligram", "logo.png")
SUPERSAMPLE = 4
FOREGROUND_WIDTH_DP = 52.0
BACKGROUND = ("#34302A", "#0B0A08", 315)


def disk(r):
    a = np.arange(-r, r + 1) ** 2
    return np.add.outer(a, a) <= r * r


def hex_rgb(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


# ---------------------------------------------------------------- segmentation

def segment(logo_path):
    src = Image.open(logo_path).convert("RGBA")
    s = SUPERSAMPLE
    big = src.resize((src.width * s, src.height * s), Image.LANCZOS)
    rgb = np.asarray(big.convert("RGB")).astype(float)
    lum = ndi.gaussian_filter(rgb.mean(2), 1.5)

    bright = lum > 90
    labels, _ = ndi.label(bright)
    cy, cx = lum.shape[0] // 2, lum.shape[1] // 2
    center_label = labels[cy, cx]
    if center_label == 0:
        sys.exit("Could not find the mascot at the center of the logo")
    sil = labels == center_label
    sil = ndi.binary_closing(sil, structure=disk(14))
    sil = ndi.binary_fill_holes(sil)
    sil = ndi.binary_opening(sil, structure=disk(6))
    soft = ndi.gaussian_filter(sil.astype(float), 3)  # anti-aliased alpha
    sil = soft > 0.5

    inner = ndi.binary_erosion(sil, structure=disk(10))
    feat = (lum < 70) & inner
    feat = ndi.binary_opening(feat, structure=disk(2))
    feat = ndi.gaussian_filter(feat.astype(float), 1.5) > 0.5

    ys, xs = np.nonzero(sil)
    bbox = (xs.min(), ys.min(), xs.max() + 1, ys.max() + 1)

    cutout = np.asarray(big).copy()
    cutout[..., 3] = (np.clip(soft * 2 - 0.5, 0, 1) * 255).astype(np.uint8)
    cutout = Image.fromarray(cutout).crop(bbox)
    mono = (sil & ~feat)[bbox[1]:bbox[3], bbox[0]:bbox[2]]
    return cutout, mono


def gradient(size, start, end, angle):
    """Linear gradient, angle in degrees as in Android <gradient> (0 = left→right, 90 = bottom→top)."""
    w, h = size
    t = np.radians(angle)
    dx, dy = np.cos(t), -np.sin(t)
    yy, xx = np.mgrid[0:h, 0:w]
    u = ((xx / max(w - 1, 1) - 0.5) * dx + (yy / max(h - 1, 1) - 0.5) * dy)
    u = (u - u.min()) / (u.max() - u.min())
    a, b = np.array(hex_rgb(start), float), np.array(hex_rgb(end), float)
    img = a + (b - a) * u[..., None]
    return Image.fromarray(np.dstack([img, np.full((h, w), 255.0)]).astype(np.uint8), "RGBA")


def foreground(cutout, px):
    canvas = Image.new("RGBA", (px, px), (0, 0, 0, 0))
    w = round(px * FOREGROUND_WIDTH_DP / 108.0)
    h = round(w * cutout.height / cutout.width)
    cat = cutout.resize((w, h), Image.LANCZOS)
    canvas.alpha_composite(cat, ((px - w) // 2, (px - h) // 2))
    return canvas


def icon(cutout, px, margin=0.0):
    """Round icon: dark gradient disc with the mascot, optional transparent margin."""
    s = SUPERSAMPLE
    full = round(px * 108 / 72) * s
    start, end, angle = BACKGROUND
    img = gradient((full, full), start, end, angle)
    img.alpha_composite(foreground(cutout, full))
    off = (full - px * s) // 2
    img = img.crop((off, off, off + px * s, off + px * s))
    mask = Image.new("L", img.size, 0)
    m = round(px * s * margin)
    ImageDraw.Draw(mask).ellipse((m, m, img.width - 1 - m, img.height - 1 - m), fill=255)
    img.putalpha(mask)
    return img.resize((px, px), Image.LANCZOS)


def main():
    cutout, _ = segment(LOGO)
    for size in (16, 32, 48, 64, 128, 256, 512):
        icon(cutout, size, 0.04).save(os.path.join(ART, f"icon{size}.png"))
        icon(cutout, size * 2, 0.04).save(os.path.join(ART, f"icon{size}@2x.png"))
    icon(cutout, 256).save(os.path.join(ART, "logo_256.png"))
    icon(cutout, 256).save(os.path.join(ART, "logo_256_no_margin.png"))
    icon(cutout, 1024).save(os.path.join(ART, "icon_round512@2x.png"))
    big = icon(cutout, 256, 0.02)
    big.save(os.path.join(ART, "icon256.ico"), sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    print("icons written to", ART)


if __name__ == "__main__":
    main()
