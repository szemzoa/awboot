#ifndef __AWBOOT_ASM_IO_H__
#define __AWBOOT_ASM_IO_H__

#include <io.h>

#ifndef readl
#define readl(addr) read32((virtual_addr_t)(addr))
#endif
#ifndef writel
#define writel(val, addr) write32((virtual_addr_t)(addr), (uint32_t)(val))
#endif
#ifndef writeb
#define writeb(val, addr) write8((virtual_addr_t)(addr), (uint8_t)(val))
#endif

#ifndef setbits_le32
#define setbits_le32(addr, mask) writel((readl(addr) | (mask)), (addr))
#endif
#ifndef clrbits_le32
#define clrbits_le32(addr, mask) writel((readl(addr) & ~(mask)), (addr))
#endif

#endif
