#pragma once
#include <pebble.h>

// Draws analog hands onto the blank dial in the bank's pediment. The dial
// itself — ring, tick marks, centre dot — is part of the background bitmap;
// only the hands are drawn.
Layer *clock_hands_create(GRect bounds);
void clock_hands_destroy(Layer *layer);

// Re-reads the wall clock and redraws. Call from a MINUTE_UNIT tick.
void clock_hands_update(Layer *layer);
