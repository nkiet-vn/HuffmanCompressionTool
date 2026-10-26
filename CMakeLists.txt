cmake_minimum_required(VERSION 3.16)
project(HuffmanCompressionTool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

# Lõi nén/giải nén (Huffman, HUF1)
add_library(huffman_core
    src/huffmanTree.cpp
    src/bitReader.cpp
    src/compressor.cpp
    src/decompressor.cpp
)

# Thành phần kiểm tra (T10): SHA-256, hash file, thống kê, verifier
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

add_executable(testT10 tests/testT10.cpp)
target_link_libraries(testT10 PRIVATE t10Core)
add_test(NAME testT10 COMMAND testT10)

add_executable(verify_samples tests/verify_samples.cpp)
target_link_libraries(verify_samples PRIVATE huffman_core t10Core)
add_test(NAME verify_samples COMMAND verify_samples)
    
add_executable(huffman src/main.cpp)
enable_testing()
add_executable(testHuffmanCore tests/test_huffmanCore.cpp src/huffmanTree.cpp)
add_test(NAME huffmanCore COMMAND testHuffmanCore)
 