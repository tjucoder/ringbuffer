#include "ringbuffer.h"
#include "stdlib.h"
#include "memory.h"

#ifdef _WIN32
#include "windows.h"
#elif __APPLE__
#include "libkern/OSAtomic.h"
#endif

void rb_memory_barrier()
{
#ifdef _WIN32
    MemoryBarrier();
#elif __APPLE__
    OSMemoryBarrier();
#elif __linux__
    asm volatile("mfence" ::: "memory");
#endif
}

#define MIN(a,b) ((a)<(b)?(a):(b))

static inline unsigned int
next_pow_of_2(unsigned int i)
{
    i--;
    i |= i >> 1;
    i |= i >> 2;
    i |= i >> 4;
    i |= i >> 8;
    i |= i >> 16;
    return i + 1;
}

typedef struct ring_buffer {
    unsigned int read_;             /* read pos */
    unsigned int write_;            /* write pos */
    unsigned int len_;              /* buffer length */
    char buf_[0];                   /* pointer to buffer */
} ring_buffer;

void *rb_malloc(unsigned int buffer_size)
{
    buffer_size = next_pow_of_2(buffer_size);
    ring_buffer *buffer = (ring_buffer *)malloc((int)(&(((ring_buffer *)0)->buf_)) + buffer_size);
    memset(buffer + (int)(&(((ring_buffer *)0)->buf_)), 0, buffer_size);
    buffer->read_ = 0;
    buffer->write_ = 0;
    buffer->len_ = buffer_size;
    return buffer;
}

void rb_free(void *rb)
{
    free(rb);
}

unsigned int rb_read(void *rb, void *buffer, unsigned int len, int accept_partial)
{
    unsigned int tail_read_len;
    ring_buffer *prb = (ring_buffer *)rb;
    unsigned int valid_len = prb->write_ - prb->read_;
    rb_memory_barrier();
    if (accept_partial == 0 && len > valid_len)
    {
        return 0;
    }
    len = MIN(len, valid_len);
    tail_read_len = MIN(len, prb->len_ - (prb->read_ & (prb->len_ - 1)));
    memcpy(buffer, prb->buf_ + (prb->read_ & (prb->len_ - 1)), tail_read_len);
    memcpy((char *)buffer + tail_read_len, prb->buf_, len - tail_read_len);
    rb_memory_barrier();
    prb->read_ += len;
    return len;
}

unsigned int rb_write(void *rb, void *buffer, unsigned int len, int accept_partial)
{
    unsigned int tail_write_len;
    ring_buffer *prb = (ring_buffer *)rb;
    unsigned int empty_len = prb->len_ - prb->write_ + prb->read_ ;
    rb_memory_barrier();
    if (accept_partial == 0 && len > empty_len)
    {
        return 0;
    }
    len = MIN(len, empty_len);
    tail_write_len = MIN(len, prb->len_ - (prb->write_ & (prb->len_ - 1)));
    memcpy(prb->buf_ + (prb->write_ & (prb->len_ - 1)), buffer, tail_write_len);
    memcpy(prb->buf_, (char *)buffer + tail_write_len, len - tail_write_len);
    rb_memory_barrier();
    prb->write_ += len;
    return len;
}
