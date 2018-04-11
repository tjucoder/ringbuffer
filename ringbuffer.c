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

typedef struct ring_buffer {
    unsigned int read_;             /* 读偏移 */
    unsigned int write_;            /* 写偏移 */
    unsigned int len_;              /* 数组的长度 */
    char buf_[0];                   /* 零长数组 */
} ring_buffer;

void *rb_malloc(unsigned int buffer_size)
{
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
    unsigned int valid_len = ((ring_buffer *)rb)->write_ - ((ring_buffer *)rb)->read_;
    rb_memory_barrier();
    if (accept_partial == 0 && len > valid_len)
    {
        return 0;
    }
    len = MIN(len, valid_len);
    tail_read_len = MIN(len, (((ring_buffer *)rb)->len_ - ((ring_buffer *)rb)->read_) % ((ring_buffer *)rb)->len_);
    memcpy(buffer, ((ring_buffer *)rb)->buf_ + ((ring_buffer *)rb)->read_ % ((ring_buffer *)rb)->len_, tail_read_len);
    memcpy((char *)buffer + tail_read_len, ((ring_buffer *)rb)->buf_, len - tail_read_len);
    rb_memory_barrier();
    ((ring_buffer *)rb)->read_ += len;
    return len;
}

unsigned int rb_write(void *rb, void *buffer, unsigned int len, int accept_partial)
{
    unsigned int tail_write_len;
    unsigned int empty_len = ((ring_buffer *)rb)->len_ + ((ring_buffer *)rb)->read_ - ((ring_buffer *)rb)->write_;
    rb_memory_barrier();
    if (accept_partial == 0 && len > empty_len)
    {
        return 0;
    }
    len = MIN(len, empty_len);
    tail_write_len = MIN(len, (((ring_buffer *)rb)->len_ - ((ring_buffer *)rb)->write_) % ((ring_buffer *)rb)->len_);
    memcpy(((ring_buffer *)rb)->buf_ + ((ring_buffer *)rb)->write_ % ((ring_buffer *)rb)->len_, buffer, tail_write_len);
    memcpy(((ring_buffer *)rb)->buf_, (char *)buffer + tail_write_len, len - tail_write_len);
    rb_memory_barrier();
    ((ring_buffer *)rb)->write_ += len;
    return len;
}
