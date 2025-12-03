CC     := clang
FORMAT := clang-format
FLAGS  := -Wall -Wextra -Werror -std=c23 -O3

include winter/module.mk

FLAGS += $(WINTER_FLAGS)

SRCS := $(WINTER_SRCS)
HDRS := $(wildcard include/**/*.h) $(WINTER_HDRS)

REL_OBJS  := $(SRCS:%.c=build/rel/%.o)
DEB_OBJS  := $(SRCS:%.c=build/deb/%.o)
TEST_OBJS := $(SRCS:%.c=build/test/%.o)

build/rel/%.o: %.c $(HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) -c -o $@ $<

build/deb/%.o: %.c $(HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) $(WINTER_DEB_FLAGS) -c -o $@ $<

build/test/%.o: %.c $(HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(FLAGS) $(WINTER_TEST_FLAGS) -c -o $@ $<

main: $(REL_OBJS) build/deb/src/main.o
	$(CC) $^ -o $@

debug: $(DEB_OBJS) build/deb/src/main.o
	$(CC) $(SANITIZER) $^ -o $@

test: $(TEST_OBJS) build/test/src/test.o
	$(CC) $(SANITIZER) $^ -o $@

clean:
	rm -rf build test

format:
	find . -type f \( -name "*.c" -o -name "*.h" \) | xargs $(FORMAT) -i

.PHONY: clean format
