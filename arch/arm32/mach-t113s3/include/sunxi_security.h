#ifndef __SUNXI_SECURITY_H__
#define __SUNXI_SECURITY_H__

#include "types.h"

void sunxi_security_setup(void);
void sunxi_gic_set_nonsecure(void);

#endif /* __SUNXI_SECURITY_H__ */
