#include <pebble.h>

static Window *s_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static GBitmap *s_battery_icon = NULL;
static BitmapLayer *s_battery_icon_layer;
static BitmapLayer *s_bitmap_layer;

#ifdef PBL_BW
// Frame-based animation for aplite
static GBitmap *s_bitmap = NULL;
static AppTimer *s_timer = NULL;
static AppTimer *s_animation_stop_timer = NULL;
static int s_current_frame = 0;
static bool s_is_playing = true;
static bool s_animation_active = false;
static bool s_current_static_is_playing = true;  // Track which static frame is loaded

#define NUM_PLAY_FRAMES 11
#define NUM_SLEEP_FRAMES 11
#define FRAME_DELAY_MS 100
#define ANIMATION_DURATION_MS 3000  // Play animation for 3 seconds

static const uint32_t s_play_frames[NUM_PLAY_FRAMES] = {
  RESOURCE_ID_KITTEN_PLAY_FRAME_0, RESOURCE_ID_KITTEN_PLAY_FRAME_1,
  RESOURCE_ID_KITTEN_PLAY_FRAME_2, RESOURCE_ID_KITTEN_PLAY_FRAME_3,
  RESOURCE_ID_KITTEN_PLAY_FRAME_4, RESOURCE_ID_KITTEN_PLAY_FRAME_5,
  RESOURCE_ID_KITTEN_PLAY_FRAME_6, RESOURCE_ID_KITTEN_PLAY_FRAME_7,
  RESOURCE_ID_KITTEN_PLAY_FRAME_8, RESOURCE_ID_KITTEN_PLAY_FRAME_9,
  RESOURCE_ID_KITTEN_PLAY_FRAME_10
};

static const uint32_t s_sleep_frames[NUM_SLEEP_FRAMES] = {
  RESOURCE_ID_KITTEN_SLEEP_FRAME_0, RESOURCE_ID_KITTEN_SLEEP_FRAME_1,
  RESOURCE_ID_KITTEN_SLEEP_FRAME_2, RESOURCE_ID_KITTEN_SLEEP_FRAME_3,
  RESOURCE_ID_KITTEN_SLEEP_FRAME_4, RESOURCE_ID_KITTEN_SLEEP_FRAME_5,
  RESOURCE_ID_KITTEN_SLEEP_FRAME_6, RESOURCE_ID_KITTEN_SLEEP_FRAME_7,
  RESOURCE_ID_KITTEN_SLEEP_FRAME_8, RESOURCE_ID_KITTEN_SLEEP_FRAME_9,
  RESOURCE_ID_KITTEN_SLEEP_FRAME_10
};
#else
// APNG-based animation for basalt
static GBitmapSequence *s_sequence = NULL;
static GBitmap *s_bitmap = NULL;
static AppTimer *s_timer = NULL;
static AppTimer *s_animation_stop_timer = NULL;
static uint32_t s_current_resource_id = 0;
static bool s_animation_active = false;

#define ANIMATION_DURATION_MS 3000  // Play animation for 3 seconds
#endif

static bool is_daytime(struct tm *tick_time) {
  int hour = tick_time->tm_hour;
  // Daytime: 08:00 - 18:00 (8 AM - 6 PM)
  return hour >= 8 && hour < 18;
}

static void timer_handler(void *context);
static void stop_animation_handler(void *context);
static void accel_data_handler(AccelData *data, uint32_t num_samples);
static void load_static_frame(bool is_playing);

#ifdef PBL_BW
// Frame-based animation for aplite
static void load_static_frame(bool is_playing) {
  // Don't reload if already showing the correct static frame
  if (s_current_static_is_playing == is_playing && s_bitmap != NULL) {
    return;
  }

  // Destroy previous bitmap if exists
  if (s_bitmap) {
    gbitmap_destroy(s_bitmap);
  }

  // Load frame 0 of the appropriate animation
  const uint32_t *frames = is_playing ? s_play_frames : s_sleep_frames;
  s_bitmap = gbitmap_create_with_resource(frames[0]);

  // Set the bitmap on the layer
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));

  // Track which static frame is loaded
  s_current_static_is_playing = is_playing;
}

