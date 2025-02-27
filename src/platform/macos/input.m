/* Copyright © 2019 Raheman Vaiya.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stddef.h>

#include "impl.h"
#include "input.h"

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

static int grabbed = 0;
static long grabbed_time;

static int input_fds[2];
static struct input_event *grabbed_keys;
static size_t grabbed_keys_sz = 0;

static uint8_t passthrough_keys[256] = {0};

static int keystate[256] = {0};

struct mod {
	uint8_t mask;
	uint8_t code1;
	uint8_t code2;
} modifiers[] = {
	{ MOD_CONTROL, 60, 63 },
	{ MOD_SHIFT, 57, 61 },
	{ MOD_META, 55, 56 },
	{ MOD_ALT, 59, 62 },
};

static long get_time_ms()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_nsec / 1E6 + ts.tv_sec * 1E3;
}

static void write_message(int fd, void *msg, ssize_t sz)
{
	assert(write(fd, msg, sz) == sz);
}

/* returns -1 if the timeout expires before a message is available. */
static int read_message(int fd, void *msg, ssize_t sz, int timeout)
{
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	select(fd+1, &fds, NULL, NULL,
		timeout ? &(struct timeval){.tv_usec = timeout*1E3} : NULL);

	/* timeout */
	if (!FD_ISSET(fd, &fds))
		return -1;

	assert (read(fd, msg, sz) == sz);

	return 0;
}

static CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void* context)
{
	uint8_t code = 0;
	uint8_t pressed = 0;
	static uint8_t active_mods = 0;
	static uint8_t keymods[256] = {0}; /* Mods active at key down time. */
	static long pressed_timestamps[256];

	/* If only apple designed its system APIs like its macbooks... */
	switch (type) {
	NSEvent *nsev;

	case NX_SYSDEFINED: /* system codes (e.g brightness) */
		nsev = [NSEvent eventWithCGEvent:event];

		code = (nsev.data1 >> 16) + 220;
		pressed = !(nsev.data1 & 0x100);
		break;
	case kCGEventFlagsChanged: /* modifier codes */
		code = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)+1;
		pressed = !keystate[code];
		break;
	case kCGEventKeyDown:
	case kCGEventKeyUp:
		/* We shift codes up by 1 so 0 is not a valid code. This is accounted for in the name table. */
		code = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)+1;
		pressed = type == kCGEventKeyDown;
		break;
	default:
		break;
	}

	/* key event. */
	if (code) {
		uint8_t mods;
		size_t i;

		/* repeat event.. */
		if (keystate[code] && pressed)
			pressed = 2;

		if (pressed == 1)
			pressed_timestamps[code] = get_time_ms();

		if (passthrough_keys[code]) {
			passthrough_keys[code]--;
			return event;
		}

		/* compute the active mod set */
		for (i = 0; i < sizeof modifiers / sizeof modifiers[0]; i++) {
			struct mod *mod = &modifiers[i];

			if (code == mod->code1 || code == mod->code2) {
				if (pressed)
					active_mods |= mod->mask;
				else
					active_mods &= ~mod->mask;
			}

		}

		/* ensure mods are consistent across keydown/up events. */
		if (pressed == 0) {
			mods = keymods[code];
		} else if(pressed == 1) {
			mods = active_mods;
			keymods[code] = mods;
		}

		/* report all non-repeat events */
		if (pressed != 2) {
			struct input_event ev;

			ev.code = code;
			ev.pressed = pressed;
			ev.mods = mods;

			write_message(input_fds[1], &ev, sizeof ev);
		}

		keystate[code] = pressed;

		for (i = 0; i < grabbed_keys_sz; i++)
			if (grabbed_keys[i].code == code && grabbed_keys[i].mods == active_mods) {
				grabbed = 1;
				grabbed_time = get_time_ms();
				return nil;
			}

		if (grabbed) {
			/* if the keydown occurred before the grab, allow the keyup to pass through. */
			if (pressed || pressed_timestamps[code] > grabbed_time) {
				return nil;
			}
		}

		//printf("Allowing %s %d\n", input_lookup_name(code), pressed);
	}

	return event;
}


const char *input_lookup_name(uint8_t code)
{
	return input_code_names[code];
}

uint8_t input_lookup_code(const char *name)
{
	size_t i;

	for(i = 0; i < sizeof input_code_names/sizeof(input_code_names[0]); i++) {
		if (input_code_names[i] && !strcmp(input_code_names[i], name))
			return i;
	}

	return 0;
}

static void _send_key(uint8_t code, int pressed)
{
	static int command_down = 0;

	/* left/right command keys */
	if (code == 56 || code == 55)
		command_down += pressed ? 1 : -1;

	/* events should bypass any active grabs */
	passthrough_keys[code]++;
	CGEventRef ev = CGEventCreateKeyboardEvent(NULL, code-1, pressed);

	/* quartz inspects the event flags instead of maintaining its own state */
	if (command_down)
		CGEventSetFlags(ev, kCGEventFlagMaskCommand);


	CGEventPost(kCGHIDEventTap, ev);
	CFRelease(ev);
}

void send_key(uint8_t code, int pressed) {
	dispatch_sync(dispatch_get_main_queue(), ^{
		_send_key(code, pressed);
	});
}

void input_ungrab_keyboard() 
{
	dispatch_sync(dispatch_get_main_queue(), ^{
		grabbed = 0;
	});
}

void input_grab_keyboard() 
{
	if (grabbed)
		return;

	dispatch_sync(dispatch_get_main_queue(), ^{
		grabbed = 1;
		grabbed_time = get_time_ms();
	});
}

struct input_event *input_next_event(int timeout)
{
	static struct input_event ev;

	long start = get_time_ms();
	if (read_message(input_fds[0], &ev, sizeof ev, timeout) < 0)
		return 0;

	return &ev;
}

struct input_event *input_wait(struct input_event *keys, size_t sz)
{
	grabbed_keys = keys;
	grabbed_keys_sz = sz;

	while (1) {
		size_t i;
		struct input_event *ev = input_next_event(0);

		for (i = 0; i < sz; i++)
			if (ev->pressed && keys[i].code == ev->code && keys[i].mods == ev->mods) {
				grabbed_keys = NULL;
				grabbed_keys_sz = 0;

				return ev;
			}
	}
}

/* called by the main thread to set up event stream. */
void init_input()
{
	CFMachPortRef tap = CGEventTapCreate(kCGHIDEventTap,
			kCGHeadInsertEventTap,
			0, kCGEventMaskForAllEvents,
			eventTapCallback, NULL);

	CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
	CGEventTapEnable(tap, true);

	if (pipe(input_fds) < 0) {
		perror("pipe");
		exit(-1);
	}
}
