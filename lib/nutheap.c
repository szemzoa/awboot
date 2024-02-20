#include <string.h>
#include <stdint.h>
#include "board.h"
#include "debug.h"
#include "nutheap.h"

#define NUTPANIC message

/*! \brief Number of bytes used for management information. */
#define NUT_HEAP_OVERHEAD (sizeof(HEAPNODE) - sizeof(HEAPNODE *))

/*! \brief Minimum size of a node. */
#ifndef NUTMEM_HEAPNODE_MIN
#define NUTMEM_HEAPNODE_MIN (sizeof(HEAPNODE))
#endif

/*!
 * \brief List of free nodes in normal memory.
 */
HEAPNODE *heapFreeList;

#ifdef NUTDEBUG_HEAP
/*!
 * \brief List of allocated nodes.
 */
static HEAPNODE *heapAllocList;
#endif

/*
 * Prepare the user region.
 *
 * Returns a pointer to the memory block's user region, after optionally
 * having added guard patterns at both ends.
 */
static inline void *PrepareUserArea(HEAPNODE *node)
{
	int *tp = (int *)(uintptr_t)&node->hn_next;
#ifdef NUTMEM_GUARD
	size_t off = (node->hn_size - NUT_HEAP_OVERHEAD) / sizeof(int) - 2;

	*tp++		= NUTMEM_GUARD_PATTERN;
	*(tp + off) = NUTMEM_GUARD_PATTERN;
#endif
	return tp;
}

/*
 * Validate the user region.
 *
 * If we have guarded user regions, then this routine will do a sanity
 * check. If the guards had been overridden, then -1 is returned.
 * However, if running in debug mode, then NUTPANIC is called instead.
 *
 * If the guards are still OK or if guard protection is not available,
 * then zero is returned.
 */
#ifdef NUTDEBUG_HEAP
static inline int DebugValidateUserArea(HEAPNODE *node, const char *file, int line)
#else
static inline int ValidateUserArea(HEAPNODE *node)
#endif
{
#ifdef NUTMEM_GUARD
	size_t off = (node->hn_size - NUT_HEAP_OVERHEAD) / sizeof(int) - 1;
	int	*tp  = (int *)(uintptr_t)&node->hn_next;

#ifdef NUTDEBUG_HEAP
	if (*tp != NUTMEM_GUARD_PATTERN) {
		NUTPANIC("%s:%d: Bad memory block at %p\n", file, line, tp + 1);
		return -1;
	}
	if (*(tp + off) != NUTMEM_GUARD_PATTERN) {
		NUTPANIC("%s:%d: Bad memory block at %p with %u bytes allocated in %s:%d\n", file, line, tp + 1, node->ht_size,
				 node->ht_file, node->ht_line);
		return -1;
	}
#else
	if (*tp != NUTMEM_GUARD_PATTERN || *(tp + off) != NUTMEM_GUARD_PATTERN) {
		return -1;
	}
#endif
#endif

	(void)node;

	return 0;
}

#ifdef NUTDEBUG_HEAP
/*
 * Remove a node from the allocation list.
 */
static void DebugUnalloc(HEAPNODE *entry, const char *file, int line)
{
	HEAPNODE	 *ht  = heapAllocList;
	HEAPNODE **htp = &heapAllocList;

	while (ht && ht != entry) {
		htp = &ht->ht_next;
		ht	= ht->ht_next;
	}
	if (ht) {
		*htp = entry->ht_next;
	} else {
		NUTPANIC("%s:%d: Memory block at %p never alloced\n", file, line, entry);
	}
}
#endif

/*!
 * \brief Allocate a block from heap memory.
 *
 * This functions allocates a memory block of the specified size and
 * returns a pointer to that block.
 *
 * The actual size of the allocated block is larger than the requested
 * size because of space required for maintenance information. This
 * additional information is invisible to the application.
 *
 * The routine looks for the smallest block that will meet the required
 * size and releases it to the caller. If the block being requested is
 * usefully smaller than the smallest free block then the block from
 * which the request is being met is split in two. The unused portion is
 * put back into the free-list.
 *
 * The contents of the allocated block is unspecified. To allocate a
 * block with all bytes set to zero use NutHeapAllocClear().
 *
 * \param root Points to the linked list of free nodes.
 * \param size Size of the requested memory block.
 *
 * \return Pointer to the allocated memory block if the function is
 *         successful or NULL if no free block of the requested size
 *         is available.
 */
