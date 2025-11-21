#include <pebble.h>

static Window *s_window;
static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;
static TextLayer *s_time_layer;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), "%H:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
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

  // Load the bitmap from resources
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ff34a9607b6df8921e81c1f2722fc55b);

  // Create smaller BitmapLayer centered below the time
  int image_size = 144;
  int image_x = 0;
  int image_y = 30;
  s_bitmap_layer = bitmap_layer_create(GRect(image_x, image_y, image_size, image_size));
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);

  // Add the bitmap layer to the window
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 10, bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add time layer to the window
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Make sure the time is displayed from the start
  update_time();
}

static void deinit() {
  // Unsubscribe from tick timer
  tick_timer_service_unsubscribe();

  // Destroy time layer
  text_layer_destroy(s_time_layer);

  // Destroy bitmap layer
  bitmap_layer_destroy(s_bitmap_layer);

  // Destroy bitmap
  gbitmap_destroy(s_bitmap);

  // Destroy window
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
