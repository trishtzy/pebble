#!/usr/bin/env bash
# Turn a generated walk-cycle video into a Pebble sprite animation.
#
#   ./resources/scripts/walk_to_pebble.sh <video.mp4> <name> [--start N] [--step N]
#              [--count N] [--key HEX] [--fuzz N] [--filter Box|Point]
#
# Emits, for each display size, a transparent walk-cycle strip:
#   resources/images/<name>_200x228.gif   deliverable / preview  (GIF, transparent)
#   resources/images/<name>_200x228.png   what the app loads     (APNG, GBitmapSequence)
#   resources/images/<name>_144x168.{gif,png}
#
# The WxH suffix is the DISPLAY the strip is cut for, matching the rest of resources/;
# the strip itself is only ~97x34 (200x228) / ~68x24 (144x168).
#
# Colour handling is a deliberate two-step, because a single nearest-neighbour remap to
# RGB222 is wrong in three places: the navy suit lands on grey (colliding with the head
# cap), the gold briefcase lands on beige (colliding with the ears), and the red tie
# lands on brown. Fuzzy -opaque overrides do not fix it either — the suit and the cap are
# only 9% apart in RGB, so any fuzz wide enough to catch the suit's video noise also eats
# the cap. So instead:
#
#   1. at FULL resolution, remap to the video's OWN flat source colours (SRC below).
#      This absorbs the video model's compression noise and leaves exactly ten values.
#   2. substitute each of those ten for its chosen RGB222 value at fuzz 0 — exact
#      matches only, no bleeding, every art decision made explicitly.
#   3. only then downscale and re-snap to the RGB222 palette.
#
# --start/--step/--count select the loop. The generated cycle has a period of 12 frames
# at 24fps, so the default 6 frames at step 2 is exactly one stride, played at ~12fps.
set -euo pipefail

src=""; name=""; start=25; step=2; count=6; key="#22F214"; fuzz=30; filt=Box
while [ $# -gt 0 ]; do
  case "$1" in
    --start) start="$2"; shift 2 ;;
    --step)  step="$2";  shift 2 ;;
    --count) count="$2"; shift 2 ;;
    --key)   key="$2";   shift 2 ;;
    --fuzz)  fuzz="$2";  shift 2 ;;
    --filter) filt="$2"; shift 2 ;;
    -*) echo "unknown flag: $1" >&2; exit 1 ;;
    *) if [ -z "$src" ]; then src="$1"; elif [ -z "$name" ]; then name="$1";
       else echo "unexpected arg: $1" >&2; exit 1; fi; shift ;;
  esac
done
[ -n "$src" ] && [ -n "$name" ] || { echo "usage: walk_to_pebble.sh <video.mp4> <name> [flags]" >&2; exit 1; }

# source colour -> RGB222 colour. Left column is measured from the generated video.
MAP=(
  0C0D0F:000000   # outline
  293655:000055   # navy suit jacket
  F7F1E0:FFFFFF   # cream face patch / white shirt
  3E4143:555555   # dark charcoal head cap
  444344:555555   # head cap, shaded
  ADA79B:AAAAAA   # grey shading
  E4D3B1:FFFFAA   # beige ear / paws / feet
  CBBC9E:FFAA55   # beige, shaded
  D4A72C:FFAA00   # gold briefcase
  AA3D32:FF0000   # red necktie
  A65230:AA5500   # warm orange fur at the back
)

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
out="$here/../images"
work="$(mktemp -d -t walk.XXXX)"
trap 'rm -rf "$work"' EXIT

srcpal="$work/srcpal.png"; dstpal="$work/dstpal.png"
s_sw=(); d_sw=()
for m in "${MAP[@]}"; do s_sw+=("xc:#${m%%:*}"); d_sw+=("xc:#${m##*:}"); done
magick "${s_sw[@]}" +append "$srcpal"
magick "${d_sw[@]}" +append "$dstpal"

mkdir -p "$work/raw" "$work/rgb" "$work/mask"
ffmpeg -v error -i "$src" -fps_mode passthrough "$work/raw/f%03d.png"