#ifdef NUTDEBUG_HEAP
void *NutHeapDebugRootAlloc(HEAPNODE **root, size_t size, const char *file, int line)
#else
void			 *NutHeapRootAlloc(HEAPNODE **root, size_t size)
#endif
{
	HEAPNODE	 *node;
	HEAPNODE **npp;
	HEAPNODE	 *fit = NULL;
	HEAPNODE **fpp = NULL;
	size_t	   need;

	/* Determine the minimum size. Add optional guard and alignment bytes.
	 ** Make sure that a HEAPNODE structure fits. */
	need = size + NUT_HEAP_OVERHEAD;
	if (need < sizeof(HEAPNODE)) {
		need = sizeof(HEAPNODE);
	}
	need = NUTMEM_TOP_ALIGN(need);

	/*
	 * Walk through the linked list of free nodes and find the best fit.
	 */
	node = *root;
	npp	 = root;
	while (node) {
		/* Found a note that fits? */
		if (node->hn_size >= need) {
			/* Is it the first one we found or was the previous
			 ** one larger? */
			if (fit == NULL || fit->hn_size > node->hn_size) {
				/* This is the best fit so far. */
				fit = node;
				fpp = npp;
				/* Is this an exact match? */
				if (node->hn_size == need) {
					/* Exact match found. Stop searching. */
					break;
				}
			}
		}
		npp	 = &node->hn_next;
		node = node->hn_next;
	}

	if (fit) {
		/* Is remaining space of the node we found large enough for
		   another node? Honor the specified threshold. */
		if (fit->hn_size - need >= NUTMEM_HEAPNODE_MIN) {
			/* Split the node. */
			node		  = (HEAPNODE *)((uintptr_t)fit + need);
			node->hn_size = fit->hn_size - need;
			node->hn_next = fit->hn_next;
			fit->hn_size  = need;
			*fpp		  = node;
		} else {
			/* Use the full node. */
			*fpp = fit->hn_next;
		}
#ifdef NUTDEBUG_HEAP
		/* Add debug information. */
		fit->ht_size = size;
		fit->ht_file = file;
		fit->ht_line = line;
		/* Add to allocation list. */
		fit->ht_next  = heapAllocList;
		heapAllocList = fit;
#endif
		fit = (HEAPNODE *)PrepareUserArea(fit);
	}
	return fit;
}

/*!
 * \brief Allocate an initialized block from heap memory.
 *
 * This functions allocates a memory block of the specified
 * size with all bytes initialized to zero and returns a
 * pointer to that block.
 *
 * \param root Points to the linked list of free nodes.
 * \param size Size of the requested memory block.
 *
 * \return Pointer to the allocated memory block if the
 *         function is successful or NULL if the requested
 *         amount of memory is not available.
 */
#ifdef NUTDEBUG_HEAP
void *NutHeapDebugRootAllocClear(HEAPNODE **root, size_t size, const char *file, int line)
{
	void *ptr;

	if ((ptr = NutHeapDebugRootAlloc(root, size, file, line)) != 0)
		memset(ptr, 0, size);

	return ptr;
}
#else
void			 *NutHeapRootAllocClear(HEAPNODE **root, size_t size)
{
	void *ptr;

	if ((ptr = NutHeapRootAlloc(root, size)) != 0)
		memset(ptr, 0, size);

	return ptr;
}
#endif

/*!
 * \brief Return a block to heap memory.
 *
 * An application calls this function, when a previously allocated
 * memory block is no longer needed.
 *
 * The heap manager checks, if the released block adjoins any other
 * free regions. If it does, then the adjacent free regions are joined
 * together to form one larger region.
 *
 * \param root  Points to the linked list of free nodes.
 * \param block Points to a memory block previously allocated.
 *
 * \return 0 on success, -1 if the caller tried to free a block which
 *         had been previously released, -2 if the block had been
 *         corrupted. Furthermore, -3 is returned if block is a NULL
 *         pointer, but using this may change as C99 allows this.
 */
