#pragma once
#include <pebble.h>

// Battery charge as plain "NN%" text in the top-right corner, over the sky.
// No icon: the facade is busy enough, and the sky wedge beside the pediment is
// the only area of flat colour on the face.
void battery_text_init(Layer *parent, GRect bounds);
void battery_text_deinit(void);
