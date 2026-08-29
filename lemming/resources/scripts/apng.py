#!/usr/bin/env python3
"""Assemble PNG frames into a palettized APNG that Pebble can actually decode.

    apng.py <out.apng> <delay_ms> <frame.png> [frame.png ...]

ImageMagick writes APNG only as colour-type 6 (RGBA truecolour) and ignores
-type/-define png:color-type. Pebble's on-watch PNG decoder reads palettized and
grayscale PNGs only, so an ImageMagick APNG loads as nothing. The working APNGs in
meow-o-clock/resources are colour-type 3 + tRNS, which is what this writes.

Frames are read as raw RGBA via ImageMagick, so this needs no Python imaging library —
only what devenv already provides. Every distinct RGBA value becomes one palette entry,
with all fully-transparent pixels folded onto index 0, so the conversion is exact rather
than a requantisation: the sprite is already down to a handful of RGB222 colours.
"""
import struct
import subprocess
import sys
import zlib


def chunk(tag: bytes, data: bytes) -> bytes:
    return (struct.pack(">I", len(data)) + tag + data
            + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))


def read_rgba(path: str):
    out = subprocess.run(
        ["magick", path, "-depth", "8", "-alpha", "on", "RGBA:-"],
        check=True, capture_output=True).stdout
    w, h = subprocess.run(
        ["magick", "identify", "-format", "%w %h", path],
        check=True, capture_output=True, text=True).stdout.split()
    w, h = int(w), int(h)
    if len(out) != w * h * 4:
        raise SystemExit(f"{path}: expected {w * h * 4} RGBA bytes, got {len(out)}")
    return w, h, out


def main() -> None:
    if len(sys.argv) < 4:
        raise SystemExit(__doc__.strip().splitlines()[2].strip())
    out_path, delay_ms, frame_paths = sys.argv[1], int(sys.argv[2]), sys.argv[3:]

    frames = [read_rgba(p) for p in frame_paths]
    w, h = frames[0][0], frames[0][1]
    for p, (fw, fh, _) in zip(frame_paths, frames):
        if (fw, fh) != (w, h):
            raise SystemExit(f"{p}: {fw}x{fh} does not match {w}x{h}")

    # Index 0 is the single transparent entry; every fully-transparent pixel folds onto
    # it regardless of the RGB left behind under the alpha.
    palette = [(0, 0, 0, 0)]
    index = {(0, 0, 0, 0): 0}
    indexed = []
    for _, _, rgba in frames:
        rows = []
        for y in range(h):
            row = bytearray()
            base = y * w * 4
            for x in range(w):
                px = rgba[base + x * 4: base + x * 4 + 4]
                key = (0, 0, 0, 0) if px[3] == 0 else (px[0], px[1], px[2], 255)
                i = index.get(key)
                if i is None:
                    i = index[key] = len(palette)
                    palette.append(key)
                row.append(i)
            rows.append(bytes(row))
        indexed.append(rows)
    if len(palette) > 256:
        raise SystemExit(f"{len(palette)} colours; a colour-type 3 PNG holds 256")

    def idat(rows):
        # Filter type 0 (None) on every scanline: the images are flat colour blocks, so
        # the predictors buy nothing and None keeps the decoder cheap.
        return zlib.compress(b"".join(b"\x00" + r for r in rows), 9)

    parts = [b"\x89PNG\r\n\x1a\n",
             chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 3, 0, 0, 0)),
             chunk(b"acTL", struct.pack(">II", len(frames), 0)),
             chunk(b"PLTE", b"".join(bytes(c[:3]) for c in palette)),
             chunk(b"tRNS", b"\x00")]

    seq = 0
    for n, rows in enumerate(indexed):
        # dispose NONE / blend SOURCE: each frame is full-size and fully opaque about
        # its own alpha, so nothing needs compositing against what came before.
        parts.append(chunk(b"fcTL", struct.pack(
            ">IIIIIHHBB", seq, w, h, 0, 0, delay_ms, 1000, 0, 0)))
        seq += 1
        if n == 0:
            parts.append(chunk(b"IDAT", idat(rows)))
        else:
            parts.append(chunk(b"fdAT", struct.pack(">I", seq) + idat(rows)))
            seq += 1
    parts.append(chunk(b"IEND", b""))

    with open(out_path, "wb") as fh:
        fh.write(b"".join(parts))
    print(f"  {out_path}: {w}x{h} x {len(frames)} frames, "
          f"{len(palette)} colours, {sum(len(p) for p in parts)} B")


if __name__ == "__main__":
    main()