#ifdef NUTDEBUG_HEAP
int NutHeapDebugRootFree(HEAPNODE **root, void *block, const char *file, int line)
#else
int	  NutHeapRootFree(HEAPNODE **root, void *block)
#endif
{
	HEAPNODE	 *node;
	HEAPNODE **npp;
	HEAPNODE	 *fnode;

	/* IMHO, this should be removed. It adds additional code, which is
	   useless for most applications. Those, who are interested, can
	   easily do their own check. C99 declares this as a NUL op. */
	if (block == NULL) {
		return -3;
	}

	/* Revive our node pointer. */
	fnode = (HEAPNODE *)((uintptr_t)block - (NUT_HEAP_OVERHEAD));

#ifdef NUTDEBUG_HEAP
	/* Sanity check. */
	if (DebugValidateUserArea(fnode, file, line)) {
		return -2;
	}
	/* Remove from allocation list. */
	if (file) {
		DebugUnalloc(fnode, file, line);
	}
#else
	  if (ValidateUserArea(fnode)) {
		  return -2;
	  }
#endif

	/*
	 * Walk through the linked list of free nodes and try
	 * to link us in.
	 */
	node = *root;
	npp	 = root;
	while (node) {
		/* If this node is directly in front of ours, merge it. */
		if ((uintptr_t)node + node->hn_size == (uintptr_t)fnode) {
			node->hn_size += fnode->hn_size;

			/*
			 * If another free node is directly following us, merge it.
			 */
			if (((uintptr_t)node + node->hn_size) == (uintptr_t)node->hn_next) {
				node->hn_size += node->hn_next->hn_size;
				node->hn_next = node->hn_next->hn_next;
			}
			break;
		}

		/*
		 * If we walked past our address, link us to the list.
		 */
		if ((uintptr_t)node > (uintptr_t)fnode) {
			*npp = fnode;

			/*
			 * If a free node is following us, merge it.
			 */
			if (((uintptr_t)fnode + fnode->hn_size) == (uintptr_t)node) {
				fnode->hn_size += node->hn_size;
				fnode->hn_next = node->hn_next;
			} else
				fnode->hn_next = node;
			break;
		}

		/* If we are within a free node, somebody may have tried to free
		   a block twice. The panic below does not make much sense, because
		   if we have NUTDEBUG_HEAP then we will also have NUTMEM_GUARD.
		   In that case the guard will have been overridden by the link
		   pointer, which is detected by the ValidateUserArea() above. */
		if (((uintptr_t)node + node->hn_size) > (uintptr_t)fnode) {
#ifdef NUTDEBUG_HEAP
			NUTPANIC("Trying to release free heap memory at in %s:%d\r\n", file, line);
#endif
			return -1;
		}

		npp	 = &node->hn_next;
		node = node->hn_next;
	}

	/*
	 * If no link was found, put us at the end of the list
	 */
	if (node == NULL) {
		fnode->hn_next = node;
		*npp		   = fnode;
	}
	return 0;
}

/*!
 * \brief Add a new memory region to the heap.
 *
 * This function can be called more than once to manage non-continous
 * memory regions. It is automatically called by Nut/OS during
 * initialization.
 *
 * \param addr Start address of the memory region.
 * \param size Number of bytes of the memory region.
 */
void NutHeapRootAdd(HEAPNODE **root, void *addr, size_t size)
{
	HEAPNODE *node = (HEAPNODE *)NUTMEM_TOP_ALIGN((uintptr_t)addr);

	node->hn_size = NUTMEM_BOTTOM_ALIGN(size - ((uintptr_t)node - (uintptr_t)addr));
#ifdef NUTDEBUG_HEAP
	NutHeapDebugRootFree(root, PrepareUserArea(node), NULL, 0);
#else
	  NutHeapRootFree(root, PrepareUserArea(node));
#endif
}

/*!
 * \brief Return the total number of bytes available.
 *
 * \return Number of bytes.
 */
size_t NutHeapRootAvailable(HEAPNODE **root)
{
	size_t	  rc = 0;
	HEAPNODE *node;

	/* Collect all free nodes. */
	for (node = *root; node; node = node->hn_next) {
		/* Reduce the size of each node by the required overhead. */
		rc += node->hn_size - NUT_HEAP_OVERHEAD;
	}
	return rc;
}

