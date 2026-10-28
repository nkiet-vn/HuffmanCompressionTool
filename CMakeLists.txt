
cmake_minimum_required(VERSION 3.16)
project(HuffmanCompressionTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type mặc định
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Cờ cảnh báo
if(MSVC)
    add_compile_options(/W4 /utf-8)   # đọc source UTF-8 đúng, không phụ thuộc bảng mã máy
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(huffman_core
    src/huffmanTree.cpp
    src/bitReader.cpp
    src/compressor.cpp
    src/decompressor.cpp
)

add_library(t10Core STATIC
    src/Sha256.cpp
    src/FileHasher.cpp
    src/CompressionStats.cpp
    src/CompressionVerifier.cpp
)

target_include_directories(t10Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(t10Core PUBLIC cxx_std_17)

target_link_libraries(t10Core PUBLIC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
)

add_library(consoleUI STATIC
    src/consoleUI.cpp
)
target_link_libraries(consoleUI PUBLIC t10Core)

add_library(appCore STATIC
    src/menuController.cpp
)
target_link_libraries(appCore PUBLIC consoleUI huffman_core)

add_executable(huffman src/main.cpp)
target_link_libraries(huffman PRIVATE huffman_core t10Core consoleUI appCore)

enable_testing()

# Chạy test trong thư mục build để file rác không rơi vào thư mục gốc dự án.
set(TEST_WORK_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_huffmanTree tests/test_huffmanTree.cpp)
target_link_libraries(test_huffmanTree PRIVATE huffman_core)
add_test(NAME huffmanTree COMMAND test_huffmanTree WORKING_DIRECTORY ${TEST_WORK_DIR})

add_executable(test_bitReader tests/test_bitReader.cpp)
target_link_libraries(test_bitReader PRIVATE huffman_core)
add_test(NAME bitReader COMMAND test_bitReader WORKING_DIRECTORY ${TEST_WORK_DIR})

add_executable(test_roundtrip tests/test_roundtrip.cpp)
target_link_libraries(test_roundtrip PRIVATE huffman_core)
add_test(NAME roundtrip COMMAND test_roundtrip WORKING_DIRECTORY ${TEST_WORK_DIR})

add_executable(test_bitWriter tests/test_bitWriter.cpp)
target_link_libraries(test_bitWriter PRIVATE huffman_core)
add_test(NAME bitWriter COMMAND test_bitWriter WORKING_DIRECTORY ${TEST_WORK_DIR})

add_executable(test_compress tests/test_compress.cpp)
target_link_libraries(test_compress PRIVATE huffman_core)
add_test(NAME compress COMMAND test_compress WORKING_DIRECTORY ${TEST_WORK_DIR})

add_executable(test_minheap tests/test_minheap.cpp)
target_link_libraries(test_minheap PRIVATE huffman_core)
add_test(NAME minheap COMMAND test_minheap WORKING_DIRECTORY ${TEST_WORK_DIR})

add_executable(test_menu tests/test_menu.cpp)
target_link_libraries(test_menu PRIVATE appCore huffman_core consoleUI)
add_test(NAME menu COMMAND test_menu WORKING_DIRECTORY ${TEST_WORK_DIR})

add_executable(testT10 tests/testT10.cpp)
target_link_libraries(testT10 PRIVATE t10Core)
add_test(NAME testT10 COMMAND testT10 WORKING_DIRECTORY ${TEST_WORK_DIR})

add_executable(verify_samples tests/verify_samples.cpp)
target_link_libraries(verify_samples PRIVATE huffman_core t10Core)
add_test(NAME verify_samples COMMAND verify_samples WORKING_DIRECTORY ${TEST_WORK_DIR})

add_executable(testHuffmanCore tests/test_huffmancore.cpp)
target_link_libraries(testHuffmanCore PRIVATE huffman_core)
add_test(NAME huffmanCore COMMAND testHuffmanCore WORKING_DIRECTORY ${TEST_WORK_DIR})
