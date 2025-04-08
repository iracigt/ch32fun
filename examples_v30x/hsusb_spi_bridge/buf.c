#include <stddef.h>
#include "buf.h"

uint8_t usbdata_buf[DATA_RBUFLEN] = {0};

int chunk_read = 0;
int chunk_write = 0;

buffer_t buf_init() {
    chunk_read = 0;
    chunk_write = 0;
    // bzero(usbdata_buf, DATA_RBUFLEN);
    memset(usbdata_buf, 0x55, DATA_RBUFLEN);

    buffer_t chunk = {
        .buf = usbdata_buf + DATA_CHUNKLEN*chunk_write,
        .len = DATA_CHUNKLEN,
    };

    return chunk;
}

buffer_t buf_next_chunk(bool stall) {
    // implicitly indicates previous write finished

    uint8_t next_write = (chunk_write + 1) % (DATA_RBUFLEN / DATA_CHUNKLEN);
    
    // Is buffer full?
    if (next_write == chunk_read) {
        if (stall) {
            // If we should stall, fail and return NULL buffer
            buffer_t chunk = { .buf = NULL, .len = 0 };
            return chunk;
        } else {
            // If we should overwrite, update read pointer
            chunk_read = (chunk_read + 1) % (DATA_RBUFLEN / DATA_CHUNKLEN);
            // fallthrough to normal not full case
        }
    }

    // Advance the write pointer (if we didn't stall)
    chunk_write = next_write;

    // chunk_write now points to next open chunk
    buffer_t chunk = {
        .buf = usbdata_buf + DATA_CHUNKLEN*chunk_write,
        .len = DATA_CHUNKLEN,
    };

    return chunk;
}

buffer_t buf_read_chunk(void) {
    // Is buffer empty?
    if (chunk_read == chunk_write) {
        // If so, fail and return NULL buffer
        buffer_t chunk = { .buf = NULL, .len = 0 };
        return chunk;
    }

    // chunk_read points to oldest full chunk
    buffer_t chunk = {
        .buf = usbdata_buf + DATA_CHUNKLEN*chunk_read,
        .len = DATA_CHUNKLEN,
    };

    chunk_read = (chunk_read + 1) % (DATA_RBUFLEN / DATA_CHUNKLEN);

    return chunk;
}