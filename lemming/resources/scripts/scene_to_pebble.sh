#!/usr/bin/env bash
# Convert a generated full-bleed scene (watchface background) into Pebble assets.
#
#   ./scene_to_pebble.sh <input.png> <name> [--crop-w N] [--cut Y,H] [--sky HEX]
#                                           [--palette "HEX HEX ..."]
#
# Unlike to_pebble.sh (which keys the background out for sprites), this keeps the scene
# opaque and edge-to-edge:
#
#   --crop-w  centre-crop the source to N px wide before scaling. Cropping in lets the
#             hero feature (the clock) occupy more of the final frame; the facade's outer
#             edges are flat stone, so nothing is lost.
#   --cut Y,H splice H rows out of the source starting at row Y. The column shafts are
#             uniform vertically, so removing a slab of them is invisible and buys the
#             height the crop costs.
#   --sky     recolour the flat background (sampled at the top-left corner) to this hex.
#             The generated navy sits almost exactly between #000055 and #555555, and
#             nearest-neighbour — in RGB *or* Lab — picks the grey. Only the sky needs
#             this; every other source colour lands on the right palette entry.
#
# Scaling is to the target WIDTH, never a cover-crop: cropping to 144x168's aspect clips
# the ends off "LEMMING BROTHERS". The leftover height is padded with sky on top and
# plaza colour underneath — the bottom padding deepens the strip the lemmings walk along.
#
# The palette is a CURATED subset of Pebble's RGB222. Remapping to all 64 entries sends
# near-neutral stone to #FFAAAA (pink). Every colour listed is still RGB222-legal.
set -euo pipefail

src=""; name=""; crop_w=0; cut_y=0; cut_h=0; sky_to=""
colors=(000000 000055 555555 AAAAAA FFFFFF 0055AA FFAA00 AA5500)

while [ $# -gt 0 ]; do
  case "$1" in
    --crop-w)  crop_w="$2"; shift 2 ;;
    --cut)     cut_y="${2%%,*}"; cut_h="${2##*,}"; shift 2 ;;
    --sky)     sky_to="$2"; shift 2 ;;
    --palette) read -r -a colors <<<"$2"; shift 2 ;;
    -*) echo "unknown flag: $1" >&2; exit 1 ;;
    *) if [ -z "$src" ]; then src="$1"; elif [ -z "$name" ]; then name="$1";
       else echo "unexpected arg: $1" >&2; exit 1; fi; shift ;;
  esac
done
[ -n "$src" ] && [ -n "$name" ] || { echo "usage: scene_to_pebble.sh <input.png> <name> [flags]" >&2; exit 1; }

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
out="$here/../images"
pal="$(mktemp -t pal.XXXX).png"
tmp="$(mktemp -t scene.XXXX).png"
trap 'rm -f "$pal" "$tmp"' EXIT

swatches=(); for c in "${colors[@]}"; do
  [ "${#c}" -eq 6 ] || { echo "bad hex: $c" >&2; exit 1; }
  swatches+=("xc:#$c")
done
magick "${swatches[@]}" +append "$pal"

read -r W H <<<"$(magick identify -format '%w %h' "$src")"
sky="$(magick "$src" -format '%[pixel:p{2,2}]' info:)"

# Splice out the column-shaft slab, then centre-crop to width.
if [ "$cut_h" -gt 0 ]; then
  magick \
    \( "$src" -crop "${W}x${cut_y}+0+0" +repage \) \
    \( "$src" -crop "${W}x$((H - cut_y - cut_h))+0+$((cut_y + cut_h))" +repage \) \
    -append "$tmp"
else
  magick "$src" "$tmp"
fi
if [ "$crop_w" -gt 0 ] && [ "$crop_w" -lt "$W" ]; then
  magick "$tmp" -gravity center -crop "${crop_w}x+0+0" +repage "$tmp"
fi
if [ -n "$sky_to" ]; then
  magick "$tmp" -fuzz 12% -fill "#$sky_to" -opaque "$sky" "$tmp"
  sky="#$sky_to"
fi

read -r CW CH <<<"$(magick identify -format '%w %h' "$tmp")"
plaza="$(magick "$tmp" -format "%[pixel:p{2,$((CH - 3))}]" info:)"

for dim in 200x228 144x168; do
  tw="${dim%x*}"; th="${dim#*x}"
  sh=$(( (CH * tw + CW / 2) / CW ))        # height after scaling to width tw
  pad=$(( th - sh ))
  if [ "$pad" -lt 0 ]; then top=0; bot=0; else top=$(( pad / 4 )); bot=$(( pad - top )); fi
  magick "$tmp" \
    -filter point -resize "${tw}x" \
    -background "$sky"   -gravity north -splice "0x${top}" \
    -background "$plaza" -gravity south -splice "0x${bot}" \
    -gravity center -crop "${dim}+0+0" +repage \
    -dither None -remap "$pal" \
    -define png:color-type=6 \
    "$out/${name}_${dim}.png"
  magick identify -format '  %f: %wx%h colors=%k\n' "$out/${name}_${dim}.png"
done
