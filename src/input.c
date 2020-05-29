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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include <ctype.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/XInput2.h>

/*
 * Use Xinput instead of standard X grabs to avoid interference with X toolkits
 * and window managers. This isn't a silver bullet since such grabs are still
 * mutually exclusive and potentially present the same problems, but do so at a
 * lower level and appear to be less commonly used. Note that master device
 * grabs and normal X grabs are still mutually exclusive which is why we
 * attempt to wrest control of the physical devices away from their
 * corresponding virtual devices (which can still fail if they are individually
 * grabbed, though this is unlikely).
 *
 * NOTE: These functions discard non-input X events. Ideally we should
 * implement a custom event loop, but the current API is cleaner and sufficient
 * for the intended use case.

 */

static Display *dpy;
static int nkbds;
static int *kbds;

static int xiop;

static void init_keyboards()
{
	int i, n;
	XIEventMask mask;

	XIDeviceInfo* devs = XIQueryDevice(dpy, XIAllDevices, &n);
	kbds = calloc(n, sizeof(int));

	for (i = 0; i < n; i++) {
		if (devs[i].use == XISlaveKeyboard)
			kbds[nkbds++] = devs[i].deviceid;
	}

	XIFreeDeviceInfo(devs);
}

static void clear_keys()
{
	char keymap[32];
	size_t i;

	XQueryKeymap(dpy, keymap);
	for (i = 0; i < sizeof keymap; i++) {
		int j;
		for (j = 0;j<8;j++) {
			if(0x01 & (keymap[i] >> j)) {
				KeyCode code = 8*i+j;
				XTestFakeKeyEvent(dpy, code, 0, CurrentTime);
			}
		}
	}
	XFlush(dpy);
}

static void grab_keyboard()
{
	XIEventMask mask;

	mask.deviceid = XIAllDevices;
	mask.mask_len = XIMaskLen(XI_LASTEVENT);
	mask.mask = calloc(mask.mask_len, sizeof(char));
	XISetMask(mask.mask, XI_KeyPress);
	XISetMask(mask.mask, XI_KeyRelease);

	clear_keys();
	for (int i = 0; i < nkbds; i++) {
		if(XIGrabDevice(dpy, kbds[i],
				DefaultRootWindow(dpy),
				CurrentTime,
				None,
				GrabModeAsync,
				GrabModeAsync,
				False, &mask)) {
			fprintf(stderr, "FATAL: Failed to grab keyboard\n");
			exit(-1);
		}
	}

	XFlush(dpy);
}


const char* input_keyseq_to_string(uint16_t seq)
{
	int i = 0;
	static char s[256];
	uint8_t mods = seq >> 8;
	uint8_t code = seq & 0xFF;
	if(seq == 0) return "UNDEFINED";

	if(mods & Mod1Mask) {
		s[i++] = 'A';
		s[i++] = '-';
	}
	if(mods & Mod4Mask) {
		s[i++] = 'M';
		s[i++] = '-';
	}
	if(mods & ShiftMask) {
		s[i++] = 'S';
		s[i++] = '-';
	}
	if(mods & ControlMask) {
		s[i++] = 'C';
		s[i++] = '-';
	}

	strcpy(s + i, XKeysymToString(XKeycodeToKeysym(dpy, code, 0)));

	return s;
}

uint16_t input_parse_keyseq(const char* key) 
{
	KeySym sym;
	if(!key || key[0] == 0) return 0;

	uint8_t code = 0;
	uint8_t mods = 0;

	while(key[1] == '-') {
		switch(key[0]) {
		case 'A':
			mods |= Mod1Mask;
			break;
		case 'M':
			mods |= Mod4Mask;
			break;
		case 'S':
			mods |= ShiftMask;
			break;
		case 'C':
			mods |= ControlMask;
			break;
		default:
			fprintf(stderr, "%s is not a valid key\n", key);
			exit(1);
		}

		key += 2;
	}

	if(key[0]) {
		sym = XStringToKeysym(key);

		if(sym == NoSymbol) {
			fprintf(stderr, "Could not find keysym for %s\n", key);
			exit(1);
		}

		if(key[1] == '\0' && isupper(key[0]))
			mods |= ShiftMask;

		code = XKeysymToKeycode(dpy, sym);
	}

	return (mods << 8) | code;
}

void init_input(Display *_dpy)
{
	dpy = _dpy;
	int ev, err;

	if (!XQueryExtension(dpy, "XInputExtension", &xiop, &ev, &err)) {
		fprintf(stderr, "FATAL: X Input extension not available.\n");
		exit(-1);
	}

	init_keyboards();
}

void input_ungrab_keyboard()
{
	int i;

	for (i = 0; i < nkbds; i++)
		XIUngrabDevice(dpy, kbds[i], CurrentTime);

	XFlush(dpy);
	clear_keys();
}

static void grab(uint16_t seq)
{
	int mods = seq >> 8;
	KeyCode code = seq & 0x00FF;
	XIEventMask mask;

	mask.deviceid = XIAllMasterDevices;
	mask.mask_len = XIMaskLen(XI_LASTEVENT);
	mask.mask = calloc(mask.mask_len, sizeof(char));
	XISetMask (mask.mask, XI_KeyRelease);

	for (int i = 0; i < nkbds; i++) {
		if(XIGrabKeycode(dpy,
				 kbds[i],
				 code,
				 DefaultRootWindow(dpy),
				 GrabModeAsync,
				 GrabModeAsync,
				 False,
				 &mask,
				 1,
				 (XIGrabModifiers[]){{mods, 0}})) {
			fprintf(stderr, "FATAL: Failed to grab %s!\n", input_keyseq_to_string(seq));
			exit(-1);
		}
	}
}

//Waits for one of the provided key sequences to be pressed and 
//returns the activation sequence after grabbing the keyboard.
uint16_t input_wait_for_key(uint16_t *keys, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
		grab(keys[i]);

	while(1) {
		XEvent ev;
		XNextEvent(dpy, &ev);
		XGenericEventCookie *cookie = &ev.xcookie;

		if (cookie->type != GenericEvent ||
		    cookie->extension != xiop ||
		    !XGetEventData(dpy, cookie))
			continue;

		if (cookie->evtype == XI_KeyPress) {
			size_t i;
			XIDeviceEvent *ev = (XIDeviceEvent*)(cookie->data);

			for (i = 0; i < n; i++) {
				if((((ev->mods.effective & ~Mod2Mask) << 8) | ev->detail) == keys[i]) {
					grab_keyboard();
					XFreeEventData(dpy, cookie);
					return keys[i];
				}
			}
		} 

		XFreeEventData(dpy, cookie);
	}
}

uint16_t input_next_key(int timeout)
{
	int xfd = XConnectionNumber(dpy);
	while(1) {
		fd_set fds;

		FD_ZERO(&fds);
		FD_SET(xfd, &fds);

		select(xfd+1,
		       &fds,
		       NULL,
		       NULL,
		       timeout ? &(struct timeval){0, timeout*1000} : NULL);

		if(!XPending(dpy))
			return 0;

		while(XPending(dpy)) {
			XEvent ev;
			XNextEvent(dpy, &ev);
			XGenericEventCookie *cookie = &ev.xcookie;

			if (cookie->type != GenericEvent ||
			    cookie->extension != xiop ||
			    !XGetEventData(dpy, cookie))
				continue;

			if (cookie->evtype == XI_KeyPress) {
				XIDeviceEvent *ev = (XIDeviceEvent*)(cookie->data);
				return (((ev->mods.effective & ~Mod2Mask) << 8) | ev->detail);
			}

			XFreeEventData(dpy, cookie);
		}
	}
}