cmake_minimum_required(VERSION 3.11)

add_test_snippet(equihash_test pow)
target_link_libraries(equihash_test pow core x17r)

add_test_snippet(progpow_test pow)
target_link_libraries(progpow_test devcore ethash external_pow)

add_test_snippet(stratum_test external_pow)

add_executable(server_stub server_stub.cpp ../../core/block_crypt.cpp) # ???????????????????????????
target_link_libraries(server_stub external_pow node x17r)
