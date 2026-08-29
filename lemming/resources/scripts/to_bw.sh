#!/usr/bin/env bash
# Derive the 1-bit (diorite/flint) assets from the composed 144x168 colour ones.
#
#   ./resources/scripts/to_bw.sh
#
# Emits:
#   resources/images/lemming_bank_bw_144x168.png     background, 2 colours
#   resources/images/lemmings_walk_bw_f<0-5>_144x168.png   walk cycle, what the app loads
#   resources/images/lemmings_walk_bw_144x168.gif          walk cycle, preview only
#
# Derived from the COMPOSED art, never re-generated from the raw renders: config.h's
# DIAL_CX/CY/R and WALK_W/H were measured off those exact files, so anything that
# changes framing or scale silently invalidates them. Every operation here is a
# per-pixel colour substitution — geometry is untouched by construction.
#
# The mapping is spelled out colour by colour rather than thresholded on luminance,
# because luminance gets three of these decisions wrong:
#
#   * #AAAAAA stone (the facade, and the largest single area) and #555555 (the sign
#     band, the column shafts) sit either side of mid-grey. A threshold splits them
#     the right way by luck, not by intent, and any re-render moves them.
#   * #FFAA00 gold is bright enough to threshold WHITE, which is what the sign
#     lettering needs — but the same gold is the clock's rim, where white would melt
#     it into the white dial. The rim survives because the art already carries a
#     black outline ring just outside it; that ring is what reads as the bezel once
#     the gold goes white.
#   * #000055 navy sky thresholds black, which is right, but only because the facade
#     goes white — the roofline is the one edge in the picture with no outline of its
#     own, so those two must land on opposite values.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
out="$here/../images"
work="$(mktemp -d -t bw.XXXX)"
trap 'rm -rf "$work"' EXIT

# --- background -------------------------------------------------------------------
# BLACK: sky, outlines, sign-band ground, column shafts.
# WHITE: stone facade, dial, plaza, roof highlight, gold (lettering + rim).
BANK_BLACK=(000000 000055 555555 AA5500)
BANK_WHITE=(AAAAAA FFFFFF FFAA00)

bank_ops=(-fuzz 0)
for c in "${BANK_BLACK[@]}"; do bank_ops+=(-fill black -opaque "#$c"); done
for c in "${BANK_WHITE[@]}"; do bank_ops+=(-fill white -opaque "#$c"); done

magick "$out/lemming_bank_144x168.png" -alpha off "${bank_ops[@]}" \
  -type Bilevel \
  "$out/lemming_bank_bw_144x168.png"

# --- walk cycle -------------------------------------------------------------------
# The lemmings cross a WHITE plaza, so they have to read as silhouettes: everything
# structural goes black and only the shirt front, face patch and paws stay white, held
# apart from the plaza by the black outline that already surrounds them.
WALK_BLACK=(000000 000055 555555 AAAAAA AA5500 FF0000)
WALK_WHITE=(FFFFFF FFFFAA FFAA55 FFAA00)

walk_ops=(-fuzz 0)
for c in "${WALK_BLACK[@]}"; do walk_ops+=(-fill black -opaque "#$c"); done
for c in "${WALK_WHITE[@]}"; do walk_ops+=(-fill white -opaque "#$c"); done

# Six separate frames, not an APNG, because 1-bit platforms cannot render one. The
# sequence decoder does read the APNG on diorite/flint — it fills the target bitmap
# without complaint — but drawing that bitmap then faults, as observed with a plain
# 1-bit blank target under both GCompOpSet and GCompOpAssign. A plain `bitmap` resource
# sidesteps the on-watch decoder entirely: the SDK converts each PNG to the platform's
# native 1-bit form at BUILD time. walk_anim.c cycles through them by hand.
#
# Grayscale + alpha at depth 1 is the format meow-o-clock's aplite frames already use
# and the one the resource compiler turns into a transparent 1-bit pbi, which is what
# makes GCompOpSet honour the cut-out instead of drawing an opaque block.
#
# Alpha is carried separately through the recolour: -opaque works on the colour plane,
# and folding the two back together with CopyOpacity is what keeps the keyed-out
# background transparent instead of snapping it to whichever of black/white is nearer.
frames=()
n=0
while [ "$n" -lt 6 ]; do
  f="$(printf '%s/f%d.png' "$work" "$n")"
  magick "$out/lemmings_walk_144x168.gif[$n]" -coalesce \
    \( +clone -alpha extract -threshold 50% -write "$work/a$n.png" +delete \) \
    -alpha off "${walk_ops[@]}" \
    "$work/a$n.png" -alpha off -compose CopyOpacity -composite \
    -type GrayscaleAlpha -depth 1 \
    "$f"
  cp "$f" "$out/lemmings_walk_bw_f${n}_144x168.png"
  frames+=("$f")
  n=$((n + 1))
done

# Preview only — nothing loads this on watch, it is the B/W counterpart of the colour
# walk GIF that sits beside it in resources/images.
magick -delay 8 -loop 0 -dispose background "${frames[@]}" \
  -define png:color-type=6 "$out/lemmings_walk_bw_144x168.gif"

magick identify -format '  %f: %wx%h colors=%k %B B\n' "$out/lemming_bank_bw_144x168.png"
for n in 0 1 2 3 4 5; do
  magick identify -format '  %f: %wx%h %B B\n' "$out/lemmings_walk_bw_f${n}_144x168.png"
done

# The whole point is two values plus transparency; anything else means a colour was
# missed above and would come out as an arbitrary grey once the SDK quantises it.
for f in lemming_bank_bw_144x168.png lemmings_walk_bw_f0_144x168.png; do
  bad="$(magick "$out/$f" -alpha off -format '%c' histogram:info: \
         | grep -oE '#[0-9A-F]{6}' | sort -u | grep -vE '#(000000|FFFFFF)' || true)"
  [ -z "$bad" ] || { echo "$f: non-monochrome colours left: $bad" >&2; exit 1; }
done
