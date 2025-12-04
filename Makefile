CC     := clang
CMAKE  := cmake
FORMAT := clang-format
CFLAGS := -Wall -Wextra -Werror -std=c23 -O3 -Iinclude

include winter/module.mk
include vendor/module.mk

CFLAGS += $(WINTER_CFLAGS) $(VENDOR_CFLAGS)

SRCS := $(addprefix src/, uuid.c core.c) $(WINTER_SRCS)
HDRS := $(wildcard include/**/*.h) $(WINTER_HDRS)

REL_OBJS  := $(SRCS:%.c=build/rel/%.o)
DEB_OBJS  := $(SRCS:%.c=build/deb/%.o)
TEST_OBJS := $(SRCS:%.c=build/test/%.o)

build/rel/%.o: %.c $(HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/deb/%.o: %.c $(HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WINTER_DEB_CFLAGS) -c -o $@ $<

build/test/%.o: %.c $(HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WINTER_TEST_CFLAGS) -c -o $@ $<

main: $(REL_OBJS) $(VENDOR_LIBS) build/rel/src/main.o
	$(CC) $(VENDOR_LD_FLAGS) $^ -o $@

debug: $(DEB_OBJS) $(VENDOR_LIBS) build/deb/src/main.o
	$(CC) $(VENDOR_LD_FLAGS) $(SANITIZER) $^ -o $@

test: $(TEST_OBJS) $(VENDOR_LIBS) build/test/src/test.o
	$(CC) $(VENDOR_LD_FLAGS) $(SANITIZER) $^ -o $@

clean:
	rm -rf build test vendor/libgit2/build vendor/libssh2/build vendor/mbedtls/build

format:
	$(FORMAT) -i $(SRCS) $(HDRS)

all: main debug test

.PHONY: clean format
