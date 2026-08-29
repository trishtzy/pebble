#include "clock_hands.h"
#include "../config.h"

static void hand(GContext *ctx, int32_t angle, int16_t len, uint8_t width)
{
	graphics_context_set_stroke_width(ctx, width);
	GPoint tip = {
		.x = DIAL_CX +
		     (int16_t)(sin_lookup(angle) * len / TRIG_MAX_RATIO),
		.y = DIAL_CY -
		     (int16_t)(cos_lookup(angle) * len / TRIG_MAX_RATIO),
	};
	graphics_draw_line(ctx, GPoint(DIAL_CX, DIAL_CY), tip);
}

static void update_proc(Layer *layer, GContext *ctx)
{
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	graphics_context_set_stroke_color(ctx, GColorBlack);

	// The hour hand advances continuously through the hour rather than
	// snapping on the hour, so it never disagrees with the minute hand.
	int32_t hour_angle = TRIG_MAX_ANGLE *
			     ((t->tm_hour % 12) * 60 + t->tm_min) / (12 * 60);
	int32_t minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;

	hand(ctx, hour_angle, HOUR_HAND_LEN, HOUR_HAND_WIDTH);
	hand(ctx, minute_angle, MINUTE_HAND_LEN, MINUTE_HAND_WIDTH);

	// Cap the pivot so the two strokes meet cleanly at the centre.
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(DIAL_CX - 1, DIAL_CY - 1, 3, 3), 0,
			   GCornerNone);
}

Layer *clock_hands_create(GRect bounds)
{
	Layer *layer = layer_create(bounds);
	layer_set_update_proc(layer, update_proc);
	return layer;
}

void clock_hands_destroy(Layer *layer)
{
	layer_destroy(layer);
}

void clock_hands_update(Layer *layer)
{
	layer_mark_dirty(layer);
}
