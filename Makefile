CC     := clang
CMAKE  := cmake
FORMAT := clang-format
CFLAGS := -Wall -Wextra -Werror -std=c23 -O3 -Ilibgit2/include

include winter/module.mk

CFLAGS += $(WINTER_CFLAGS)

ifeq ($(UNAME), Darwin)
	LD_FLAGS := -framework GSS -framework Security -framework CoreFoundation -lz -liconv
else ifeq ($(UNAME), Linux)
	LD_FLAGS := -lssl -lcrypto -lz -lpthread
else
    $(error unsupported platform)
endif

SRCS := $(WINTER_SRCS)
HDRS := $(wildcard include/**/*.h) $(WINTER_HDRS)

REL_OBJS  := $(SRCS:%.c=build/rel/%.o)
DEB_OBJS  := $(SRCS:%.c=build/deb/%.o)
TEST_OBJS := $(SRCS:%.c=build/test/%.o)

LIBGIT2 := libgit2/build/libgit2.a

$(LIBGIT2): libgit2/CMakeLists.txt
	@mkdir -p libgit2/build 
	cd libgit2/build && cmake .. -G "Unix Makefiles" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DUSE_SSH=OFF
	$(MAKE) -C libgit2/build libgit2package

build/rel/%.o: %.c $(HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

build/deb/%.o: %.c $(HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WINTER_DEB_CFLAGS) -c -o $@ $<

build/test/%.o: %.c $(HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(WINTER_TEST_CFLAGS) -c -o $@ $<

main: $(REL_OBJS) $(LIBGIT2) build/deb/src/main.o
	$(CC) $(LD_FLAGS) $^ -o $@

debug: $(DEB_OBJS) $(LIBGIT2) build/deb/src/main.o
	$(CC) $(LD_FLAGS) $(SANITIZER) $^ -o $@

test: $(TEST_OBJS) $(LIBGIT2) build/test/src/test.o
	$(CC) $(LD_FLAGS) $(SANITIZER) $^ -o $@

clean:
	rm -rf build test libgit2/build

format:
	$(FORMAT) -i $(SRCS) $(HDRS)

.PHONY: clean format
