#include "bank_window.h"
#include "../config.h"
#include "../modules/clock_hands.h"
#include "../modules/glance.h"
#include "../modules/walk_anim.h"

// Set to 1 to run one traverse on load. The emulator does not produce realistic
// accelerometer data, so this is the only way to see the animation there.
#define LEMMING_DEBUG_AUTOSTART 0

static Window *s_window = NULL;
static BitmapLayer *s_bank_layer = NULL;
static GBitmap *s_bank_bitmap = NULL;
static Layer *s_hands_layer = NULL;

static void on_glance(void)
{
	walk_anim_start();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	// MINUTE_UNIT, not SECOND_UNIT: the hands only ever move once a minute,
	// so waking sixty times as often would buy nothing but battery drain.
	clock_hands_update(s_hands_layer);
}

#if LEMMING_DEBUG_AUTOSTART
// Replays the traverse on a loop so it can be caught with `pebble screenshot`.
static void debug_replay(void *context)
{
	walk_anim_start();
	app_timer_register(6000, debug_replay, NULL);
}
#endif

static void window_load(Window *window)
{
	Layer *root = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(root);

	// Back to front: static facade, then the hands on its blank dial, then
	// the lemmings in front of everything along the plaza.
	s_bank_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BANK_BG);
	s_bank_layer = bitmap_layer_create(bounds);
	bitmap_layer_set_bitmap(s_bank_layer, s_bank_bitmap);
	layer_add_child(root, bitmap_layer_get_layer(s_bank_layer));

	s_hands_layer = clock_hands_create(bounds);
	layer_add_child(root, s_hands_layer);

	walk_anim_init(root, bounds);

	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	glance_init(on_glance);

#if LEMMING_DEBUG_AUTOSTART
	debug_replay(NULL);
#endif
}

static void window_unload(Window *window)
{
	glance_deinit();
	tick_timer_service_unsubscribe();

	walk_anim_deinit();
	if (s_hands_layer) {
		clock_hands_destroy(s_hands_layer);
		s_hands_layer = NULL;
	}
	if (s_bank_layer) {
		bitmap_layer_destroy(s_bank_layer);
		s_bank_layer = NULL;
	}
	if (s_bank_bitmap) {
		gbitmap_destroy(s_bank_bitmap);
		s_bank_bitmap = NULL;
	}
}

void bank_window_push(void)
{
	s_window = window_create();
	window_set_background_color(s_window, GColorBlack);
	window_set_window_handlers(s_window, (WindowHandlers){
						     .load = window_load,
						     .unload = window_unload,
					     });
	window_stack_push(s_window, true);
}

void bank_window_deinit(void)
{
	if (s_window) {
		window_destroy(s_window);
		s_window = NULL;
	}
}
