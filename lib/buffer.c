#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "buffer.h"
#include "helpers.h"

int buffer_init (struct buffer* buf, size_t size) {
    buf->max_size = size;
    buf->read_pointer = 0;
    buf->write_pointer = 0;

    uint8_t* content = (uint8_t*) malloc (sizeof(uint8_t) * buf->max_size);
    if (content) {
        buf->content = content;
        return 0;
    }
    
    return -1;
}

int buffer_resize (struct buffer* buf, size_t size) {
    uint8_t* new_content = (uint8_t*) realloc (buf->content, sizeof(uint8_t) * size);
    if (new_content) {
        buf->content = new_content;
        buf->max_size = size;
        return 0;
    }

    return -1;
}

void buffer_destroy (struct buffer* buf) {
    if (buf->content) {
        free (buf->content);
        buf->content = NULL;
    }
    
    buf->max_size = 0;
    buf->read_pointer = 0;
    buf->write_pointer = 0;
}

size_t buffer_get_size (const struct buffer* buf) {
    if (buf->read_pointer > buf->write_pointer) {
        return (size_t) (buf->write_pointer + buf->max_size - buf->read_pointer);
    }

    return (size_t) (buf->write_pointer - buf->read_pointer);
}

void buffer_consume_all (struct buffer* buf) {
    buffer_consume (buf, NULL, th_safe_size_t_to_u32(buffer_get_size (buf)), false);
}

int buffer_consume (struct buffer* buf, uint8_t* text, const size_t len, bool read_only) {

    if (len > buf->max_size) {
        return -1;
    }
    uint32_t consume_len = (uint32_t) len;

    if (buf->read_pointer + consume_len <= buf->max_size) {

        if (text) {
            memcpy (text, buf->content + buf->read_pointer, consume_len);
        }

        if (!read_only) {
            buf->read_pointer += consume_len;
        }
    } else {
        uint32_t diff = (uint32_t) buf->max_size - buf->read_pointer;

        if (text) {
            memcpy (text, &buf->content[buf->read_pointer], diff);
            memcpy (&text[diff], buf->content, consume_len - diff);
        }

        if (!read_only) {
            buf->read_pointer = consume_len - diff;
        }
    }

    return 0;
}

int buffer_append (struct buffer* buf, const uint8_t* text, const size_t len) {
    if (len > buf->max_size) {
        return -1;
    }

    uint32_t safe_len = th_safe_size_t_to_u32 (len);

    if ((buf->write_pointer + safe_len) <= buf->max_size) {
        memcpy (&buf->content[buf->write_pointer], text, safe_len);
        buf->write_pointer += safe_len;
    } else {
        uint32_t diff = (uint32_t) buf->max_size - buf->write_pointer;

        memcpy (&buf->content[buf->write_pointer], text, diff);
        memcpy (buf->content, &text[diff], safe_len - diff);
        buf->write_pointer = safe_len - diff;
    }

    return 0;
}

void buffer_print (const struct buffer* buf) {

    uint32_t i = buf->read_pointer;

    printf ("%x ", buf->content[i++]);
    while (i != buf->read_pointer) {
        printf ("%x ", buf->content[i++]);

        if (i >= buf->max_size) {
            i = 0;
        }
    }

    printf ("\n");
}
