#include <pebble.h>

// Rectangular Pebble (basalt/aplite): 144x168. Chalk (round): 180x180.
#define CLOCK_CX PBL_IF_ROUND_ELSE(90, 72)
#define CLOCK_CY PBL_IF_ROUND_ELSE(90, 84)
#define TICK_OUTER_R PBL_IF_ROUND_ELSE(84, 62)
#define TICK_INNER_R PBL_IF_ROUND_ELSE(76, 55)
#define MOON_OFFSET_Y 46
#define MOON_RADIUS 13

static Window *s_window;
static Layer *s_bg_layer, *s_moon_layer, *s_hands_layer;
static GPath *s_minute_arrow, *s_hour_arrow;

static const GPathInfo MINUTE_HAND_POINTS = {
	.num_points = 3, .points = (GPoint[]){{-4, 12}, {4, 12}, {0, -62}}};

static const GPathInfo HOUR_HAND_POINTS = {
	.num_points = 3, .points = (GPoint[]){{-5, 12}, {5, 12}, {0, -40}}};

// ---- Moon phase ----

static int isqrt(int n)
{
	int x = 1;
	while (x * x <= n)
		x++;
	return x - 1;
}

// Returns moon age in days 0–29 (0 = new moon, ~15 = full moon).
// Uses Julian Day Number arithmetic against the Jan 6 2000 new moon.
static int get_moon_age(struct tm *t)
{
	int year = t->tm_year + 1900;
	int month = t->tm_mon + 1;
	int day = t->tm_mday;
	int a = (14 - month) / 12;
	int y = year + 4800 - a;
	int m = month + 12 * a - 3;
	long jdn = (long)day + (153 * m + 2) / 5 + 365L * y + y / 4 - y / 100 +
		   y / 400 - 32045;
	long days = jdn - 2451550L; // days since JDN of 2000-01-06 new moon
	long h = (days * 100L) % 2953L;
	if (h < 0)
		h += 2953L;
	return (int)(h / 100); // 0–29
}

// Draws the illuminated portion of the moon row-by-row using Pebble integer
// trig. Algorithm: terminator x = cw * cos(phase_angle).
//   Waxing: illuminate right of terminator; Waning: illuminate left of
//   -terminator.
static void draw_moon(GContext *ctx, GPoint center, int r, int moon_age)
{
	int32_t phase_angle = (int32_t)(moon_age * 100) * TRIG_MAX_ANGLE / 2953;
	int32_t cos_phase = cos_lookup(phase_angle);

	graphics_context_set_fill_color(
		ctx, PBL_IF_COLOR_ELSE(GColorOxfordBlue, GColorBlack));
	graphics_fill_circle(ctx, center, r);
	graphics_context_set_stroke_color(
		ctx, PBL_IF_COLOR_ELSE(GColorPastelYellow, GColorWhite));

	bool waxing = (moon_age < 15);

	for (int dy = -(r - 1); dy <= (r - 1); dy++) {
		int cw = isqrt(r * r - dy * dy);
		if (cw == 0)
			continue;
		int tx = (int)((int32_t)cw * cos_phase / TRIG_MAX_RATIO);
		GPoint p1, p2;
		if (waxing) {
			p1 = GPoint(center.x + tx, center.y + dy);
			p2 = GPoint(center.x + cw, center.y + dy);
		} else {
			p1 = GPoint(center.x - cw, center.y + dy);
			p2 = GPoint(center.x - tx, center.y + dy);
		}
		if (p1.x <= p2.x) {
			graphics_draw_line(ctx, p1, p2);
		}
	}

	graphics_context_set_stroke_color(
		ctx, PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite));
	graphics_draw_circle(ctx, center, r);
}

// ---- Stars ----

#define NUM_STARS 25

static const GPoint STAR_POSITIONS[NUM_STARS] = {
	{8, 12},    {28, 6},   {52, 18},  {80, 5},    {108, 14},
	{130, 8},   {138, 35}, {140, 68}, {136, 105}, {128, 148},
	{108, 162}, {78, 165}, {48, 160}, {20, 152},  {6, 118},
	{4, 82},    {10, 48},  {18, 25},  {42, 10},   {95, 22},
	{120, 38},  {15, 95},  {32, 140}, {118, 90},  {60, 30},
};

// 0=tiny(1px), 1=small, 2=medium
static const uint8_t STAR_RADIUS[NUM_STARS] = {
	1, 0, 2, 1, 0, 2, 1, 0, 1, 2, 0, 1, 2,
	0, 1, 2, 0, 1, 0, 2, 1, 0, 2, 1, 0,
};

