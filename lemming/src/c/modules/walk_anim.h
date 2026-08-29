#pragma once
#include <pebble.h>

// The three banker lemmings crossing the plaza. The asset is a short walk CYCLE
// — the characters step in place — and this module supplies the horizontal
// travel, so the resource stays a 6-frame 97x34 strip instead of a full-screen
// 50-frame sequence.
void walk_anim_init(Layer *parent, GRect bounds);
void walk_anim_deinit(void);

// Start one left-to-right traverse. Ignored while a traverse is already
// running: restarting mid-cross would teleport the group back off-screen.
void walk_anim_start(void);
bool walk_anim_is_running(void);

// Re-read the frames after the colour/black-and-white setting changed. Cancels
// a traverse in progress: the group would otherwise change colour mid-plaza.
// Emery only, like the setting itself — see settings.h.
#if defined(PBL_PLATFORM_EMERY)
void walk_anim_reload(void);
#endif
