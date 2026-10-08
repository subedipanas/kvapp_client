#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <assert.h>

#include "helpers.h"
#include "buffer.h"
#include "client.h"

const uint8_t IN_BUFFER_SIZE = 50;
const size_t K_MAX_MSG = 100;

enum : uint8_t {
    TAG_NIL = 0,
    TAG_ERR = 1,
    TAG_STR = 2,
    TAG_INT = 3,
    TAG_DBL = 4,
    TAG_ARR = 5
};

static const char* TAG_STRINGS[] = {
    "<NULL", "<ERROR:", "<STR:", "<INT:", "<DBL:", "<ARR:"
};

uint32_t print_response (uint8_t* buf, uint32_t counter) {

    uint8_t tag;
    memcpy (&tag, buf+counter, sizeof (tag));
    counter += sizeof (tag);

    printf ("%s", TAG_STRINGS[tag]);
    switch (tag) {
        case TAG_NIL:
            printf (">");
            break;
        case TAG_ERR:
        case TAG_STR:
            uint32_t message_len;
            memcpy (&message_len, buf + counter, sizeof (message_len));
            counter += sizeof (message_len);

            char out_buffer[256];
            memcpy (out_buffer, buf + counter, message_len);
            memset (out_buffer + message_len, '\0', 1);
            counter += message_len;

            printf ("%s>", out_buffer);
            break;
        case TAG_INT:
            int64_t response_int;
            memcpy (&response_int, buf + counter, sizeof (response_int));
            counter += sizeof (response_int);
            
            printf ("%ld>", response_int);
            break;
        case TAG_DBL:
            double response_dbl;
            memcpy (&response_dbl, buf + counter, sizeof (response_dbl));
            counter += sizeof (response_dbl);
            
            printf ("%f>", response_dbl);
            break;
        case TAG_ARR:
            uint32_t array_len;
            memcpy (&array_len, buf + counter, sizeof (array_len));
            counter += sizeof (array_len);

            printf ("%d:", array_len);
            for (uint32_t i=0; i<array_len; ++i) {
                counter = print_response (buf, counter);
                printf (",");
            }
            printf (">");
    }

    return counter;
}

static int32_t read_full(int fd, uint8_t* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

[[maybe_unused]] static int32_t write_all(int fd, const uint8_t* buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

[[maybe_unused]] static int32_t read_res(int fd) {

    // response header
    struct buffer buf;
    if (buffer_init (&buf, 4) < 0) {
        msg("init_buffer() error");
        return -1;
    }

    errno = 0;
    int32_t err = read_full(fd, buf.content, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }

        buffer_destroy(&buf);
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, buf.content, 4);
    if (len > K_MAX_MSG) {
        msg("too long");

        buffer_destroy(&buf);
        return -1;
    }

    // response body
    buffer_resize (&buf, len);
    err = read_full(fd, buf.content, len);
    if (err) {
        msg("read() error");

        buffer_destroy (&buf);
        return err;
    }

    print_response (buf.content, 0);
    printf ("\n");

    buffer_destroy (&buf);
    return 0;
    
}

//static int32_t send_req(int fd, const uint8_t* text, uint32_t len) {
[[maybe_unused]] static int32_t send_req(int fd, char** cmd, int n_items) {

    uint32_t len = 0;
    len += sizeof (uint32_t);   // len n_items

    for (int i=0; i<n_items; ++i) {
        len += 4;   // len n_items[i]
        len += (uint32_t) strlen(*(cmd + i)); // n_items[i]
    }

    if (len > K_MAX_MSG) {
        return -1;
    }

    struct buffer buf;
    if (buffer_init (&buf, K_MAX_MSG) < 0) {
        msg("init_buffer() error");
        return -1;
    }

    if (buffer_append (&buf, (const uint8_t *) &len, 4) < 0) {
        msg("append_to_buffer() error");
        buffer_destroy (&buf);
        return -1;
    }

    if (buffer_append (&buf, (const uint8_t *) &n_items, 4) < 0) {
        msg("append_to_buffer() error");
        buffer_destroy (&buf);
        return -1;
    }

    for (int i=0; i<n_items; ++i) {
        uint32_t p = (uint32_t) strlen(*(cmd + i));
        if (buffer_append (&buf, (const uint8_t *) &p, 4) < 0) {
            msg("append_to_buffer() error");
            buffer_destroy (&buf);
            return -1;
        }

        if (buffer_append (&buf, (const uint8_t *) *(cmd + i), (uint32_t) strlen(*(cmd + i))) < 0) {
            msg("append_to_buffer() error");
            buffer_destroy (&buf);
            return -1;
        }
    }

    write_all(fd, buf.content, len + sizeof (len));

    buffer_destroy (&buf);
    return 0;
}

int main(void) {
    int fd = connect_to_server();

    // normal messages
    char in_buffer[IN_BUFFER_SIZE];
    char* cmd[10];
    int n_items = 0;
    
    while (1) {
        n_items = 0;
        printf ("> ");

        if (scanf("%49[^\n]", in_buffer) != 1) {
            break;
        }

        if (strcmp(in_buffer, "exit") == 0) {
            break;
        }

        char* word = strtok (in_buffer, " ");
        while (word != NULL && n_items < 10) {
            cmd[n_items++] = word;
            word = strtok (NULL, " ");
        }

        send_req(fd, cmd, n_items);
        if (read_res(fd)) {
            // break in case read_res returns invalid value. eg. response too long.
            break;
        }

        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }

    printf("End session!\n");

    disconnect_from_server(fd);
    return EXIT_SUCCESS;

}