/*!
 * \brief Return the size of the largest block available.
 *
 * \return Number of bytes.
 */
size_t NutHeapRootRegionAvailable(HEAPNODE **root)
{
	size_t	  rc = 0;
	HEAPNODE *node;

	/* Collect all free nodes. */
	for (node = *root; node; node = node->hn_next) {
		if (rc < node->hn_size) {
			rc = node->hn_size;
		}
	}
	/* Reduce the size by the required overhead. */
	return rc - NUT_HEAP_OVERHEAD;
}

/**
 * \brief Change the size of an allocated memory block.
 *
 * If more memory is requested than available at that block the data
 * is copied to a new, bigger block.
 *
 * \param block Points to a previously allocated memory block. If NULL,
 *              then this call is equivalent to NutHeapRootAlloc().
 * \param size  The requested new size. If 0, then this call is
 *              equivalent to NutHeapRootFree().
 *
 * \return A pointer to the memory block on success or NULL on failures.
 */
#ifdef NUTDEBUG_HEAP
void *NutHeapDebugRootRealloc(HEAPNODE **root, void *block, size_t size, const char *file, int line)
#else
  void *NutHeapRootRealloc(HEAPNODE **root, void *block, size_t size)
#endif
{
	HEAPNODE	 *node;
	HEAPNODE **npp;
	HEAPNODE	 *fnode;
	void		 *newmem;

#ifdef NUTDEBUG_HEAP
	/* With NULL pointer the call is equivalent to alloc. */
	if (block == NULL) {
		return NutHeapDebugRootAlloc(root, size, file, line);
	}
	/* With zero size the call is equivalent to free. */
	if (size == 0) {
		if (NutHeapDebugRootFree(root, block, file, line)) {
			return NULL;
		}
		return block;
	}

	/* Revive our node pointer. */
	fnode = (HEAPNODE *)((uintptr_t)block - (NUT_HEAP_OVERHEAD));

	/* Sanity check. */
	if (DebugValidateUserArea(fnode, file, line)) {
		return NULL;
	}
#else
	  if (block == NULL) {
		  return NutHeapRootAlloc(root, size);
	  }
	  if (size == 0) {
		  if (NutHeapRootFree(root, block)) {
			  return NULL;
		  }
		  return block;
	  }
	  fnode = (HEAPNODE *)((uintptr_t)block - (NUT_HEAP_OVERHEAD));
	  if (ValidateUserArea(fnode)) {
		  return NULL;
	  }
#endif

	/* Determine the minimum size. Add optional guard and alignment bytes.
	   Make sure that a HEAPNODE structure fits. */
	size += NUT_HEAP_OVERHEAD;
	if (size < sizeof(HEAPNODE)) {
		size = sizeof(HEAPNODE);
	}
	size = NUTMEM_TOP_ALIGN(size);

	/*
	 * Expansion.
	 */
	if (size > fnode->hn_size) {
		size_t size_miss = size - fnode->hn_size;

		/* Find the free node following next. */
		node = *root;
		npp	 = root;
		while (node && node < fnode) {
			npp	 = &node->hn_next;
			node = node->hn_next;
		}

		/* If we found a node and if this node is large enough and
		   if it directly follows without a gap, then use it. */
		if (node && node->hn_size >= size_miss && /* */
			(uintptr_t)fnode + fnode->hn_size == (uintptr_t)node) {
			/* Check if the following node is large enough to be split. */
			if (node->hn_size - size_miss >= NUTMEM_HEAPNODE_MIN) {
				/* Adjust the allocated size. */
				fnode->hn_size += size_miss;
				/* Insert the remaining part into the free list. */
				*npp = (HEAPNODE *)((uintptr_t)node + size_miss);
				/* Due to possible overlapping it is important to set
				   the pointer first, then the size. */
				(*npp)->hn_next = node->hn_next;
				(*npp)->hn_size = node->hn_size - size_miss;
				PrepareUserArea(fnode);
			} else {
				/* Adjust the allocated size. */
				fnode->hn_size += node->hn_size;
				PrepareUserArea(fnode);
				/* Remove the merged node from the free list. */
				*npp = node->hn_next;
			}
			/* Return the original pointer. */
			return block;
		}

		/* Relocate if no sufficiently large block follows. */
#ifdef NUTDEBUG_HEAP
		newmem = NutHeapDebugRootAlloc(root, size, file, line);
#else
		  newmem = NutHeapRootAlloc(root, size);
#endif
		if (newmem) {
			memcpy(newmem, block, fnode->hn_size - NUT_HEAP_OVERHEAD);
#ifdef NUTDEBUG_HEAP
			NutHeapDebugRootFree(root, block, file, line);
#else
			  NutHeapRootFree(root, block);
#endif
		}
		return newmem;
	}

	/*
	 * Reduction.
	 */
	if (size < fnode->hn_size - NUTMEM_HEAPNODE_MIN) {
		/* Release the remaining part to the free list. */
		node		  = (HEAPNODE *)((uintptr_t)fnode + size);
		node->hn_size = fnode->hn_size - size;
#ifdef NUTDEBUG_HEAP
		NutHeapDebugRootFree(root, PrepareUserArea(node), NULL, 0);
#else
		  NutHeapRootFree(root, PrepareUserArea(node));
#endif
		/* Adjust the allocated size. */
		fnode->hn_size = size;
		PrepareUserArea(fnode);
	}
	return block;
}