static void start_animation(bool is_playing) {
  s_is_playing = is_playing;
  s_current_frame = 0;
  s_animation_active = true;

  // Cancel any pending timers
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  if (s_animation_stop_timer) {
    app_timer_cancel(s_animation_stop_timer);
    s_animation_stop_timer = NULL;
  }

  // Start animation
  s_timer = app_timer_register(0, timer_handler, NULL);

  // Schedule animation to stop after duration
  s_animation_stop_timer = app_timer_register(ANIMATION_DURATION_MS, stop_animation_handler, NULL);
}

static void stop_animation_handler(void *context) {
  s_animation_active = false;

  // Cancel animation timer
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }

  // Revert to static first frame
  load_static_frame(s_is_playing);
}

static void timer_handler(void *context) {
  // Stop if animation is no longer active
  if (!s_animation_active) {
    return;
  }

  // Destroy previous bitmap
  if (s_bitmap) {
    gbitmap_destroy(s_bitmap);
  }

  // Get current frame's resource ID
  const uint32_t *frames = s_is_playing ? s_play_frames : s_sleep_frames;
  int num_frames = s_is_playing ? NUM_PLAY_FRAMES : NUM_SLEEP_FRAMES;

  // Load current frame
  s_bitmap = gbitmap_create_with_resource(frames[s_current_frame]);

  // Set the bitmap on the layer
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));

  // Advance to next frame
  s_current_frame = (s_current_frame + 1) % num_frames;

  // Schedule next frame
  s_timer = app_timer_register(FRAME_DELAY_MS, timer_handler, NULL);
}

#else
// APNG-based animation for color platforms
static void load_static_frame(bool is_playing) {
  uint32_t resource_id = is_playing ? RESOURCE_ID_KITTEN_PLAY_TIME : RESOURCE_ID_KITTEN_SLEEPING;

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

  // Load the sequence
  s_sequence = gbitmap_sequence_create_with_resource(resource_id);
  if (s_sequence) {
    // Create blank bitmap with the correct size
    GSize frame_size = gbitmap_sequence_get_bitmap_size(s_sequence);
    s_bitmap = gbitmap_create_blank(frame_size, GBitmapFormat8Bit);

    // Load first frame only (no animation)
    gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, NULL);

    // Set the bitmap on the layer
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));

    s_current_resource_id = resource_id;
  }
}

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

  // Cancel any pending timers
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  if (s_animation_stop_timer) {
    app_timer_cancel(s_animation_stop_timer);
    s_animation_stop_timer = NULL;
  }

  // Load the new sequence
  s_sequence = gbitmap_sequence_create_with_resource(resource_id);
  if (s_sequence) {
    // Create blank bitmap with the correct size
    GSize frame_size = gbitmap_sequence_get_bitmap_size(s_sequence);

    // Always use 8-bit format - APNG decoder outputs 8-bit even for BW platforms
    s_bitmap = gbitmap_create_blank(frame_size, GBitmapFormat8Bit);

    // Set the bitmap on the layer
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);

    // Start animation
    s_current_resource_id = resource_id;
    s_animation_active = true;
    s_timer = app_timer_register(0, timer_handler, NULL);

    // Schedule animation to stop after duration
    s_animation_stop_timer = app_timer_register(ANIMATION_DURATION_MS, stop_animation_handler, NULL);
  }
}

static void stop_animation_handler(void *context) {
  s_animation_active = false;

  // Cancel animation timer
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }

  // Revert to static first frame
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  bool is_playing = is_daytime(tick_time);
  load_static_frame(is_playing);
}

static void timer_handler(void *context) {
  // Stop if animation is no longer active
  if (!s_animation_active) {
    return;
  }

  uint32_t next_delay;

  // Advance to the next APNG frame, and get the delay for this frame
  if (gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_bitmap, &next_delay)) {
    // Set the new frame into the BitmapLayer
    bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
    layer_mark_dirty(bitmap_layer_get_layer(s_bitmap_layer));

    // Timer for that frame's delay
    s_timer = app_timer_register(next_delay, timer_handler, NULL);
  } else {
    // Sequence ended, restart from beginning
    gbitmap_sequence_restart(s_sequence);
    s_timer = app_timer_register(0, timer_handler, NULL);
  }
}
#endif

