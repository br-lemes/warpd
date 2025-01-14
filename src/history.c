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

#include <stdlib.h>

#define BUF_SZ 16

static struct {
	size_t head;
	size_t tail;
	int full;

	size_t cur;

	struct {
		int x;
		int y;
	} buf[BUF_SZ];
} hist = {0};

static void add(int x, int y)
{
	if (hist.full)
		hist.tail = (hist.tail+1) % BUF_SZ;

	hist.buf[hist.head].x = x;
	hist.buf[hist.head].y = y;

	hist.cur = hist.head;
	hist.head = (hist.head+1) % BUF_SZ;
	hist.full = hist.head == hist.tail;
}

static void truncate()
{
	if (!hist.full && hist.tail == hist.head) 
		return;

	hist.head = (hist.cur+1) % BUF_SZ;
	hist.full = hist.tail == hist.head;
}

int hist_get(int *x, int *y)
{
	if (!hist.full && hist.tail == hist.head) 
		return -1;

	*x = hist.buf[hist.cur].x;
	*y = hist.buf[hist.cur].y;

	return 0;
}

void hist_add(int x, int y)
{
	int cx, cy;

	if (!hist_get(&cx, &cy) && cx == x && cy == y) 
		return; //dedup

	truncate();
	add(x, y);
}

void hist_prev()
{
	if (!hist.full && hist.tail == hist.head)
		return;

	if (hist.cur == hist.tail) 
		return;

	hist.cur = hist.cur == 0 ? BUF_SZ-1 : hist.cur-1;
}

void hist_next()
{
	if (!hist.full && hist.tail == hist.head) 
		return;

	size_t n = (hist.cur + 1) % BUF_SZ;

	if (n != hist.head)
		hist.cur = n;
}
