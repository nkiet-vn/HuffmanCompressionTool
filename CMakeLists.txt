cmake_minimum_required(VERSION 3.16)
project(HuffmanCompressionTool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Các file .cpp sẽ được thêm ở các ticket sau(DSA-2 đến DSA-5).
add_executable(huffman src/main.cpp)
# DSA-4: test cho BitWriter (header-only nên không cần thêm file .cpp nào)
enable_testing()
add_executable(test_bit_writer tests/test_bit_writer.cpp)
add_test(NAME bit_writer COMMAND test_bit_writer)
add_library(t10Core STATIC
    src/Sha256.cpp
    src/FileHasher.cpp
    src/CompressionStats.cpp
    src/CompressionVerifier.cpp
)
target_include_directories(t10Core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(t10Core PUBLIC cxx_std_17)
target_link_libraries(t10Core PUBLIC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)

enable_testing()
add_executable(testT10 tests/testT10.cpp)
target_link_libraries(testT10 PRIVATE t10Core)
add_test(NAME testT10 COMMAND testT10)