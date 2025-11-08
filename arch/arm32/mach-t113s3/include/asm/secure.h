#ifndef __AWBOOT_ASM_SECURE_H__
#define __AWBOOT_ASM_SECURE_H__

#include <stdint.h>

#define __secure __attribute__((section("._secure.text")))
#define __secure_data __attribute__((section("._secure.data")))

#if !defined(__ASSEMBLY__) && !defined(__ASSEMBLER__)
#include <stdint.h>

extern uintptr_t secure_reloc_off;

static inline uintptr_t secure_ram_offset(uintptr_t ptr)
{
	return ptr - secure_reloc_off;
}

#define secure_ram_addr(_fn) ((__typeof__(&_fn))(secure_ram_offset((uintptr_t)&(_fn))))
#endif

#endif
