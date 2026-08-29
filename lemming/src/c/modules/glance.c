#include "glance.h"

// Zone the accelerometer reads when the watch is tilted towards the wearer with
// the screen facing them. Taken from meow-o-clock, which took them from
// pebble_glancing_demo.
#define ACTIVE_ZONE_X_MIN -500
#define ACTIVE_ZONE_X_MAX 500
#define ACTIVE_ZONE_Y_MIN -900
#define ACTIVE_ZONE_Y_MAX 200
#define ACTIVE_ZONE_Z_MIN -1100
#define ACTIVE_ZONE_Z_MAX 0

// 10Hz in batches of 10 wakes the app once a second, which is what the battery
// guide asks for. meow-o-clock samples at 25Hz/25; the extra samples buy
// nothing here, because the wrist stays inside the zone for seconds at a time
// once it is there.
#define GLANCE_SAMPLE_RATE 10
#define GLANCE_BATCH 10

static GlanceHandler s_handler = NULL;
static bool s_was_inactive = true;

static void accel_data_handler(AccelData *data, uint32_t num_samples)
{
	bool in_active_zone = false;
	for (uint32_t i = 0; i < num_samples; i++) {
		if (data[i].x >= ACTIVE_ZONE_X_MIN &&
		    data[i].x <= ACTIVE_ZONE_X_MAX &&
		    data[i].y >= ACTIVE_ZONE_Y_MIN &&
		    data[i].y <= ACTIVE_ZONE_Y_MAX &&
		    data[i].z >= ACTIVE_ZONE_Z_MIN &&
		    data[i].z <= ACTIVE_ZONE_Z_MAX) {
			in_active_zone = true;
			break;
		}
	}

	// Fire on the inactive -> active transition only. Firing on every batch
	// while the wrist is held up would restart the walk continuously.
	if (in_active_zone && s_was_inactive && s_handler) {
		s_handler();
	}
	s_was_inactive = !in_active_zone;
}

void glance_init(GlanceHandler handler)
{
	s_handler = handler;
	s_was_inactive = true;
	accel_data_service_subscribe(GLANCE_BATCH, accel_data_handler);
	accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
}

void glance_deinit(void)
{
	accel_data_service_unsubscribe();
	s_handler = NULL;
}
