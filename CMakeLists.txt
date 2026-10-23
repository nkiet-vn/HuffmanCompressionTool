cmake_minimum_required(VERSION 3.16)
project(HuffmanCompressionTool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(huffman_core
    src/huffmanTree.cpp
    src/bitReader.cpp
    src/compressor.cpp
    src/decompressor.cpp
)

add_executable(huffman src/main.cpp)
target_link_libraries(huffman PRIVATE huffman_core)

enable_testing()

add_executable(test_huffmanTree tests/test_huffmanTree.cpp)
target_link_libraries(test_huffmanTree PRIVATE huffman_core)
add_test(NAME huffmanTree COMMAND test_huffmanTree)

add_executable(test_bitReader tests/test_bitReader.cpp)
target_link_libraries(test_bitReader PRIVATE huffman_core)
add_test(NAME bitReader COMMAND test_bitReader)

add_executable(test_roundtrip tests/test_roundtrip.cpp)
target_link_libraries(test_roundtrip PRIVATE huffman_core)
add_test(NAME roundtrip COMMAND test_roundtrip)

add_executable(test_bit_writer tests/test_bit_writer.cpp)
add_test(NAME bit_writer COMMAND test_bit_writer)

add_executable(test_compress tests/test_compress.cpp)
target_link_libraries(test_compress PRIVATE huffman_core)
add_test(NAME compress COMMAND test_compress)
