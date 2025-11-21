#ifndef __AWBOOT_PROC_ARMV_PTRACE_H__
#define __AWBOOT_PROC_ARMV_PTRACE_H__

#define USR_MODE        0x10
#define FIQ_MODE        0x11
#define IRQ_MODE        0x12
#define SVC_MODE        0x13
#define MON_MODE        0x16
#define ABT_MODE        0x17
#define HYP_MODE        0x1a
#define UNDEF_MODE      0x1b
#define SYSTEM_MODE     0x1f

#define MODE_MASK       0x1f

#define T_BIT           (1 << 5)
#define F_BIT           (1 << 6)
#define I_BIT           (1 << 7)
#define A_BIT           (1 << 8)

#endif
