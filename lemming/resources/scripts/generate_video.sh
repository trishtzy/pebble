#!/usr/bin/env bash
# Animate a still pixel-art sprite row into a short clip with a video model.
#
#   ./resources/scripts/generate_video.sh <prompt.md> <name> [duration] [model]
#
# Reads the text under the "## Motion" heading of <prompt.md>, submits it to OpenRouter's
# video endpoint with resources/images/<name>_raw.png pinned as the FIRST FRAME, polls
# until the job completes, and writes resources/video/<name>.mp4.
#
# Pinning the first frame is what keeps the video model on-model: given only text it
# drifts off the palette and the silhouettes within a second.
#
# 720p, no audio. At $0.03/video-second that is ~$0.12 for the default 4s.
# Needs OPENROUTER_API_KEY. Sourced from ./.env at run time if not already exported.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/../.." && pwd)"
promptfile="${1:?usage: generate_video.sh <prompt.md> <name> [duration] [model]}"
name="${2:?usage: generate_video.sh <prompt.md> <name> [duration] [model]}"
duration="${3:-4}"
model="${4:-google/veo-3.1-lite}"

if [ -z "${OPENROUTER_API_KEY:-}" ] && [ -f "$root/.env" ]; then
  set -a; . "$root/.env"; set +a
fi
: "${OPENROUTER_API_KEY:?OPENROUTER_API_KEY not set (export it, or add it to $root/.env)}"

prompt="$(awk '/^## Motion$/{f=1;next} /^## Notes$/{f=0} f' "$promptfile")"
[ -n "$prompt" ] || { echo "could not extract '## Motion' prompt from $promptfile" >&2; exit 1; }

first="$root/resources/images/${name}_raw.png"
[ -f "$first" ] || { echo "missing first frame: $first" >&2; exit 1; }

mkdir -p "$root/resources/video"
out="$root/resources/video/${name}.mp4"
frame="$(mktemp -t frame.XXXX).png"
body="$(mktemp -t vidreq.XXXX)"; resp="$(mktemp -t vidresp.XXXX)"
trap 'rm -f "$frame" "$body" "$resp"' EXIT

# veo accepts 16:9 or 9:16 only; match 1280x720 exactly so nothing is re-letterboxed.
magick "$first" -filter point -resize '1280x720^' -gravity center -extent 1280x720 "$frame"
data="data:image/png;base64,$(base64 < "$frame" | tr -d '\n')"

jq -n --arg m "$model" --arg p "$prompt" --arg u "$data" --argjson d "$duration" \
  '{model:$m, prompt:$p, duration:$d, resolution:"720p", aspect_ratio:"16:9",
    generate_audio:false,
    frame_images:[{type:"image_url", frame_type:"first_frame", image_url:{url:$u}}]}' > "$body"

echo "submitting ${duration}s 720p job to $model ..."
curl -sS -X POST https://openrouter.ai/api/v1/videos \
  -H "Authorization: Bearer $OPENROUTER_API_KEY" \
  -H "Content-Type: application/json" \
  --data-binary @"$body" > "$resp"

id="$(jq -r '.id // empty' "$resp")"
[ -n "$id" ] || { echo "submit failed:" >&2; jq -r '.error.message // .' "$resp" >&2; exit 1; }
poll="$(jq -r '.polling_url // empty' "$resp")"
poll="${poll:-https://openrouter.ai/api/v1/videos/$id}"
echo "job $id"

for _ in $(seq 1 120); do
  sleep 5
  curl -sS "$poll" -H "Authorization: Bearer $OPENROUTER_API_KEY" > "$resp"
  status="$(jq -r '.status // "unknown"' "$resp")"
  echo "  status: $status"
  case "$status" in
    completed) break ;;
    failed|cancelled) jq -r '.error.message // .' "$resp" >&2; exit 1 ;;
  esac
done
[ "$(jq -r '.status' "$resp")" = completed ] || { echo "timed out waiting for $id" >&2; exit 1; }

url="$(jq -r '.unsigned_urls[0] // empty' "$resp")"
[ -n "$url" ] || { echo "no video url in completed job:" >&2; jq . "$resp" >&2; exit 1; }
curl -sSL "$url" -H "Authorization: Bearer $OPENROUTER_API_KEY" -o "$out"

echo "video: $out"
ffprobe -v error -select_streams v:0 \
  -show_entries stream=width,height,nb_frames,r_frame_rate,duration \
  -of default=noprint_wrappers=1 "$out"