// Each star twinkles off for 1 second every 15s, staggered by offset
static const uint8_t STAR_TWINKLE[NUM_STARS] = {
	0,  3,	6, 9, 12, 1,  4, 7, 10, 13, 2, 5, 8,
	11, 14, 0, 4, 8,  12, 2, 6, 10, 1,  9, 5,
};

// ---- Layer callbacks ----

static void bg_update_proc(Layer *layer, GContext *ctx)
{
	GRect bounds = layer_get_bounds(layer);
	GPoint center = GPoint(CLOCK_CX, CLOCK_CY);

	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);

	// Twinkling stars
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	graphics_context_set_fill_color(ctx, GColorWhite);
	for (int i = 0; i < NUM_STARS; i++) {
		if ((t->tm_sec % 15) == STAR_TWINKLE[i])
			continue; // this star is off this second
		graphics_fill_circle(ctx, STAR_POSITIONS[i], STAR_RADIUS[i]);
	}

	// 11 tick marks (skip 6 o'clock — the moon subdial sits there)
	graphics_context_set_stroke_color(ctx, GColorWhite);
	for (int i = 0; i < 12; i++) {
		if (i == 6)
			continue;
		int32_t angle = TRIG_MAX_ANGLE * i / 12;
		GPoint outer = {.x = (int16_t)(sin_lookup(angle) *
					       TICK_OUTER_R / TRIG_MAX_RATIO) +
				     center.x,
				.y = (int16_t)(-cos_lookup(angle) *
					       TICK_OUTER_R / TRIG_MAX_RATIO) +
				     center.y};
		GPoint inner_pt = {
			.x = (int16_t)(sin_lookup(angle) * TICK_INNER_R /
				       TRIG_MAX_RATIO) +
			     center.x,
			.y = (int16_t)(-cos_lookup(angle) * TICK_INNER_R /
				       TRIG_MAX_RATIO) +
			     center.y};
		graphics_draw_line(ctx, inner_pt, outer);
	}
}

static void moon_update_proc(Layer *layer, GContext *ctx)
{
	GPoint moon_center = GPoint(CLOCK_CX, CLOCK_CY + MOON_OFFSET_Y);
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	draw_moon(ctx, moon_center, MOON_RADIUS, get_moon_age(t));
}

static void hands_update_proc(Layer *layer, GContext *ctx)
{
	GPoint center = GPoint(CLOCK_CX, CLOCK_CY);
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_context_set_stroke_color(ctx, GColorBlack);

	gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
	gpath_draw_filled(ctx, s_minute_arrow);
	gpath_draw_outline(ctx, s_minute_arrow);

	gpath_rotate_to(s_hour_arrow,
			(TRIG_MAX_ANGLE *
			 (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) /
				(12 * 6));
	gpath_draw_filled(ctx, s_hour_arrow);
	gpath_draw_outline(ctx, s_hour_arrow);

	// Center pivot dot
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(center.x - 2, center.y - 2, 5, 5), 0,
			   GCornerNone);
}

// ---- Tick handler ----

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	layer_mark_dirty(window_get_root_layer(s_window));
}

// ---- Window lifecycle ----

static void window_load(Window *window)
{
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	GPoint center = GPoint(CLOCK_CX, CLOCK_CY);

	s_bg_layer = layer_create(bounds);
	layer_set_update_proc(s_bg_layer, bg_update_proc);
	layer_add_child(window_layer, s_bg_layer);

	s_moon_layer = layer_create(bounds);
	layer_set_update_proc(s_moon_layer, moon_update_proc);
	layer_add_child(window_layer, s_moon_layer);

	s_hands_layer = layer_create(bounds);
	layer_set_update_proc(s_hands_layer, hands_update_proc);
	layer_add_child(window_layer, s_hands_layer);

	s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
	s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);
	gpath_move_to(s_minute_arrow, center);
	gpath_move_to(s_hour_arrow, center);
}

static void window_unload(Window *window)
{
	gpath_destroy(s_minute_arrow);
	gpath_destroy(s_hour_arrow);
	layer_destroy(s_bg_layer);
	layer_destroy(s_moon_layer);
	layer_destroy(s_hands_layer);
}

// ---- App lifecycle ----

static void init(void)
{
	s_window = window_create();
	window_set_window_handlers(s_window, (WindowHandlers){
						     .load = window_load,
						     .unload = window_unload,
					     });
	window_stack_push(s_window, true);
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit(void)
{
	tick_timer_service_unsubscribe();
	window_destroy(s_window);
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
}
