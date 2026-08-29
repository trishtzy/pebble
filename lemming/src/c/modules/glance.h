#pragma once
#include <pebble.h>

// Wrist-flick ("glance") detection, ported from meow-o-clock: rather than
// looking for a tap impulse, it watches for the accelerometer entering the zone
// that corresponds to the watch face being turned towards the wearer, and fires
// once on each entry.
typedef void (*GlanceHandler)(void);

void glance_init(GlanceHandler handler);
void glance_deinit(void);
