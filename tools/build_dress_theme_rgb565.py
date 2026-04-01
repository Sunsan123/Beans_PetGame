#!/usr/bin/env python3

import argparse
import struct
from pathlib import Path

from PIL import Image


RAW_NAME = "000_theme_preview.rgb565"
RAW_MAGIC = b"DTH1"


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def composite_over_white(r: int, g: int, b: int, a: int) -> tuple[int, int, int]:
    if a >= 255:
        return r, g, b
    if a <= 0:
        return 255, 255, 255
    inv = 255 - a
    return (
        (r * a + 255 * inv) // 255,
        (g * a + 255 * inv) // 255,
        (b * a + 255 * inv) // 255,
    )


def convert_png_to_rgb565(src_path: Path) -> Path:
    dst_path = src_path.with_name(RAW_NAME)
    with Image.open(src_path) as im:
        rgba = im.convert("RGBA")
        w, h = rgba.size
        payload = bytearray()
        for r, g, b, a in rgba.getdata():
            rr, gg, bb = composite_over_white(r, g, b, a)
            payload += struct.pack("<H", rgb888_to_rgb565(rr, gg, bb))

    dst_path.write_bytes(RAW_MAGIC + struct.pack("<HH", w, h) + payload)
    return dst_path


def build_theme_previews(root: Path) -> int:
    count = 0
    for src_path in root.rglob("000_*.png"):
        convert_png_to_rgb565(src_path)
        count += 1
    return count


def main() -> int:
    parser = argparse.ArgumentParser(description="Build RGB565 dress theme previews next to 000_*.png files.")
    parser.add_argument("root", type=Path, help="Theme root folder to scan recursively.")
    args = parser.parse_args()

    root = args.root.resolve()
    if not root.exists():
        raise SystemExit(f"Theme root not found: {root}")

    count = build_theme_previews(root)
    print(f"Built {count} RGB565 theme previews in {root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
