#!/usr/bin/env bash
# Convert a generated character sprite into Pebble watchface assets.
#
#   ./to_pebble.sh <input.png> <name> [bg-hex]
#
# Produces resources/images/<name>_200x228.png (Emery) and
#          resources/images/<name>_144x168.png (Basalt/Chalk-ish)
# keyed to transparent, nearest-neighbour scaled, and quantised to the
# Pebble RGB222 64-colour palette (each channel one of 00/55/AA/FF).
set -euo pipefail

src="${1:?usage: to_pebble.sh <input.png> <name> [bg-hex]}"
name="${2:?usage: to_pebble.sh <input.png> <name> [bg-hex]}"
bg="${3:-}"

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
out="$here/../images"
pal="$(mktemp -t pebble64XXXX).png"
tmp="$(mktemp -t sprite.XXXX).png"
trap 'rm -f "$pal" "$tmp"' EXIT

# 64-colour RGB222 palette as a 64x1 swatch strip.
levels=(00 55 AA FF)
swatches=()
for r in "${levels[@]}"; do for g in "${levels[@]}"; do for b in "${levels[@]}"; do
  swatches+=("xc:#$r$g$b")
done; done; done
magick "${swatches[@]}" +append "$pal"

# Key out the flat background. Default: flood-fill from the top-left corner, so
# only the connected background goes transparent and interior cream/white stays.
# Pass an explicit hex as $3 to key that colour globally instead.
if [ -n "$bg" ]; then
  magick "$src" -alpha set -fuzz 12% -transparent "$bg" -trim +repage "$tmp"
else
  magick "$src" -alpha set -fuzz 12% -fill none -draw 'alpha 2,2 floodfill' \
    -trim +repage "$tmp"
fi

for dim in 200x228 144x168; do
  magick "$tmp" \
    -filter point -resize "${dim}" \
    -background none -gravity center -extent "${dim}" \
    -dither None -remap "$pal" \
    -define png:color-type=6 \
    "$out/${name}_${dim}.png"
  magick identify -format '  %f: %wx%h colors=%k\n' "$out/${name}_${dim}.png"
done
