#pragma once

#include "winter.h"

typedef unsigned char uuid_t[16];

result_t
uuid_v7_create(uuid_t out);

void
uuid_v7_package(uuid_t out, uint64_t timestamp, uint64_t rand_a, uint64_t rand_b);

void
uuid_unparse(const uuid_t uuid, char out[static 37]);
