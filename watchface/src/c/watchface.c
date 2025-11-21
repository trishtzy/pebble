#include <pebble.h>

static Window *s_window;
static GBitmap *s_bitmap;
static BitmapLayer *s_bitmap_layer;
static TextLayer *s_time_layer;
static TextLayer *s_battery_layer;
static GBitmap *s_battery_icon = NULL;
static BitmapLayer *s_battery_icon_layer;

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

static void battery_callback(BatteryChargeState state) {
  // Write the battery percentage into a buffer
  static char battery_buffer[16];
  snprintf(battery_buffer, sizeof(battery_buffer), "%d%%", state.charge_percent);

  // Display this on the battery layer
  text_layer_set_text(s_battery_layer, battery_buffer);

  // Destroy old battery icon if it exists
  if (s_battery_icon) {
    gbitmap_destroy(s_battery_icon);
  }

  // Load the appropriate battery icon based on state
  if (state.is_charging) {
    s_battery_icon = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_CHARGING);
  } else if (state.charge_percent == 100) {
    s_battery_icon = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_FULL);
  } else if (state.charge_percent >= 60) {
    s_battery_icon = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_HEALTHY);
  } else if (state.charge_percent >= 40) {
    s_battery_icon = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_HALF);
  } else {
    s_battery_icon = gbitmap_create_with_resource(RESOURCE_ID_BATTERY_LOW);
  }

  // Update the battery icon layer
  bitmap_layer_set_bitmap(s_battery_icon_layer, s_battery_icon);
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
  int image_y = 36;
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

  // Create battery icon layer in top right corner
  int icon_size = 20;
  int icon_x = bounds.size.w - 50;
  int icon_y = 0;
  s_battery_icon_layer = bitmap_layer_create(GRect(icon_x, icon_y, icon_size, icon_size));
  bitmap_layer_set_compositing_mode(s_battery_icon_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_battery_icon_layer));

  // Create battery TextLayer next to the icon
  int battery_width = 45;
  int battery_height = 20;
  int battery_x = bounds.size.w - 50;
  int battery_y = 0;
  s_battery_layer = text_layer_create(GRect(battery_x, battery_y, battery_width, battery_height));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorBlack);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);

  // Add battery layer to the window
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);

  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
}

static void deinit() {
  // Unsubscribe from services
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();

  // Destroy layers
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_time_layer);
  bitmap_layer_destroy(s_battery_icon_layer);
  bitmap_layer_destroy(s_bitmap_layer);

  // Destroy bitmaps
  if (s_battery_icon) {
    gbitmap_destroy(s_battery_icon);
  }
  gbitmap_destroy(s_bitmap);

  // Destroy window
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
