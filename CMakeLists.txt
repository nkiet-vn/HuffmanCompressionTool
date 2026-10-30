cmake_minimum_required(VERSION 3.16)
project(HuffmanCompressionTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(huffman_core STATIC
    src/canonicalHuffman.cpp src/huffmanTree.cpp src/bitReader.cpp
    src/crc32.cpp src/fileFormat.cpp src/fileInspector.cpp
    src/compressor.cpp src/decompressor.cpp
    src/sha256.cpp src/fileHasher.cpp src/compressionStats.cpp
    src/compressionVerifier.cpp
)
target_include_directories(huffman_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(huffman_core PUBLIC cxx_std_17)
target_link_libraries(huffman_core PUBLIC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>)
# Compatibility aliases for the original tests/CLI, without duplicating implementations.
add_library(huffmanCore ALIAS huffman_core)
add_library(utilCore INTERFACE)
target_link_libraries(utilCore INTERFACE huffman_core)

add_library(consoleUi STATIC src/consoleUI.cpp)
target_include_directories(consoleUi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(consoleUi PUBLIC cxx_std_17)
target_link_libraries(consoleUi PUBLIC utilCore)

add_library(appCore STATIC src/menuController.cpp)
target_include_directories(appCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(appCore PUBLIC cxx_std_17)
target_link_libraries(appCore PUBLIC consoleUi huffmanCore)

add_executable(huffman src/main.cpp)
target_link_libraries(huffman PRIVATE huffmanCore utilCore consoleUi appCore)
install(TARGETS huffman RUNTIME DESTINATION bin)

option(HUFFMAN_BUILD_GUI "Build the native Qt 6 Type Foundry GUI" ON)
if(HUFFMAN_BUILD_GUI)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 QuickDialogs2 Concurrent Widgets)
    add_subdirectory(gui)
endif()

option(HUFFMAN_BUILD_TESTS "Build HuffmanCompressionTool tests" ON)

if(HUFFMAN_BUILD_TESTS)
    enable_testing()

    function(addHuffmanTest name source)
        set(testTarget "test_${name}")
        add_executable(${testTarget} ${source})
        target_link_libraries(${testTarget} PRIVATE ${ARGN})
        add_test(NAME ${name} COMMAND ${testTarget}
                 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endfunction()

    addHuffmanTest(trace_inspector tests/test_trace_inspector.cpp huffman_core)
    addHuffmanTest(huffmanTree tests/test_huffmanTree.cpp huffmanCore)
    addHuffmanTest(bitReader tests/test_bitReader.cpp huffmanCore)
    addHuffmanTest(roundtrip tests/test_roundtrip.cpp huffmanCore)
    addHuffmanTest(bitWriter tests/test_bitWriter.cpp huffmanCore)
    addHuffmanTest(compress tests/test_compress.cpp huffmanCore)
    addHuffmanTest(minheap tests/test_minheap.cpp huffmanCore)
    addHuffmanTest(menu tests/test_menu.cpp appCore huffmanCore consoleUi)
    addHuffmanTest(testT10 tests/testT10.cpp utilCore)
    addHuffmanTest(verify_samples tests/verify_samples.cpp huffmanCore utilCore)
    addHuffmanTest(huffmanCore tests/test_huffmancore.cpp huffmanCore)
    addHuffmanTest(huf2 tests/test_huf2.cpp huffmanCore utilCore)
    addHuffmanTest(canonicalHuffman tests/test_canonicalHuffman.cpp huffmanCore)
    addHuffmanTest(output_guard tests/test_output_guard.cpp huffmanCore)

    add_test(NAME cli_info COMMAND huffman -i ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    set_tests_properties(cli_info PROPERTIES PASS_REGULAR_EXPRESSION "Tệp thường")
    add_test(NAME cli_ascii_info COMMAND huffman --ascii -i ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    set_tests_properties(cli_ascii_info PROPERTIES PASS_REGULAR_EXPRESSION "Tep thuong")
    add_test(NAME cli_info_missing
             COMMAND huffman -i ${CMAKE_CURRENT_BINARY_DIR}/missing-cli-info-file)
    set_tests_properties(cli_info_missing PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli_info_bad_args COMMAND huffman -i)
    set_tests_properties(cli_info_bad_args PROPERTIES WILL_FAIL TRUE)
endif()