# Exact (fuzz 0) source -> RGB222 substitutions. Applied at full resolution, where the
# colours are still clean and flat.
recolor=(-fuzz 0)
for m in "${MAP[@]}"; do recolor+=(-fill "#${m##*:}" -opaque "#${m%%:*}"); done

# Alpha and colour are carried SEPARATELY from here on. -remap discards the alpha
# channel, so keying first and remapping second silently returns an opaque frame with the
# background green snapped to whichever art colour is nearest.
sel=()
for i in $(seq 0 $((count - 1))); do
  n=$(printf '%03d' $((start + i * step)))
  [ -f "$work/raw/f$n.png" ] || { echo "frame $n past end of video" >&2; exit 1; }
  # Silhouette: 1-bit, because Pebble has no partial transparency.
  magick "$work/raw/f$n.png" \
    -fuzz "${fuzz}%" -transparent "$key" \
    -alpha extract -threshold 50% \
    "$work/mask/$n.png"
  # Colour: snap to the video's own flat palette, then to RGB222.
  magick "$work/raw/f$n.png" -alpha off \
    -dither None -remap "$srcpal" \
    "${recolor[@]}" \
    "$work/rgb/$n.png"
  sel+=("$n")
done

# Union bounding box: cropping each frame to its own bbox would make the sprite jitter
# as the limbs swing in and out of it.
read -r X0 Y0 X1 Y1 <<<"$(
  for n in "${sel[@]}"; do
    magick "$work/mask/$n.png" -trim -format '%w %h %X %Y\n' info:
  done | awk '{ x=$3+0; y=$4+0;
                if (NR==1 || x<x0) x0=x; if (NR==1 || y<y0) y0=y;
                if (NR==1 || x+$1>x1) x1=x+$1; if (NR==1 || y+$2>y1) y1=y+$2 }
              END { print x0, y0, x1, y1 }')"
BW=$((X1 - X0)); BH=$((Y1 - Y0))
echo "union bbox: ${BW}x${BH}+${X0}+${Y0}"

for spec in 200x228:34 144x168:24; do
  dim="${spec%%:*}"; h="${spec##*:}"
  w=$(( (BW * h + BH / 2) / BH ))
  rm -f "$work"/s_*.png
  for n in "${sel[@]}"; do
    # Nearest-neighbour on BOTH channels. A smooth filter on the colour plane blends the
    # keyed-out background across the silhouette edge and fringes the sprite.
    magick \
      \( "$work/rgb/$n.png"  -crop "${BW}x${BH}+${X0}+${Y0}" +repage \
         -filter point -resize "${w}x${h}!" \) \
      \( "$work/mask/$n.png" -crop "${BW}x${BH}+${X0}+${Y0}" +repage \
         -filter point -resize "${w}x${h}!" -threshold 50% \) \
      -alpha off -compose CopyOpacity -composite \
      "$work/s_$n.png"
  done
  magick -delay 8 -loop 0 -dispose background "$work"/s_*.png \
    -define png:color-type=6 "$out/${name}_${dim}.gif"
  # APNG via apng.py, not ImageMagick: IM's APNG coder always writes colour-type 6
  # (RGBA) and ignores -type/-define png:color-type, and Pebble's on-watch PNG decoder
  # reads palettized/grayscale only. See the header of apng.py.
  "$here/apng.py" "$out/${name}_${dim}.png" 80 "$work"/s_*.png >/dev/null
  printf '  %s_%s: %sx%s x %s frames  (gif %s B, apng %s B)\n' \
    "$name" "$dim" "$w" "$h" "$count" \
    "$(wc -c < "$out/${name}_${dim}.gif" | tr -d ' ')" \
    "$(wc -c < "$out/${name}_${dim}.png" | tr -d ' ')"
  # No final remap: every colour is already an exact RGB222 value from the MAP table and
  # point resizing introduces none. Verify rather than assume.
  bad="$(magick "$work/s_${sel[0]}.png" -alpha off -format '%c' histogram:info: \
         | grep -oE '#[0-9A-F]{6}' | sort -u \
         | grep -vE '#(00|55|AA|FF){3}' || true)"
  [ -z "$bad" ] || { echo "non-RGB222 colours leaked: $bad" >&2; exit 1; }
done
