#include <pebble.h>

#include "windows/bank_window.h"

int main(void)
{
	bank_window_push();
	app_event_loop();
	bank_window_deinit();
}
