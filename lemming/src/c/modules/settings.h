#pragma once
#include <pebble.h>

// The colour / black-and-white choice, and the phone-side plumbing behind it.
//
// EMERY ONLY. diorite and flint have no colour to offer, and basalt has no
// black-and-white assets to switch to (the 1-bit ones are built for the 1-bit
// screens, and are the same 144x168 size, but they are `bitmap` frames rather
// than the sequence basalt plays). So on every other platform this compiles
// down to settings_bw() returning false and nothing else: no persist read, no
// AppMessage buffers, no inbox handler, no dead resource ids.
//
// The mode lives on the WATCH, in persistent storage, not on the phone. A
// watchface is loaded far more often than it is configured, usually with the
// phone out of range, so the phone is the thing that sends changes — never the
// thing that is asked at startup.
typedef void (*SettingsChangedHandler)(void);

// Reads the stored mode and, on emery, opens AppMessage. `handler` is called
// when the phone sends a mode that DIFFERS from the current one; it is never
// called from inside this function, so the window can be built from
// settings_bw() immediately after.
void settings_init(SettingsChangedHandler handler);
void settings_deinit(void);

// True when the black-and-white setting is on. This is the user's choice, not
// the screen's capability — PBL_BW is that, and the two are independent.
bool settings_bw(void);
