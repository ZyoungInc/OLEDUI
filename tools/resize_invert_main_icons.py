#!/usr/bin/env python3
from __future__ import annotations

import math
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "i2c_oled" / "main" / "wououi_128_64.cpp"

ICONS = {
    "likes_icon_pic": (36, 35),
    "collection_icon_pic": (36, 37),
    "slot_icon_pic": (36, 36),
    "share_icon_pic": (36, 36),
}

TARGET_W = 30
TARGET_H = 30


def parse_bytes(block: str) -> list[int]:
    return [int(token, 16) for token in re.findall(r"0x([0-9A-Fa-f]{2})", block)]


def decode_xbm(data: list[int], width: int, height: int) -> list[list[int]]:
    row_bytes = (width + 7) // 8
    pixels = [[0 for _ in range(width)] for _ in range(height)]
    for y in range(height):
        for x in range(width):
            byte = data[y * row_bytes + (x // 8)]
            pixels[y][x] = (byte >> (x % 8)) & 1
    return pixels


def resize_fit_nn(src: list[list[int]], dst_w: int, dst_h: int) -> list[list[int]]:
    src_h = len(src)
    src_w = len(src[0]) if src else 0
    if src_w == 0 or src_h == 0:
        return [[0 for _ in range(dst_w)] for _ in range(dst_h)]

    scale = min(dst_w / src_w, dst_h / src_h)
    scaled_w = max(1, min(dst_w, int(round(src_w * scale))))
    scaled_h = max(1, min(dst_h, int(round(src_h * scale))))
    off_x = (dst_w - scaled_w) // 2
    off_y = (dst_h - scaled_h) // 2

    dst = [[0 for _ in range(dst_w)] for _ in range(dst_h)]
    for y in range(scaled_h):
        sy = min(src_h - 1, int(y * src_h / scaled_h))
        for x in range(scaled_w):
            sx = min(src_w - 1, int(x * src_w / scaled_w))
            dst[off_y + y][off_x + x] = src[sy][sx]
    return dst


def invert(pixels: list[list[int]]) -> list[list[int]]:
    return [[1 - px for px in row] for row in pixels]


def encode_xbm(pixels: list[list[int]]) -> list[int]:
    height = len(pixels)
    width = len(pixels[0]) if pixels else 0
    row_bytes = (width + 7) // 8
    out = [0 for _ in range(row_bytes * height)]
    for y in range(height):
        for x in range(width):
            if pixels[y][x]:
                out[y * row_bytes + (x // 8)] |= 1 << (x % 8)
    return out


def format_c_array(name: str, data: list[int], width: int, height: int) -> str:
    lines = []
    lines.append(f"PROGMEM const uint8_t {name}[] =")
    lines.append("{")
    for i in range(0, len(data), 8):
        chunk = ", ".join(f"0x{b:02X}" for b in data[i : i + 8])
        suffix = "," if i + 8 < len(data) else ""
        lines.append(f"  {chunk}{suffix}")
    lines.append("};")
    lines.append("")
    lines.append(f"/* ({width} X {height}) */")
    return "\n".join(lines)


def replace_block(text: str, name: str, replacement: str) -> str:
    pattern = re.compile(
        rf"PROGMEM const uint8_t {re.escape(name)}\[\]\s*=\s*\{{.*?\n\}};",
        re.S,
    )
    if not pattern.search(text):
        raise RuntimeError(f"icon block not found: {name}")
    return pattern.sub(replacement, text, count=1)


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")

    for name, (src_w, src_h) in ICONS.items():
        match = re.search(
            rf"PROGMEM const uint8_t {re.escape(name)}\[\]\s*=\s*\{{(.*?)\n\}};",
            text,
            re.S,
        )
        if not match:
            raise RuntimeError(f"failed to locate {name}")

        data = parse_bytes(match.group(1))
        expected_len = ((src_w + 7) // 8) * src_h
        if len(data) < expected_len:
            data.extend([0] * (expected_len - len(data)))
        elif len(data) > expected_len:
            data = data[:expected_len]
        src_pixels = decode_xbm(data, src_w, src_h)
        dst_pixels = invert(resize_fit_nn(src_pixels, TARGET_W, TARGET_H))
        dst_data = encode_xbm(dst_pixels)
        text = replace_block(text, name, format_c_array(name, dst_data, TARGET_W, TARGET_H))

    text = text.replace("{ likes_icon_pic, 36, 35 },", "{ likes_icon_pic, 30, 30 },")
    text = text.replace("{ collection_icon_pic, 36, 37 },", "{ collection_icon_pic, 30, 30 },")
    text = text.replace("{ slot_icon_pic, 36, 36 },", "{ slot_icon_pic, 30, 30 },")
    text = text.replace("{ share_icon_pic, 36, 36 },", "{ share_icon_pic, 30, 30 },")

    TARGET.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
