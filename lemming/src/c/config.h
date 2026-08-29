#pragma once

// Layout constants measured off the generated art, not guessed. The dial centre
// and radius come from the largest white connected component in
// lemming_bank_<size>.png; the sprite size is whatever walk_to_pebble.sh
// emitted for that display. They are verified end to end by
// resources/scripts/preview_watchface.sh, which composites with these exact
// numbers — if the preview GIF looks right, these are right.
#if defined(PBL_PLATFORM_EMERY)
#define DIAL_CX 99
#define DIAL_CY 55
#define DIAL_R 27
#define WALK_W 97
#define WALK_H 34
#else /* basalt, 144x168 */
#define DIAL_CX 71
#define DIAL_CY 40
#define DIAL_R 19
#define WALK_W 68
#define WALK_H 24
#endif

#define HOUR_HAND_LEN (DIAL_R * 55 / 100)
#define MINUTE_HAND_LEN (DIAL_R * 85 / 100)
#define HOUR_HAND_WIDTH 3
#define MINUTE_HAND_WIDTH 2

// One walk-cycle frame every 80ms, and 50 steps to cross the screen: 4.0s per
// traverse, inside the 3-5s the animation is specced for.
#define WALK_FRAME_MS 80
#define WALK_STEPS 50
