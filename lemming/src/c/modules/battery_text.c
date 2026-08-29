#include "battery_text.h"
#include "../config.h"

static TextLayer *s_layer = NULL;
// "100%" plus terminator. TextLayer does not copy the string it is given, so
// this has to outlive every call — it is the layer's backing store, not a
// scratch buffer.
static char s_text[5];

static void render(BatteryChargeState state)
{
	// charge_percent is reported in 10% steps by the firmware; there is no
	// finer reading to be had, so "37%" is not a number this can ever show.
	snprintf(s_text, sizeof(s_text), "%d%%", state.charge_percent);
	layer_mark_dirty(text_layer_get_layer(s_layer));
}

static void battery_handler(BatteryChargeState state)
{
	render(state);
}

void battery_text_init(Layer *parent, GRect bounds)
{
	s_layer = text_layer_create(GRect(bounds.size.w - BATT_W - BATT_MARGIN,
					  BATT_TOP, BATT_W, BATT_H));
	text_layer_set_background_color(s_layer, GColorClear);
	text_layer_set_text_color(s_layer, GColorWhite);
	text_layer_set_font(s_layer, fonts_get_system_font(BATT_FONT));
	text_layer_set_text_alignment(s_layer, GTextAlignmentRight);
	text_layer_set_text(s_layer, s_text);
	layer_add_child(parent, text_layer_get_layer(s_layer));

	// Subscribing only registers for *changes*, and the charge sits still
	// for hours at a time. Without this peek the corner would be blank
	// until the battery next moved a step.
	render(battery_state_service_peek());
	battery_state_service_subscribe(battery_handler);
}

void battery_text_deinit(void)
{
	battery_state_service_unsubscribe();
	if (s_layer) {
		text_layer_destroy(s_layer);
		s_layer = NULL;
	}
}
