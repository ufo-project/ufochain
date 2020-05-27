cmake_minimum_required(VERSION 3.11)

# use progpow v0.9.2
# add_definitions(-DPROGPOW_VERSION=93)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_MODULE_PATH}")

include_directories(impl)

set(POW_SRC
    ufoHash.cpp
    external_progpow.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/crypto/equihashR_impl.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/arith_uint256.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/uint256.cpp
    ${PROJECT_SOURCE_DIR}/3rdparty/utilstrencodings.cpp
)

if(UNIX)
    set_source_files_properties(impl/crypto/equihashR_impl.cpp PROPERTIES COMPILE_FLAGS -O2)
endif()

add_library(pow STATIC ${POW_SRC})

target_compile_definitions(pow PRIVATE ENABLE_MINING)

target_link_libraries(pow 
    PRIVATE
        Boost::boost
        blake2b
        x17r
        progpow
)

add_library(external_pow STATIC
    external_pow_stub.cpp
    stratum.cpp
    stratum_server.cpp
)

target_link_libraries(external_pow p2p pow core x17r)

if (NOT ANDROID AND NOT IOS)
    add_executable(miner_client miner_client.cpp ../core/block_crypt.cpp) # ???????????????????????????

    #find_package(Boost 1.67 REQUIRED COMPONENTS program_options)
    target_link_libraries(miner_client pow external_pow Boost::program_options x17r progpow)

    add_executable(minepool_client minepool_client.cpp ../core/block_crypt.cpp) # ???????????????????????????

    #find_package(Boost 1.67 REQUIRED COMPONENTS program_options)
    target_link_libraries(minepool_client pow external_pow Boost::program_options x17r progpow)

    add_subdirectory(unittests)
endif()
