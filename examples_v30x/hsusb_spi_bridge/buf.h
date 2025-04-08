#ifndef _BUF_H
#define _BUF_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *buf;
    uint16_t len;
} buffer_t;

#define BUF_STALL true
#define BUF_OVERWRITE false

buffer_t buf_init(void);
buffer_t buf_next_chunk(bool stall);
buffer_t buf_read_chunk(void);

#define DATA_CHUNKLEN       (1024)
#define DATA_RBUFLEN        (32*DATA_CHUNKLEN)

#endif