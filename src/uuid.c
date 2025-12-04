#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

#include "uuid.h"

result_t
uuid_v7_create(uuid_t out) {
    assert(out != nullptr);

    struct timeval now;
    if (gettimeofday(&now, NULL) != 0) {
        failure(errno, msg("cannot get time of day"));
    }

    const int64_t milliseconds = now.tv_sec * 1000L + now.tv_usec / 1000L;
    const int64_t rand_a = random();
    const int64_t rand_b = random() << 32 | random();

    uuid_v7_package(out, (uint64_t)milliseconds, (uint64_t)rand_a, (uint64_t)rand_b);

    return SUCCESS;
}

static void
intcpy(unsigned char* dst, const uint64_t src, const size_t n) {
    for (size_t i = 0; i < n; i++) {
        dst[i] = (unsigned char)(src >> (8 * (n - i - 1)));
    }
}

void
uuid_v7_package(uuid_t out, const uint64_t timestamp, const uint64_t rand_a, const uint64_t rand_b) {
    intcpy(out + 0, timestamp, 6);
    intcpy(out + 6, rand_a, 2);
    intcpy(out + 8, rand_b, 8);

    // set version to 7
    out[6] = (out[6] & 0x0f) | (7 << 4);

    // set variant to 2
    out[8] = (out[8] & 0x3f) | (2 << 6);
}

void
uuid_unparse(const uuid_t uuid, char out[static 37]) {
    static const char hex[] = "0123456789abcdef";

    for (int i = 0, p = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            out[p++] = '-';
        }

        const unsigned char c = uuid[i];
        out[p++] = hex[c >> 4];
        out[p++] = hex[c & 0xf];
    }

    out[36] = '\0';
}