/*!
 * \brief Check consistency of heap.
 *
 * Right now this function will just return 0 unless \ref NUTDEBUG_HEAP
 * is defined.
 *
 * \return -1 if any error has been detected, 0 otherwise.
 */
int NutHeapCheck(void)
{
#ifdef NUTDEBUG_HEAP
	HEAPNODE *node;

	for (node = heapAllocList; node; node = node->ht_next) {
		if (DebugValidateUserArea(node, __FILE__, __LINE__)) {
			return -1;
		}
	}
#endif
	return 0;
}

/*!
 * \brief Dump heap memory to a given stream.
 */
void NutHeapDump(void *stream)
{
	HEAPNODE *node;

	for (node = heapFreeList; node; node = node->hn_next) {
		debug("%p(%d)\r\n", node, (int)node->hn_size);
	}

#ifdef NUTDEBUG_HEAP
	for (node = heapAllocList; node; node = node->ht_next) {
		debug("%p(%u) %s:%d\r\n", node, (int)node->ht_size, node->ht_file, node->ht_line);
	}
#endif
}

/*!
 * \brief Allocate space for an array.
 *
 * \param num  Number of elements.
 * \param size Size of a single element.
 *
 * \return A pointer to the allocated space or a null pointer if
 *         not enough memory space is available.
 */

void *calloc(size_t num, size_t size)
{
	register void *p;

#ifdef SIZE_T_MAX
	if (num && size && SIZE_T_MAX / num < size) {
		return NULL;
	}
#endif
	size *= num;
	p = NutHeapAlloc(size);
	if (p)
		memset(p, 0, size);
	return (p);
}

/*!
 * \brief Allocate a block from heap memory.
 *
 * This function simply calls NutHeapAlloc(). It overrides the function
 * of the runtime library, when the application is linked with nutcrt or
 * nutcrtf.
 *
 * \param len Size of the requested memory block.
 *
 * \return Pointer to the allocated memory block if the
 *         function is successful or NULL if the requested
 *         amount of memory is not available.
 */
void *malloc(size_t len)
{
	debug("malloc %u bytes\r\n", len);
	return NutHeapAlloc(len);
}

/*!
 * \brief Return a block to heap memory.
 *
 * This function simply calls NutHeapFree(). It overrides the function
 * of the runtime library, when the application is linked with nutcrt or
 * nutcrtf.
 *
 * \param p Points to a memory block previously allocated
 *          through a call to malloc().
 */
void free(void *p)
{
	NutHeapFree(p);
}

void *NutHeapAllocAligned(size_t size, size_t alignment)
{
	void	 *p1; // original block
	void **p2; // aligned block
	int	   offset = alignment - 1 + sizeof(void *);

	if ((p1 = (void *)NutHeapAlloc(size + offset)) == NULL)
		return NULL;

	p2	   = (void **)(((size_t)(p1) + offset) & ~(alignment - 1));
	p2[-1] = p1;

	return p2;
}

void NutHeapFreeAligned(void *pointer)
{
	NutHeapFree(((void **)pointer)[-1]);
}

/*@}*/
