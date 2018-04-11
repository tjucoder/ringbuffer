/* Only support one reader and one writer
 * Protected by MIT license:
 * https://mit-license.org/
 */

#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* malloc ring buffer, return value will be used by other function */
void *rb_malloc(unsigned int buffer_size);

/* free ring buffer */
void rb_free(void *rb);

/* read ring buffer, when accept_partial is NULL, must read len or never read */
/* return value: readed bytes indeed */
unsigned int rb_read(void *rb, void *buffer, unsigned int len, int accept_partial);

/* write ring buffer, when accept_partial is NULL, must write len or never write */
/* return value: writed bytes indeed */
unsigned int rb_write(void *rb, void *buffer, unsigned int len, int accept_partial);

#ifdef __cplusplus
}
#endif

#endif