#include <pebble.h>

static Window *s_window;
static TextLayer *s_time_layer;
static BitmapLayer *s_bitmap_layer;
static GBitmapSequence *s_sequence = NULL;
static GBitmap *s_bitmap = NULL;
static AppTimer *s_timer = NULL;

static uint32_t s_current_resource_id = 0;

static bool is_daytime(struct tm *tick_time) {
  int hour = tick_time->tm_hour;
  // Daytime: 08:00 - 18:00 (8 AM - 6 PM)
  return hour >= 8 && hour < 18;
}

static void timer_handler(void *context);

static void load_sequence(uint32_t resource_id) {
  // Don't reload if already loaded
  if (s_current_resource_id == resource_id) {
    return;
  }

  // Clean up existing sequence and bitmap
  if (s_sequence) {
    gbitmap_sequence_destroy(s_sequence);
    s_sequence = NULL;
  }
  if (s_bitmap) {
    gbitmap_destroy(s_bitmap);
    s_bitmap = NULL;
  }

  // Cancel any pending timer
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }

  // Load the new sequence
  s_sequence = gbitmap_sequence_create_with_resource(resource_id);
  if (s_sequence) {
    // Create blank bitmap with the correct size
    GSize frame_size = gbitmap_sequence_get_bitmap_size(s_sequence);
    s_bitmap = gbitmap_create_blank(frame_size, GBitmapFormat8Bit);

    // Set the bitmap on the layer
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);

    // Start animation
    s_current_resource_id = resource_id;
    s_timer = app_timer_register(0, timer_handler, NULL);
  }
}

static void timer_handler(void *context) {
  if (!s_sequence || !s_bitmap) {
    return;
  }

  // Advance to the next frame
  uint32_t next_delay;
  if (gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay)) {
    // Schedule next frame update
    s_timer = app_timer_register(next_delay, timer_handler, NULL);
  } else {
    // Sequence ended, restart from beginning
    gbitmap_sequence_restart(s_sequence);
    s_timer = app_timer_register(0, timer_handler, NULL);
  }

  // Mark layer as dirty to redraw
  layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));
}

static void update_time() {
  // Get current time
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), "%H:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);

  // Determine which animation to show based on time
  uint32_t resource_id = is_daytime(tick_time)
    ? RESOURCE_ID_KITTEN_PLAY_TIME
    : RESOURCE_ID_KITTEN_SLEEPING;

  // Load the appropriate animation
  load_sequence(resource_id);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void init() {
  // Create main window
  s_window = window_create();
  window_stack_push(s_window, true);

  // Get window bounds
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create BitmapLayer for animation (full screen to match 144x168 APNG)
  int image_width = 144;
  int image_height = 168;
  int image_x = 0;
  int image_y = 0;
  s_bitmap_layer = bitmap_layer_create(GRect(image_x, image_y, image_width, image_height));
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 10, bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Make sure the time is displayed from the start and load initial animation
  update_time();
}

static void deinit() {
  // Unsubscribe from services
  tick_timer_service_unsubscribe();

  // Cancel timer
  if (s_timer) {
    app_timer_cancel(s_timer);
  }

  // Destroy layers
  text_layer_destroy(s_time_layer);
  bitmap_layer_destroy(s_bitmap_layer);

  // Destroy bitmaps and sequence
  if (s_sequence) {
    gbitmap_sequence_destroy(s_sequence);
  }
  if (s_bitmap) {
    gbitmap_destroy(s_bitmap);
  }

  // Destroy window
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
