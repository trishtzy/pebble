# lemming_bank — "LEMMING BROTHERS" bank facade (watchface background)

Generated via OpenRouter, model `google/gemini-3.1-flash-lite-image`, portrait
(≈ the 200:228 aspect of the Emery display).

Target: 200x228 @ 202 PPI, 64 colours (Pebble RGB222 — each channel one of 00/55/AA/FF).
This is the STATIC layer of the watchface. The clock dial is generated BLANK; the app
draws the hands. The bottom plaza band is generated EMPTY; the walking lemmings are a
separate animated layer composited in front of it at runtime.

## Prompt

8-bit pixel art scene: the grand stone facade of a neoclassical bank, in the style of a 1990s handheld video-game background. Drawn on a coarse low-resolution grid of roughly 100 pixels wide by 114 pixels tall, then upscaled with hard nearest-neighbour edges so every single pixel is a large crisp visible square. No blur, no anti-aliasing, no soft edges anywhere. Portrait orientation, taller than it is wide.

BUILDING (a Washington-style city hall / classical bank, viewed straight on, perfectly symmetrical, filling the frame from left edge to right edge):
- TOP: a wide, shallow triangular PEDIMENT (gable) of pale grey stone with a bold dark outline, occupying roughly the top third of the image.
- Set into the centre of that pediment, a HUGE ROUND CLOCK — the single biggest feature of the whole picture. Its diameter is about HALF the total width of the image and it fills the pediment almost edge to edge, its top touching the apex of the gable. The dial is a circle of pale cream, ringed by a thick gold border, with twelve chunky dark square tick marks evenly around the inside of the ring and one small dark square dot at the exact centre. The dial is COMPLETELY EMPTY otherwise — absolutely NO clock hands, NO numerals, NO text, NO digits inside the circle. Just the ring, the ticks, and the centre dot.
- BELOW the pediment: a horizontal FRIEZE band of darker grey stone spanning the full width, carrying the words "LEMMING BROTHERS" in chunky blocky gold capital letters, one single line, centred, big and clearly readable.
- BELOW the frieze: a row of FIVE SHORT, STOCKY, PLAIN stone COLUMNS, evenly spaced, pale grey with a darker grey shadow side, each with a simple square capital at the top and a square base at the bottom. The columns are squat — no taller than the clock is wide — and they are SMOOTH, with no fluting, no grooves and no fine vertical lines. Deep near-black shadow in the gaps between the columns.
- AT THE BOTTOM, occupying roughly the bottom sixth of the image: three wide horizontal STONE STEPS running the full width of the frame, and in front of them a completely EMPTY flat pale plaza strip along the very bottom edge, tall enough for a small character to stand on. The steps and the plaza strip are bare — no characters, no people, no animals, no objects, no railings, no lamp posts, no signs.
- SKY behind and above the building: one single flat solid dark navy blue, completely empty — no clouds, no stars, no gradient, no birds.

STRICT STYLE RULES:
- FLAT SOLID COLOUR BLOCKS ONLY. Every region is one uniform colour. Zero gradients, zero airbrush, zero soft shading, zero blending, zero drop shadows, zero glow.
- Where shading is needed, use one extra flat darker tone of the same hue as a hard-edged block — never a smooth ramp.
- Total palette of at most 10 flat colours: black, dark navy blue, mid grey, pale grey, off-white, cream, gold, dark gold, dark brown, white.
- Dominant colours are dark navy blue, gold, black and grey.
- Every colour must be a bold, saturated, clearly distinct value so the scene stays readable on a tiny low-colour LCD.
- A clean 1-pixel dark outline around the building silhouette and around the main internal shapes.
- Chunky, simple, readable forms. Big features. Nothing thinner than 2 pixels. Everything strictly horizontal or vertical — no diagonals except the two edges of the pediment.

COMPOSITION: one single building, centred, filling the whole frame edge to edge with no border, no frame, no vignette, no watermark, no signature, no UI overlay, no extra text anywhere except the words "LEMMING BROTHERS" on the frieze.

## Notes

- The dial must come back EMPTY — the "NO clock hands" line is load-bearing, the model
  reflexively draws hands otherwise. If hands survive, blank the dial with `magick` and
  re-composite a procedural dial rather than re-rolling the whole scene.
- The empty bottom plaza strip is where `lemmings_walk_*` is composited at runtime, so
  the "no characters on the steps" line is also load-bearing.
- Model-rendered "LEMMING BROTHERS" often mangles below 144px width. Fallback: generate a
  blank frieze and composite the text with `magick -font ... +antialias` at small size,
  then nearest-neighbour upscale.
