#!/usr/bin/env bash
# Regenerate the lemming v4 8-bit character design and turn it into Pebble assets.
#
#   ./resources/scripts/generate_v4.sh [name]
#
# Reads the prompt from resources/prompts/lemming_v4.md, calls OpenRouter, writes the
# raw PNG to resources/images/<name>_raw.png, then runs to_pebble.sh to emit the
# 200x228 (Emery) and 144x168 assets quantised to the RGB222 64-colour palette.
#
# Needs OPENROUTER_API_KEY. Sourced from ./.env at run time if not already exported.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/../.." && pwd)"
name="${1:-lemming_v4}"
model="google/gemini-3.1-flash-lite-image"

if [ -z "${OPENROUTER_API_KEY:-}" ] && [ -f "$root/.env" ]; then
  set -a; . "$root/.env"; set +a
fi
: "${OPENROUTER_API_KEY:?OPENROUTER_API_KEY not set (export it, or add it to $root/.env)}"

# Prompt = the text between the "## Prompt" and "## Notes" headings.
prompt="$(awk '/^## Prompt$/{f=1;next} /^## Notes$/{f=0} f' "$root/resources/prompts/lemming_v4.md")"
[ -n "$prompt" ] || { echo "could not extract prompt from lemming_v4.md" >&2; exit 1; }

raw="$root/resources/images/${name}_raw.png"
body="$(mktemp -t orreq.XXXX)"; resp="$(mktemp -t orresp.XXXX)"
trap 'rm -f "$body" "$resp"' EXIT

jq -n --arg m "$model" --arg p "$prompt" \
  '{model:$m, modalities:["image","text"], messages:[{role:"user",content:$p}]}' > "$body"

echo "generating with $model ..."
curl -sS https://openrouter.ai/api/v1/chat/completions \
  -H "Authorization: Bearer $OPENROUTER_API_KEY" \
  -H "Content-Type: application/json" \
  --data-binary @"$body" > "$resp"

if ! jq -e '.choices[0].message.images[0]' "$resp" >/dev/null 2>&1; then
  echo "no image in response:" >&2; jq -r '.error.message // .' "$resp" >&2; exit 1
fi

jq -r '.choices[0].message.images[0].image_url.url' "$resp" \
  | sed 's|^data:image/[a-z]*;base64,||' \
  | base64 -d > "$raw"

echo "raw image: $raw ($(magick identify -format '%wx%h' "$raw"))"
"$here/to_pebble.sh" "$raw" "$name"
