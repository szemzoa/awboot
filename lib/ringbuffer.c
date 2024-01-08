#include "ringbuffer.h"

int32_t ringbuffer_init(ringbuffer_t *const rb, void *buf, uint32_t size)
{
	/*
	 * buf size must be aligned to power of 2
	 */
	if ((size & (size - 1)) != 0)
		return -1;

	rb->size		= size - 1;
	rb->read_index	= 0;
	rb->write_index = rb->read_index;
	rb->buf			= (uint8_t *)buf;

	return 0;
}

int32_t ringbuffer_get(ringbuffer_t *const rb, uint8_t *data)
{
	if (rb->write_index != rb->read_index) {
		*data = rb->buf[rb->read_index & rb->size];
		rb->read_index++;
		return 0;
	}

	return 0;
}

int32_t ringbuffer_put(ringbuffer_t *const rb, uint8_t data)
{
	rb->buf[rb->write_index & rb->size] = data;

	/*
	 * buffer full strategy: new data will overwrite the oldest data in
	 * the buffer
	 */
	if ((rb->write_index - rb->read_index) > rb->size) {
		rb->read_index = rb->write_index - rb->size;
	}

	rb->write_index++;

	return 0;
}

uint32_t ringbuffer_num(const ringbuffer_t *const rb)
{
	return rb->write_index - rb->read_index;
}

uint32_t ringbuffer_flush(ringbuffer_t *const rb)
{
	rb->read_index = rb->write_index;

	return 0;
}
