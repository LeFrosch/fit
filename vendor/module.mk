LIBGIT2     := vendor/libgit2/build/libgit2.a
LIBGIT2_HDR := vendor/libgit2/include

LIBSSH2     := vendor/libssh2/build/src/libssh2.a
LIBSSH2_HDR := vendor/libssh2/include

MBEDCRYPTO  := vendor/mbedtls/build/library/libmbedcrypto.a
MBEDTLS_HDR := vendor/mbedtls/include

VENDOR_CFLAGS := -I$(LIBGIT2_HDR) -I$(LIBSSH2_HDR) -I$(MBEDTLS_HDR)
VENDOR_LIBS   := $(LIBGIT2) $(LIBSSH2) $(MBEDCRYPTO)

PWD   := $(shell pwd)
UNAME := $(shell uname -s)

ifeq ($(UNAME), Darwin)
	VENDOR_LD_FLAGS := -framework GSS -framework Security -framework CoreFoundation -lz -liconv
else ifeq ($(UNAME), Linux)
	VENDOR_LD_FLAGS := -lssl -lcrypto -lz -lpthread
else
    $(error unsupported platform)
endif

$(MBEDCRYPTO): vendor/mbedtls/CMakeLists.txt
	@mkdir -p vendor/mbedtls/build

	cd vendor/mbedtls/build && cmake .. -G "Unix Makefiles" \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_BUILD_TYPE=Debug

	$(MAKE) -C vendor/mbedtls/build mbedcrypto

$(LIBSSH2): $(MBEDCRYPTO) vendor/libssh2/CMakeLists.txt
	@mkdir -p vendor/libssh2/build

	cd vendor/libssh2/build && cmake .. -G "Unix Makefiles" \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCRYPTO_BACKEND=mbedTLS \
		-DMBEDCRYPTO_LIBRARY=$(PWD)/$(MBEDCRYPTO) \
		-DMBEDTLS_INCLUDE_DIR=$(PWD)/$(MBEDTLS_HDR)

	$(MAKE) -C vendor/libssh2/build libssh2_static

$(LIBGIT2): $(LIBSSH2) vendor/libgit2/CMakeLists.txt
	@mkdir -p vendor/libgit2/build

	cd vendor/libgit2/build && cmake .. -G "Unix Makefiles" \
		-DBUILD_SHARED_LIBS=OFF \
		-DCMAKE_BUILD_TYPE=Debug \
		-DUSE_SSH=ON \
		-DLIBSSH2_LIBRARY=$(PWD)/$(LIBSSH2) \
		-DLIBSSH2_INCLUDE_DIR=$(PWD)/$(LIBSSH2_HDR)

	$(MAKE) -C vendor/libgit2/build libgit2package
