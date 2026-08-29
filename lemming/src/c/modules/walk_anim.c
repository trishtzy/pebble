#include "walk_anim.h"
#include "../config.h"
#include "settings.h"

static BitmapLayer *s_layer = NULL;
static AppTimer *s_timer = NULL;
static int s_step = 0;
static int16_t s_screen_w = 0;
static int16_t s_screen_h = 0;

// The walk cycle reaches the screen two different ways, and only the frame
// SOURCE differs — the traverse below is shared.
//
// Colour platforms play the APNG through a GBitmapSequence, which is the
// cheapest thing to ship: one resource, decoded on demand.
//
// diorite and flint cannot. Their sequence decoder reads the APNG happily and
// fills the target bitmap without complaint, but drawing that bitmap then
// faults inside the firmware — observed with a plain 1-bit blank target under
// both GCompOpSet and GCompOpAssign. A palettized blank target was not tried,
// so the sequence path is not proven impossible here, just not worth the hunt:
// shipping the six frames as ordinary `bitmap` resources has the SDK convert
// them to the platform's native 1-bit form at BUILD time, which sidesteps the
// on-watch decoder altogether. That is also what meow-o-clock does for aplite.
#ifdef PBL_BW

#define WALK_FRAMES 6

static const uint32_t s_frame_ids[WALK_FRAMES] = {
	RESOURCE_ID_LEMMINGS_WALK_F0, RESOURCE_ID_LEMMINGS_WALK_F1,
	RESOURCE_ID_LEMMINGS_WALK_F2, RESOURCE_ID_LEMMINGS_WALK_F3,
	RESOURCE_ID_LEMMINGS_WALK_F4, RESOURCE_ID_LEMMINGS_WALK_F5,
};
static GBitmap *s_frames[WALK_FRAMES];
static int s_frame_idx = 0;

// All six are held resident, as the sequence is on colour: 68x24 at 1 bit is
// about 200 bytes a frame, so the whole cycle costs less than re-reading one
// frame from flash every 80ms would.
static bool frames_load(void)
{
	for (int i = 0; i < WALK_FRAMES; i++) {
		s_frames[i] = gbitmap_create_with_resource(s_frame_ids[i]);
		if (!s_frames[i]) {
			return false;
		}
	}
	return true;
}

static void frames_unload(void)
{
	for (int i = 0; i < WALK_FRAMES; i++) {
		if (s_frames[i]) {
			gbitmap_destroy(s_frames[i]);
			s_frames[i] = NULL;
		}
	}
}

static bool frames_ready(void)
{
	return s_frames[0] != NULL;
}

static void frames_rewind(void)
{
	s_frame_idx = 0;
}

// Wraps at the end of the cycle. The traverse is longer than the cycle, so this
// happens several times per crossing.
static void frames_advance(void)
{
	bitmap_layer_set_bitmap(s_layer, s_frames[s_frame_idx]);
	s_frame_idx = (s_frame_idx + 1) % WALK_FRAMES;
}

#else /* colour: basalt, emery */

static GBitmapSequence *s_sequence = NULL;
static GBitmap *s_frame = NULL;

// On emery the black-and-white mode is a different SEQUENCE, not a different
// code path: the fault that keeps diorite and flint off the APNG is a 1-bit
// platform fault, and emery decodes both sequences happily. So the setting
// costs one resource id here and nothing else.
static uint32_t walk_resource(void)
{
#if defined(PBL_PLATFORM_EMERY)
	return settings_bw() ? RESOURCE_ID_LEMMINGS_WALK_BW
			     : RESOURCE_ID_LEMMINGS_WALK;
#else
	return RESOURCE_ID_LEMMINGS_WALK;
#endif
}

static bool frames_load(void)
{
	// The sequence is tiny (~4 KB of flash) and is kept resident rather
	// than being loaded per flick: re-reading it from flash on every wrist
	// turn would cost more than holding it does.
	s_sequence = gbitmap_sequence_create_with_resource(walk_resource());
	if (!s_sequence) {
		return false;
	}
	s_frame = gbitmap_create_blank(
		gbitmap_sequence_get_bitmap_size(s_sequence),
		GBitmapFormat8Bit);
	return s_frame != NULL;
}

static void frames_unload(void)
{
	if (s_frame) {
		gbitmap_destroy(s_frame);
		s_frame = NULL;
	}
	if (s_sequence) {
		gbitmap_sequence_destroy(s_sequence);
		s_sequence = NULL;
	}
}

static bool frames_ready(void)
{
	return s_sequence != NULL;
}

static void frames_rewind(void)
{
	gbitmap_sequence_restart(s_sequence);
}

static void frames_advance(void)
{
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
}

#endif

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

	frames_advance();
	place(s_step);
	layer_mark_dirty(bitmap_layer_get_layer(s_layer));

	s_timer = app_timer_register(WALK_FRAME_MS, tick, NULL);
}

void walk_anim_init(Layer *parent, GRect bounds)
{
	s_screen_w = bounds.size.w;
	s_screen_h = bounds.size.h;

	if (!frames_load()) {
		frames_unload();
		return;
	}

	s_layer = bitmap_layer_create(
		GRect(-WALK_W, s_screen_h - WALK_H, WALK_W, WALK_H));
	// GCompOpSet is what honours the transparent index; without it the
	// sprite is drawn as an opaque rectangle over the plaza.
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
	frames_unload();
	if (s_layer) {
		bitmap_layer_destroy(s_layer);
		s_layer = NULL;
	}
}

bool walk_anim_is_running(void)
{
	return s_timer != NULL;
}

#if defined(PBL_PLATFORM_EMERY)

void walk_anim_reload(void)
{
	// s_layer is NULL when the frames never loaded at all; there is nothing
	// to swap, and walk_anim_start() already declines to run without them.
	if (!s_layer) {
		return;
	}

	if (s_timer) {
		app_timer_cancel(s_timer);
		s_timer = NULL;
	}
	// Hidden and detached from its bitmap BEFORE the unload: the layer
	// holds the frame by pointer, and a redraw between the free and the
	// reload would follow it into freed memory.
	layer_set_hidden(bitmap_layer_get_layer(s_layer), true);
	bitmap_layer_set_bitmap(s_layer, NULL);

	frames_unload();
	if (!frames_load()) {
		// Same contract as init: no traverse rather than a crash on the
		// next wrist flick.
		frames_unload();
	}
}

#endif /* PBL_PLATFORM_EMERY */

void walk_anim_start(void)
{
	// s_layer is NULL when the frames failed to load: no traverse rather
	// than a crash on the first wrist flick.
	if (s_timer || !s_layer || !frames_ready()) {
		return;
	}

	s_step = 0;
	frames_rewind();
	frames_advance();
	place(s_step);
	layer_set_hidden(bitmap_layer_get_layer(s_layer), false);

	s_timer = app_timer_register(WALK_FRAME_MS, tick, NULL);
}