static void update_time() {
  // Get current time
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), "%H:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);

  // Write the current date into a buffer
  static char s_date_buffer[20];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%a, %d %b %Y", tick_time);

  // Display this date on the date TextLayer
  text_layer_set_text(s_date_layer, s_date_buffer);

  // Load static frame based on time of day
  bool is_playing = is_daytime(tick_time);
  load_static_frame(is_playing);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  static bool was_inactive = true;

  // Define active zone: watch tilted towards user with screen facing user
  // Based on pebble_glancing_demo zones
  #define ACTIVE_ZONE_X_MIN -500
  #define ACTIVE_ZONE_X_MAX 500
  #define ACTIVE_ZONE_Y_MIN -900
  #define ACTIVE_ZONE_Y_MAX 200
  #define ACTIVE_ZONE_Z_MIN -1100
  #define ACTIVE_ZONE_Z_MAX 0

  // Check if any sample is in the active zone
  bool in_active_zone = false;
  for (uint32_t i = 0; i < num_samples; i++) {
    if (data[i].x >= ACTIVE_ZONE_X_MIN && data[i].x <= ACTIVE_ZONE_X_MAX &&
        data[i].y >= ACTIVE_ZONE_Y_MIN && data[i].y <= ACTIVE_ZONE_Y_MAX &&
        data[i].z >= ACTIVE_ZONE_Z_MIN && data[i].z <= ACTIVE_ZONE_Z_MAX) {
      in_active_zone = true;
      break;
    }
  }

  // Trigger animation when transitioning from inactive to active (wrist flick)
  if (in_active_zone && was_inactive) {
    // Get current time to determine which animation to play
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Determine which animation to show based on time
#ifdef PBL_BW
    // Use frame-based animation for aplite
    bool is_playing = is_daytime(tick_time);
    start_animation(is_playing);
#else
    // Use APNG animation for basalt
    uint32_t resource_id = is_daytime(tick_time)
      ? RESOURCE_ID_KITTEN_PLAY_TIME
      : RESOURCE_ID_KITTEN_SLEEPING;
    load_sequence(resource_id);
#endif
  }

  was_inactive = !in_active_zone;
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

  // Create BitmapLayer for animation (full screen to match 144x168 APNG)
  int image_width = 144;
  int image_height = 168;
  int image_x = 0;
  int image_y = 0;
  s_bitmap_layer = bitmap_layer_create(GRect(image_x, image_y, image_width, image_height));
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

  // Create time TextLayer (vertically centered with date)
  s_time_layer = text_layer_create(GRect(0, 10, bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(0, 53, bounds.size.w, 30));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  // Create battery TextLayer in top right corner with 3px padding from right edge
  // Width is sized to fit "100%" exactly
  int battery_width = 30;
  int battery_height = 20;
  int battery_x = bounds.size.w - battery_width - 5;
  int battery_y = 0;

  // Create battery icon layer to the left of the text with 2px gap
  int icon_size = 20;
  int icon_x = battery_x - icon_size - 2;
  int icon_y = 0;
  s_battery_icon_layer = bitmap_layer_create(GRect(icon_x, icon_y, icon_size, icon_size));
  bitmap_layer_set_compositing_mode(s_battery_icon_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_battery_icon_layer));

  s_battery_layer = text_layer_create(GRect(battery_x, battery_y, battery_width, battery_height));
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorBlack);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);

  // Register for accelerometer data to detect wrist flicks
  // Sample at 25Hz (25 samples per second) with 25 samples per callback
  accel_data_service_subscribe(25, accel_data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);

  // Make sure the time is displayed from the start
  update_time();

  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
}

static void deinit() {
  // Unsubscribe from services
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  accel_data_service_unsubscribe();

  // Cancel timers
  if (s_timer) {
    app_timer_cancel(s_timer);
  }
  if (s_animation_stop_timer) {
    app_timer_cancel(s_animation_stop_timer);
  }

  // Destroy layers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  bitmap_layer_destroy(s_battery_icon_layer);
  bitmap_layer_destroy(s_bitmap_layer);

  // Destroy bitmaps and sequence
  if (s_battery_icon) {
    gbitmap_destroy(s_battery_icon);
  }

#ifdef PBL_BW
  // Frame-based animation cleanup
  if (s_bitmap) {
    gbitmap_destroy(s_bitmap);
  }
#else
  // APNG animation cleanup
  if (s_sequence) {
    gbitmap_sequence_destroy(s_sequence);
  }
  if (s_bitmap) {
    gbitmap_destroy(s_bitmap);
  }
#endif

  // Destroy window
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
