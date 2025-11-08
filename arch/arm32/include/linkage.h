#ifndef __ARM32_LINKAGE_H__
#define __ARM32_LINKAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ASM_NL
#define ASM_NL ;
#endif

#define ALIGN	  .align 0
#define ALIGN_STR ".align 0"

#define LENTRY(name) \
	ALIGN ASM_NL    \
	name:

#define ENTRY(name)           \
	.globl name ASM_NL \
	LENTRY(name)

#define WEAK(name)          \
	.weak name ASM_NL \
	name:

#define END(name) .size name, .-name

#define ENDPROC(name)        \
	.type name, %function; \
	END(name)


#ifdef __cplusplus
}
#endif

#endif /* __ARM32_LINKAGE_H__ */
