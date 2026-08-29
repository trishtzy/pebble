#include "walk_anim.h"
#include "../config.h"

static BitmapLayer *s_layer = NULL;
static GBitmapSequence *s_sequence = NULL;
static GBitmap *s_frame = NULL;
static AppTimer *s_timer = NULL;
static int s_step = 0;
static int16_t s_screen_w = 0;
static int16_t s_screen_h = 0;

// Off the left edge at step 0, off the right edge at the last step, so the
// group is never seen popping into or out of existence.
static int16_t step_x(int step)
{
	return (int16_t)(-WALK_W + (int32_t)step * (s_screen_w + WALK_W) /
					   (WALK_STEPS - 1));
}

static void place(int step)
{
	layer_set_frame(
		bitmap_layer_get_layer(s_layer),
		GRect(step_x(step), s_screen_h - WALK_H, WALK_W, WALK_H));
}

static void tick(void *context)
{
	s_timer = NULL;

	if (++s_step >= WALK_STEPS) {
		// Traverse finished: hide the layer and stop the timer
		// entirely, so an idle watchface costs nothing beyond the
		// once-a-minute hand redraw.
		layer_set_hidden(bitmap_layer_get_layer(s_layer), true);
		return;
	}

	uint32_t delay_ms;
	if (!gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_frame,
						       &delay_ms)) {
		// End of the cycle — loop it. The traverse is longer than the
		// cycle, so this happens several times per crossing.
		gbitmap_sequence_restart(s_sequence);
		gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_frame,
							  &delay_ms);
	}

	bitmap_layer_set_bitmap(s_layer, s_frame);
	place(s_step);
	layer_mark_dirty(bitmap_layer_get_layer(s_layer));

	s_timer = app_timer_register(WALK_FRAME_MS, tick, NULL);
}

void walk_anim_init(Layer *parent, GRect bounds)
{
	s_screen_w = bounds.size.w;
	s_screen_h = bounds.size.h;

	// The sequence is tiny (~4 KB of flash) and is kept resident rather
	// than being loaded per flick: re-reading it from flash on every wrist
	// turn would cost more than holding it does.
	s_sequence = gbitmap_sequence_create_with_resource(
		RESOURCE_ID_LEMMINGS_WALK);
	s_frame = gbitmap_create_blank(
		gbitmap_sequence_get_bitmap_size(s_sequence),
		GBitmapFormat8Bit);

	s_layer = bitmap_layer_create(
		GRect(-WALK_W, s_screen_h - WALK_H, WALK_W, WALK_H));
	// GCompOpSet is what honours the APNG's transparent index; without it
	// the sprite is drawn as an opaque rectangle over the plaza.
	bitmap_layer_set_compositing_mode(s_layer, GCompOpSet);
	layer_set_hidden(bitmap_layer_get_layer(s_layer), true);
	layer_add_child(parent, bitmap_layer_get_layer(s_layer));
}

void walk_anim_deinit(void)
{
	if (s_timer) {
		app_timer_cancel(s_timer);
		s_timer = NULL;
	}
	if (s_frame) {
		gbitmap_destroy(s_frame);
		s_frame = NULL;
	}
	if (s_sequence) {
		gbitmap_sequence_destroy(s_sequence);
		s_sequence = NULL;
	}
	if (s_layer) {
		bitmap_layer_destroy(s_layer);
		s_layer = NULL;
	}
}

bool walk_anim_is_running(void)
{
	return s_timer != NULL;
}

void walk_anim_start(void)
{
	if (s_timer || !s_sequence) {
		return;
	}

	s_step = 0;
	gbitmap_sequence_restart(s_sequence);

	uint32_t delay_ms;
	gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_frame,
						  &delay_ms);
	bitmap_layer_set_bitmap(s_layer, s_frame);
	place(s_step);
	layer_set_hidden(bitmap_layer_get_layer(s_layer), false);

	s_timer = app_timer_register(WALK_FRAME_MS, tick, NULL);
}
