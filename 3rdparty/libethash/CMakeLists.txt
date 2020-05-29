cmake_minimum_required(VERSION 3.11)

add_definitions()

set(ETHASH_SRC
    bit_manipulation.h
    builtins.h
    endianness.hpp
    ethash.h
    ethash.hpp
    ethash-internal.hpp
    ethash.cpp
    hash_types.h
    managed.cpp
    keccak.h
    keccak.hpp
    keccak.c
    keccakf800.c
    keccakf1600.c
    kiss99.hpp
    primes.h
    primes.c
    progpow.hpp
    progpow.cpp
)

add_library(ethash STATIC ${ETHASH_SRC})
target_link_libraries(ethash)
