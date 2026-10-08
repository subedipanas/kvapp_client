#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct buffer {
    uint8_t* content;
    size_t max_size;
    uint32_t write_pointer;
    uint32_t read_pointer;
};

int buffer_init (struct buffer* buf, size_t size);
void buffer_destroy (struct buffer* buf);
int buffer_resize (struct buffer* buf, size_t size);
int buffer_append (struct buffer* buf, const uint8_t* text, const size_t len);
int buffer_consume (struct buffer* buf, uint8_t* text, const size_t len, bool read_only);
void buffer_consume_all (struct buffer* buf);
size_t buffer_get_size (const struct buffer* buf);
void buffer_print (const struct buffer* buf);

#endif
