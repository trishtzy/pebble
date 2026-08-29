#include "settings.h"

#if defined(PBL_PLATFORM_EMERY)

// Numbered explicitly rather than taken from an enum of one: the value outlives
// the app, so the slot number is part of the on-watch format and renumbering it
// would silently reset everybody's watchface to colour.
#define PERSIST_KEY_BW 1

// One boolean in and nothing out. The buffers are still sized in whole tens of
// bytes because a dictionary carries seven bytes of header plus seven per tuple
// before the payload, and because AppMessage rejects an inbox too small for
// what the phone sends rather than truncating it.
#define INBOX_SIZE 64
#define OUTBOX_SIZE 64

static bool s_bw = false;
static SettingsChangedHandler s_handler = NULL;

// The config page sends a plain number, and the SDK sizes the tuple by
// MAGNITUDE rather than by any declared type — 0 and 1 both fit in a byte. So
// reading value->int32 unconditionally, as most examples do, reads three bytes
// past a one-byte payload.
static bool tuple_is_true(const Tuple *t)
{
	switch (t->length) {
	case 1:
		return t->value->uint8 != 0;
	case 2:
		return t->value->uint16 != 0;
	case 4:
		return t->value->uint32 != 0;
	default:
		return false;
	}
}

static void inbox_received(DictionaryIterator *iter, void *context)
{
	Tuple *t = dict_find(iter, MESSAGE_KEY_BW_MODE);
	if (!t) {
		return;
	}

	bool bw = tuple_is_true(t);
	if (bw == s_bw) {
		// The config page posts the whole form on every save, so most
		// messages carry the mode the watch is already in. Writing
		// persist and tearing down every bitmap for those would be
		// flash wear and a visible flicker for no change.
		return;
	}

	s_bw = bw;
	persist_write_bool(PERSIST_KEY_BW, s_bw);
	if (s_handler) {
		s_handler();
	}
}

void settings_init(SettingsChangedHandler handler)
{
	s_handler = handler;
	// An absent key reads false, which is the colour default: an existing
	// install that never opens the settings page keeps the face it has.
	s_bw = persist_read_bool(PERSIST_KEY_BW);

	app_message_register_inbox_received(inbox_received);
	app_message_open(INBOX_SIZE, OUTBOX_SIZE);
}

void settings_deinit(void)
{
	app_message_deregister_callbacks();
	s_handler = NULL;
}

bool settings_bw(void)
{
	return s_bw;
}

#else /* basalt, diorite, flint — see settings.h */

void settings_init(SettingsChangedHandler handler)
{
	(void)handler;
}

void settings_deinit(void)
{
}

bool settings_bw(void)
{
	return false;
}

#endif
