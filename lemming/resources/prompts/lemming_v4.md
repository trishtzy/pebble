# lemming_v4 — 8-bit character design

Generated via OpenRouter, model `google/gemini-3.1-flash-lite-image`, size `896x1024`
(≈ the 200:228 portrait aspect of the Emery display).

Target: 200x228 @ 202 PPI, 64 colours (Pebble RGB222 — each channel one of 00/55/AA/FF).
The model cannot emit those dimensions or that palette directly; generate large and flat,
then run `resources/scripts/to_pebble.sh` to downscale (nearest-neighbour) and remap.

## Prompt

8-bit pixel art character sprite of a chubby lemming, in the style of a 1990s handheld video-game sprite. Drawn on a coarse low-resolution grid of roughly 90 pixels wide by 100 pixels tall, then upscaled with hard nearest-neighbour edges so every single pixel is a large crisp visible square. No blur, no anti-aliasing, no soft edges anywhere.

CHARACTER (a plush lemming, front view, sitting upright, facing viewer):
- Silhouette is a PEAR / EGG shape: a big round head on top, slightly narrower than the body, sitting directly on a wide heavy rounded bottom. Taller than it is wide. Not a perfect circle.
- Dark charcoal grey fur forms a CAP over the top and back of the head, coming down over the brow and around the sides of the head like a hood.
- A large CREAM-WHITE face patch: rounded muzzle and full puffy cheeks filling the lower two thirds of the head.
- WARM ORANGE fur only as narrow vertical panels down the far left and far right EDGES of the body — like side stripes framing the body.
- A big CREAM-WHITE belly filling the whole centre front of the body, from under the chin down to the feet.
- Two shiny BLACK round button eyes set into the grey cap, each with one single white square highlight pixel in the upper-left.
- A small BLACK triangular nose, and below it a short simple mouth line curving into a gentle smile.
- IMPORTANT: two small BEIGE rounded EARS poking out at the left and right sides of the head, level with the eyes. These are ears — do NOT draw them as arms, horns, or antennae.
- Two tiny BEIGE stubby front paws held low against the belly, and two small BEIGE flat feet poking out at the very bottom of the body.
- Expression: calm, sweet, a soft closed smile.

STRICT STYLE RULES:
- FLAT SOLID COLOUR BLOCKS ONLY. Every region is one uniform colour. Zero gradients, zero airbrush, zero soft shading, zero blending, zero dithering, zero drop shadows, zero glow.
- Where shading is needed, use one extra flat darker tone of the same hue as a hard-edged block — never a smooth ramp.
- Total palette of at most 10 flat colours: black, dark charcoal grey, mid grey, warm orange, darker orange, cream white, off-white, beige, dark brown, white.
- Every colour must be a bold, saturated, clearly distinct value so the sprite stays readable on a tiny low-colour LCD.
- A clean 1-pixel dark outline around the whole silhouette and around the main internal shapes.
- Chunky, simple, readable forms. Big features. Nothing thinner than 2 pixels.

COMPOSITION: one single character, centred, full body, portrait orientation, filling most of the frame with a small even margin all around. Background is one single completely flat solid colour (pale mint green) with nothing else in it — no ground shadow, no text, no grid, no border, no frame, no watermark, no second character, no props.

## Notes

- The "ears not arms" line is load-bearing — without it the model draws the head tufts as limbs.
- The first attempt (without the STRICT STYLE RULES block) came back with soft airbrush
  shading on the belly and flanks, which smears badly under RGB222 quantisation.
- For side/back views, swap the CHARACTER block for the corresponding inspo reference
  (`resources/inspo/LAM3L_2` side, `LAM3L_3` back) and keep everything else identical.
