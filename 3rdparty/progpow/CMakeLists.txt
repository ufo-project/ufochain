cmake_minimum_required(VERSION 3.11)

# use progpow v0.9.2
# add_definitions(-DPROGPOW_VERSION=93)

if (WIN32)
	set(PROGPOW_SRC
		progpow.c
		internal.c
		sha3.c
		io.c
		io_win32.c
		mmap_win32.c
	)
else()
	set(PROGPOW_SRC
		progpow.c
		internal.c
		sha3.c
		io.c
		io_posix.c
	)
endif()

add_library(progpow STATIC ${PROGPOW_SRC})
target_link_libraries(progpow)

