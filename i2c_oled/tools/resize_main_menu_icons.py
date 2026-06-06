#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


SRC = Path(__file__).resolve().parents[1] / "main" / "wououi_128_64.cpp"
ICON_NAMES = [
    "likes_icon_pic",
    "collection_icon_pic",
    "slot_icon_pic",
    "share_icon_pic",
]

SRC_W = 30
SRC_H = 30
TARGET_W = 26
TARGET_H = 26
CANVAS_W = 30
CANVAS_H = 30


def parse_bytes(block: str) -> list[int]:
    return [int(token, 0) for token in re.findall(r"0x[0-9A-Fa-f]+|\d+", block)]


def unpack_bits(data: list[int], width: int, height: int) -> list[list[int]]:
    stride = (width + 7) // 8
    bits = [[0] * width for _ in range(height)]
    for y in range(height):
        for x in range(width):
            byte = data[y * stride + x // 8]
            bits[y][x] = (byte >> (x % 8)) & 1
    return bits


def pack_bits(bits: list[list[int]]) -> list[int]:
    height = len(bits)
    width = len(bits[0]) if height else 0
    stride = (width + 7) // 8
    out: list[int] = []
    for y in range(height):
        row = [0] * stride
        for x in range(width):
            if bits[y][x]:
                row[x // 8] |= 1 << (x % 8)
        out.extend(row)
    return out


def bbox(bits: list[list[int]]) -> tuple[int, int, int, int] | None:
    xs: list[int] = []
    ys: list[int] = []
    for y, row in enumerate(bits):
        for x, value in enumerate(row):
            if value:
                xs.append(x)
                ys.append(y)
    if not xs:
        return None
    return min(xs), min(ys), max(xs), max(ys)


def crop(bits: list[list[int]], rect: tuple[int, int, int, int]) -> list[list[int]]:
    x0, y0, x1, y1 = rect
    return [row[x0 : x1 + 1] for row in bits[y0 : y1 + 1]]


def resize_nn(bits: list[list[int]], out_w: int, out_h: int) -> list[list[int]]:
    src_h = len(bits)
    src_w = len(bits[0]) if src_h else 0
    out = [[0] * out_w for _ in range(out_h)]
    if src_w == 0 or src_h == 0:
        return out

    for y in range(out_h):
        sy = min(src_h - 1, int((y + 0.5) * src_h / out_h))
        for x in range(out_w):
            sx = min(src_w - 1, int((x + 0.5) * src_w / out_w))
            out[y][x] = bits[sy][sx]
    return out


def scale_visible_mask(data: list[int]) -> list[int]:
    bits = unpack_bits(data, SRC_W, SRC_H)
    visible = [[1 - bit for bit in row] for row in bits]
    rect = bbox(visible)
    if rect is None:
        return data

    cropped = crop(visible, rect)
    crop_h = len(cropped)
    crop_w = len(cropped[0]) if crop_h else 0
    if crop_w == 0 or crop_h == 0:
        return data

    scale = min(TARGET_W / crop_w, TARGET_H / crop_h)
    out_w = max(1, round(crop_w * scale))
    out_h = max(1, round(crop_h * scale))
    resized = resize_nn(cropped, out_w, out_h)

    canvas = [[1] * CANVAS_W for _ in range(CANVAS_H)]
    off_x = (CANVAS_W - out_w) // 2
    off_y = (CANVAS_H - out_h) // 2
    for y in range(out_h):
        for x in range(out_w):
            if resized[y][x]:
                canvas[off_y + y][off_x + x] = 0

    return pack_bits(canvas)


def format_bytes(data: list[int]) -> str:
    lines: list[str] = []
    for i in range(0, len(data), 8):
        chunk = data[i : i + 8]
        lines.append("  " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    return "\n".join(lines)


def main() -> None:
    text = SRC.read_text()
    for name in ICON_NAMES:
        pattern = rf"(PROGMEM const uint8_t {name}\[\] =\n\{{\n)(.*?)(\n\}};)"
        match = re.search(pattern, text, re.S)
        if not match:
            raise SystemExit(f"Could not find icon block: {name}")
        data = parse_bytes(match.group(2))
        if len(data) != 120:
            raise SystemExit(f"{name} expected 120 bytes, got {len(data)}")
        new_data = scale_visible_mask(data)
        replacement = match.group(1) + format_bytes(new_data) + match.group(3)
        text = text[: match.start()] + replacement + text[match.end() :]

    SRC.write_text(text)


if __name__ == "__main__":
    main()
