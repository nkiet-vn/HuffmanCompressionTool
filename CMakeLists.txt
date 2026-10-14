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