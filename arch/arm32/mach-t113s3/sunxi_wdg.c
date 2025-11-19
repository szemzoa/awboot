/*
 * driver/wdg-t113.c
 *
 * Copyright(c) 2007-2022 Jianjun Jiang <8192542@qq.com>
 * Official site: http://xboot.org
 * Mobile phone: +86-18665388956
 * QQ: 8192542
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "sunxi_wdg.h"
#include "sunxi_clk.h"
#include "common.h"
#include "reg-ccu.h"
#include "debug.h"
#include "types.h"

#define WDG_BASE ((u32)0x20500a0)

enum {
	WDG_IRQ_EN	   = 0x00,
	WDG_IRQ_STA	   = 0x04,
	WDG_SOFT_RST   = 0x08,
	WDG_CTRL	   = 0x10,
	WDG_CFG		   = 0x14,
	WDG_MODE	   = 0x18,
	WDG_OUTPUT_CFG = 0x1c,
};

static const int wdt_timeout_map[] = {
	[1]	 = 0x1, /* 1s  */
	[2]	 = 0x2, /* 2s  */
	[3]	 = 0x3, /* 3s  */
	[4]	 = 0x4, /* 4s  */
	[5]	 = 0x5, /* 5s  */
	[6]	 = 0x6, /* 6s  */
	[8]	 = 0x7, /* 8s  */
	[10] = 0x8, /* 10s */
	[12] = 0x9, /* 12s */
	[14] = 0xA, /* 14s */
	[16] = 0xB, /* 16s */
};

void sunxi_wdg_set(int timeout)
{
	u32 val;

	if (timeout < 0)
		timeout = 0;
	if (timeout > 16)
		timeout = 16;

	if (timeout > 0) {
		if (wdt_timeout_map[timeout] == 0)
			timeout++;

		val = read32(WDG_BASE + WDG_MODE) & 0xffff;
		val &= ~(0xf << 4);
		val |= (wdt_timeout_map[timeout] << 4) | (0x1 << 0);
		write32(WDG_BASE + WDG_MODE, (0x16aa << 16) | val);
		write32(WDG_BASE + WDG_CTRL, (0xa57 << 1) | (1 << 0));
		trace("WDG: (re)set at %us\r\n", timeout);
	} else // Disable
	{
		write32(WDG_BASE + WDG_MODE, (0x16aa << 16) | (0 << 0));
		write32(WDG_BASE + WDG_CTRL, (0xa57 << 1) | (1 << 0));
		trace("WDG: disabled\r\n");
	}
}

void sunxi_wdg_init()
{
	write32(WDG_BASE + WDG_IRQ_EN, 0x0);
	write32(WDG_BASE + WDG_IRQ_STA, 0x1);
	write32(WDG_BASE + WDG_CFG, (0x16aa << 16) | (0x1 << 0));
	write32(WDG_BASE + WDG_MODE, (0x16aa << 16) | (0 << 0));
	write32(WDG_BASE + WDG_CTRL, (0xa57 << 1) | (1 << 0));
	trace("WDG: init\r\n");
}
