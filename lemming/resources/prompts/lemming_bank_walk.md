# lemming_bank_walk — three banker lemmings, walk cycle (watchface animation layer)

Two stages:

1. **First frame** — a still of the three characters, generated with
   `google/gemini-3.1-flash-lite-image` at 16:9. This is the style anchor; it locks the
   palette and the silhouettes so the video model cannot drift off model.
2. **Motion** — that still is passed to `google/veo-3.1-lite` as `first_frame`, with the
   `## Motion` prompt below. 4s, 720p, 16:9, no audio (~$0.12/generation).

The video is post-processed by `walk_to_pebble.sh`: chroma-key the green, crop to the
band, nearest-neighbour downscale, remap to RGB222, emit GIF (deliverable) + APNG (what
`GBitmapSequence` actually loads on Emery/Basalt).

Green `#00FF00` background, not transparent: the video model cannot emit an alpha
channel, and pure green appears nowhere in the character palette, so a global chroma-key
is exact. Flood-filling from a corner instead would leak through the gaps between the
characters' legs.

## Prompt

8-bit pixel art sprite row of THREE chubby lemming bank clerks walking in a line, in the style of a 1990s handheld video-game sprite sheet. Drawn on a coarse low-resolution grid, then upscaled with hard nearest-neighbour edges so every single pixel is a large crisp visible square. No blur, no anti-aliasing, no soft edges anywhere. Wide landscape frame.

CHARACTERS (three identical plush lemmings, seen from the SIDE in profile, standing upright on their hind feet, all facing and walking to the RIGHT, arranged in a single evenly spaced row across the middle of the frame, all three the same size and standing on the same invisible ground line):
- Silhouette is a PEAR / EGG shape: a big round head on top, sitting directly on a wide heavy rounded body. Taller than it is wide.
- Dark charcoal grey fur forms a CAP over the top and back of the head, coming down over the brow like a hood.
- A large CREAM-WHITE face patch: rounded muzzle and a full puffy cheek filling the lower front of the head.
- One shiny BLACK round button eye in the grey cap, with one single white square highlight pixel in the upper-left.
- A small BLACK triangular nose at the front of the muzzle, and a short simple mouth line curving into a gentle smile.
- IMPORTANT: one small BEIGE rounded EAR poking out at the top side of the head, level with the eye. It is an ear — do NOT draw it as an arm, a horn, or an antenna.
- OUTFIT: a smart DARK NAVY BLUE banker's suit jacket covering the whole body, with a crisp WHITE shirt collar and a small RED necktie at the throat. Narrow WARM ORANGE fur shows at the very back edge of the body below the jacket. Two small BEIGE flat feet poke out at the bottom.
- Each one carries a small GOLD-YELLOW rectangular BRIEFCASE in the near paw, held down at its side.
- Walking pose: legs mid-stride, one foot forward and one foot back, leaning very slightly forward. Expression: calm, sweet, a soft closed smile.

STRICT STYLE RULES:
- FLAT SOLID COLOUR BLOCKS ONLY. Every region is one uniform colour. Zero gradients, zero airbrush, zero soft shading, zero blending, zero dithering, zero drop shadows, zero glow.
- Where shading is needed, use one extra flat darker tone of the same hue as a hard-edged block — never a smooth ramp.
- Total palette of at most 10 flat colours: black, dark charcoal grey, dark navy blue, warm orange, cream white, white, beige, gold yellow, red, and the green background.
- Every colour must be a bold, saturated, clearly distinct value so the sprites stay readable on a tiny low-colour LCD.
- A clean 1-pixel dark outline around each whole character silhouette and around the main internal shapes.
- Chunky, simple, readable forms. Big features. Nothing thinner than 2 pixels.

COMPOSITION: exactly three characters, side by side, centred as a group, large in frame, with clear empty space above their heads and below their feet. The background is ONE SINGLE COMPLETELY FLAT PURE BRIGHT GREEN (#00FF00) filling every part of the frame that is not a character — no ground, no floor, no ground shadow, no horizon line, no building, no scenery, no text, no grid, no border, no frame, no watermark, no fourth character, no props other than the three briefcases.

## Motion

Locked-off static camera, absolutely no camera movement, no pan, no tilt, no zoom, no dolly, no parallax. The three pixel-art lemming bank clerks march on the spot in a walk cycle: legs swing forward and back in alternating strides, briefcases swing gently, bodies bob very slightly up and down with each step. The three characters stay in exactly the same positions in the frame the whole time and never drift sideways, never leave the frame, and never change size. The flat pure green background stays completely empty, flat and unchanged — nothing is added to it and no shadow falls on it. The art style stays exactly as in the first frame: flat 8-bit pixel art, hard chunky pixel edges, no blur, no anti-aliasing, no motion blur, no lighting changes, no new colours.

## Notes

- "Ear, not an arm" is inherited from `lemming_v4.md` and is load-bearing.
- Walking on the spot, not across the frame: the horizontal traverse is done by the app,
  which translates the sprite layer. That keeps the asset a short seamless cycle instead
  of a long one-shot, which matters for both flash size and battery.
- The suits deliberately cover most of `lemming_v5`'s cream belly and orange flanks. The
  head keeps v5's markings so the character still reads as the same lemming.
