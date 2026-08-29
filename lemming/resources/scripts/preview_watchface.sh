#!/usr/bin/env bash
# Render the finished watchface as an animated GIF, for review outside the emulator.
#
#   ./resources/scripts/preview_watchface.sh [HH:MM] [frames] [delay_cs]
#
# Composites, per display size:
#   - the static bank background
#   - procedural analog hands on the blank dial, at HH:MM
#   - the walk-cycle strip, translated left-to-right across the plaza
#
# This mirrors exactly what src/c/lemming-bank.c does at runtime, so it doubles as a
# check on the layout constants (dial centre, hand lengths, plaza baseline) before they
# are compiled in. Writes resources/images/lemming_bank_preview_<WxH>.gif.
set -euo pipefail

time_str="${1:-10:09}"
frames="${2:-50}"
delay="${3:-8}"                       # centiseconds; 8 => 80ms => 4.0s over 50 frames
hh="${time_str%%:*}"; mm="${time_str##*:}"

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
img="$here/../images"
work="$(mktemp -d -t preview.XXXX)"
trap 'rm -rf "$work"' EXIT

#     display   dial cx,cy  dial r  plaza baseline (sprite bottom edge)
for spec in 200x228:99:55:27:228 144x168:71:40:19:168; do
  IFS=: read -r dim cx cy r base <<<"$spec"
  W="${dim%x*}"; H="${dim#*x}"
  bg="$img/lemming_bank_${dim}.png"
  walk="$img/lemmings_walk_${dim}.gif"
  [ -f "$bg" ] && [ -f "$walk" ] || { echo "missing assets for $dim" >&2; exit 1; }

  rm -f "$work"/w_*.png "$work"/p_*.png
  magick "$walk" -coalesce +adjoin "$work/w_%d.png"
  nwalk="$(ls "$work"/w_*.png | wc -l | tr -d ' ')"
  read -r SW SH <<<"$(magick identify -format '%w %h' "$work/w_0.png")"

  # Hand endpoints. awk for the trig; +antialias keeps the strokes hard-edged.
  read -r hx hy mx my <<<"$(awk -v cx="$cx" -v cy="$cy" -v r="$r" -v hh="$hh" -v mm="$mm" '
    BEGIN {
      pi = atan2(0, -1)
      ha = ((hh % 12) * 30 + mm * 0.5) * pi / 180
      ma = (mm * 6) * pi / 180
      hl = r * 0.55; ml = r * 0.85
      printf "%d %d %d %d\n", cx + hl*sin(ha), cy - hl*cos(ha), cx + ml*sin(ma), cy - ml*cos(ma)
    }')"

  for i in $(seq 0 $((frames - 1))); do
    # Enter fully off the left edge, exit fully off the right: the group is on screen
    # for the whole clip and never pops in or out.
    x=$(( -SW + i * (W + SW) / (frames - 1) ))
    y=$(( base - SH ))
    magick "$bg" \
      +antialias -stroke '#000000' -strokewidth 3 \
      -draw "line $cx,$cy $hx,$hy" \
      -strokewidth 2 \
      -draw "line $cx,$cy $mx,$my" \
      -stroke none -fill '#000000' \
      -draw "rectangle $((cx-1)),$((cy-1)) $((cx+1)),$((cy+1))" \
      "$work/w_$((i % nwalk)).png" -geometry "+${x}+${y}" -compose over -composite \
      "$work/p_$(printf '%03d' "$i").png"
  done

  magick -delay "$delay" -loop 0 "$work"/p_*.png "$img/lemming_bank_preview_${dim}.gif"
  printf '  lemming_bank_preview_%s.gif: %s frames @ %sms (%.1fs, %s B)\n' \
    "$dim" "$frames" "$((delay * 10))" \
    "$(awk -v f="$frames" -v d="$delay" 'BEGIN{print f*d/100}')" \
    "$(wc -c < "$img/lemming_bank_preview_${dim}.gif" | tr -d ' ')"
done
