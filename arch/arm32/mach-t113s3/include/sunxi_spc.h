#ifndef SUNXI_T113S3_SPC_H
#define SUNXI_T113S3_SPC_H

#include "types.h"

#define SUNXI_SPC_BASE			0x02000800u
#define SUNXI_SPC_NUM_PORTS		14u

#define SUNXI_SPC_DECPORT_STA_REG(port) \
	(SUNXI_SPC_BASE + 0x0000u + ((u32)(port) * 0x10u))
#define SUNXI_SPC_DECPORT_SET_REG(port) \
	(SUNXI_SPC_BASE + 0x0004u + ((u32)(port) * 0x10u))
#define SUNXI_SPC_DECPORT_CLR_REG(port) \
	(SUNXI_SPC_BASE + 0x0008u + ((u32)(port) * 0x10u))

#endif /* SUNXI_T113S3_SPC_H */
