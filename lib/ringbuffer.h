#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <inttypes.h>

typedef struct {
	uint8_t *buf;
	uint32_t size;
	uint32_t read_index;
	uint32_t write_index;
} ringbuffer_t;

int32_t	 ringbuffer_init(ringbuffer_t *const rb, void *buf, uint32_t size);
int32_t	 ringbuffer_get(ringbuffer_t *const rb, uint8_t *data);
int32_t	 ringbuffer_put(ringbuffer_t *const rb, uint8_t data);
uint32_t ringbuffer_num(const ringbuffer_t *const rb);
uint32_t ringbuffer_flush(ringbuffer_t *const rb);

#endif
