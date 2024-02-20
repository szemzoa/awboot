#ifndef _SYS_HEAP_H_
#define _SYS_HEAP_H_

//#define NUTDEBUG_HEAP

#define NUTMEM_ALIGNMENT	   4
#define NUTMEM_BOTTOM_ALIGN(s) ((s) & ~(NUTMEM_ALIGNMENT - 1))
#define NUTMEM_TOP_ALIGN(s)	   NUTMEM_BOTTOM_ALIGN((s + (NUTMEM_ALIGNMENT - 1)))

typedef struct _HEAPNODE HEAPNODE;

struct _HEAPNODE {
	size_t hn_size; /*!< \brief Size of this node. */
#ifdef NUTDEBUG_HEAP
	HEAPNODE	 *ht_next;
	size_t		ht_size;
	const char *ht_file;
	int			ht_line;
#endif
	HEAPNODE *hn_next; /*!< \brief Link to next free node. */
};

extern HEAPNODE *heapFreeList;

#define NutHeapAdd(a, s)		 NutHeapRootAdd(&heapFreeList, a, s)
#define NutHeapAvailable()		 NutHeapRootAvailable(&heapFreeList)
#define NutHeapRegionAvailable() NutHeapRootRegionAvailable(&heapFreeList)

#ifdef NUTDEBUG_HEAP
#define NutHeapAlloc(s)		 NutHeapDebugRootAlloc(&heapFreeList, s, __FILE__, __LINE__)
#define NutHeapAllocClear(s) NutHeapDebugRootAllocClear(&heapFreeList, s, __FILE__, __LINE__)
#define NutHeapFree(p)		 NutHeapDebugRootFree(&heapFreeList, p, __FILE__, __LINE__)
#define NutHeapRealloc(p, s) NutHeapDebugRootRealloc(&heapFreeList, p, s, __FILE__, __LINE__)
#else
#define NutHeapAlloc(s)		 NutHeapRootAlloc(&heapFreeList, s)
#define NutHeapAllocClear(s) NutHeapRootAllocClear(&heapFreeList, s)
#define NutHeapFree(p)		 NutHeapRootFree(&heapFreeList, p)
#define NutHeapRealloc(p, s) NutHeapRootRealloc(&heapFreeList, p, s)
#endif

extern void	  NutHeapRootAdd(HEAPNODE **root, void *addr, size_t size);
extern size_t NutHeapRootAvailable(HEAPNODE **root);
extern size_t NutHeapRootRegionAvailable(HEAPNODE **root);

#ifdef NUTDEBUG_HEAP
extern void *NutHeapDebugRootAlloc(HEAPNODE **root, size_t size, const char *file, int line);
extern void *NutHeapDebugRootAllocClear(HEAPNODE **root, size_t size, const char *file, int line);
extern int	 NutHeapDebugRootFree(HEAPNODE **root, void *block, const char *file, int line);
extern void *NutHeapDebugRootRealloc(HEAPNODE **root, void *block, size_t size, const char *file, int line);
#else
extern void *NutHeapRootAlloc(HEAPNODE **root, size_t size);
extern void *NutHeapRootAllocClear(HEAPNODE **root, size_t size);
extern int	 NutHeapRootFree(HEAPNODE **root, void *block);
extern void *NutHeapRootRealloc(HEAPNODE **root, void *block, size_t size);
#endif

extern int	NutHeapCheck(void);
extern void NutHeapDump(void *stream);

extern void *calloc(size_t num, size_t size);
extern void *malloc(size_t len);
extern void	 free(void *p);

//#define free	NutHeapFree
//#define malloc	NutHeapAlloc

#endif
