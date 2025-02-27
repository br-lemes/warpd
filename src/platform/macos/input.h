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

#ifndef IMPL_INPUT_H
#define IMPL_INPUT_H

#include <stdio.h>

/* Massaged from: https://opensource.apple.com/source/ApplePS2Keyboard/ApplePS2Keyboard-9/ApplePS2ToADBMap.h */

/* TODO: make sure names are consistent with the linux map. */

const char *input_code_names[] = {
	[1] = "a",
	[2] = "s",
	[3] = "d",
	[4] = "f",
	[5] = "h",
	[6] = "g",
	[7] = "z",
	[8] = "x",
	[9] = "c",
	[10] = "v",
	[12] = "b",
	[13] = "q",
	[14] = "w",
	[15] = "e",
	[16] = "r",
	[17] = "y",
	[18] = "t",
	[19] = "1",
	[20] = "2",
	[21] = "3",
	[22] = "4",
	[23] = "6",
	[24] = "5",
	[25] = "=",
	[26] = "9",
	[27] = "7",
	[28] = "-",
	[29] = "8",
	[30] = "0",
	[31] = "]",
	[32] = "o",
	[33] = "u",
	[34] = "[",
	[35] = "i",
	[36] = "p",
	[37] = "enter",
	[38] = "l",
	[39] = "j",
	[40] = "'",
	[41] = "k",
	[42] = ";",
	[43] = "\\",
	[44] = "comma",
	[45] = "/",
	[46] = "n",
	[47] = "m",
	[48] = ".",
	[49] = "tab",
	[50] = "space",
	[51] = "`",
	[52] = "backspace",
	[54] = "esc",
	[55] = "rightmeta",
	[56] = "leftmeta",
	[57] = "leftshift",
	[58] = "capslock",
	[59] = "leftalt",
	[60] = "leftcontrol",
	[61] = "rightshift",
	[62] = "rightalt",
	[63] = "rightcontrol",
	[64] = "fn",
	[66] = "keypad del",
	[68] = "keypad *",
	[70] = "keypad +",
	[72] = "num lock",
	[73] = "volup",
	[74] = "voldown",
	[75] = "mute",
	[76] = "keypad /",
	[77] = "keypad enter",
	[79] = "keypad -",
	[83] = "keypad insert",
	[84] = "keypad end",
	[85] = "keypad down",
	[86] = "keypad pgdn",
	[87] = "keypad left",
	[88] = "keypad 5",
	[89] = "keypad right",
	[90] = "keypad home",
	[92] = "keypad up",
	[93] = "keypad pgup",
	[97] = "f5",
	[98] = "f6",
	[99] = "f7",
	[100] = "f3",
	[101] = "f8",
	[102] = "f9",
	[104] = "f11",
	[106] = "printscr",
	[108] = "scroll lock",
	[110] = "f10",
	[111] = "applications",
	[112] = "f12",
	[114] = "pause",
	[115] = "insert",
	[116] = "home",
	[117] = "pageup",
	[118] = "delete",
	[119] = "f4",
	[120] = "end",
	[121] = "f2",
	[122] = "pagedown",
	[123] = "f1",
	[124] = "left",
	[125] = "right",
	[126] = "down",
	[127] = "up",

	[220] = "sound_up",
	[221] = "sound_down",
	[222] = "brightness_up",
	[223] = "brightness_down",
	[224] = "caps_lock",
	[225] = "help",
	[226] = "key",
	[227] = "mute",
	[228] = "arrow_key",
	[229] = "arrow_key",
	[230] = "num_lock",
	[231] = "contrast_up",
	[232] = "contrast_down",
	[233] = "launch_panel",
	[234] = "eject",
	[235] = "vidmirror",
	[236] = "play",
	[237] = "next",
	[238] = "previous",
	[239] = "fast",
	[240] = "rewind",
	[241] = "illumination_up",
	[242] = "illumination_down",
	[243] = "illumination_toggle",
	[244] = "num_special_keys"

};

#endif
