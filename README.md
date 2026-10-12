# HuffmanCompressionTool

**Huffman Archiver** is a lightweight file compression and decompression application written in standard C++17. It uses **Huffman Coding**, a lossless compression algorithm, to reduce file size while preserving the original data exactly.

The application supports both text and binary files, uses a custom binary file format, and verifies data integrity during decompression.

## 1. Overview

Huffman Archiver demonstrates the practical implementation of a lossless compression system, combining data structures, bitwise operations, binary file processing, and algorithm design.

### Key Features

- **Lossless compression:** Restores the original file byte-for-byte.
- **Text and binary support:** Processes arbitrary file contents without assuming a specific file type.
- **Huffman coding:** Builds a coding tree based on byte frequencies.
- **Custom binary format:** Stores the metadata required for decompression.
- **Data integrity verification:** Uses checksums to detect corrupted or modified data.
- **Memory efficiency:** Processes files in blocks of 64 KB instead of loading the entire file into memory.
- **Error handling:** Detects invalid input, corrupted headers, truncated files, and unsupported format versions.
- **Command-line interface:** Supports compression, decompression, and inspection of compressed files.
- **Console interface:** Provides an interactive menu and operation status information.

## 2. How It Works

Huffman coding assigns variable-length binary codes to symbols according to their frequencies. More frequent symbols generally receive shorter codes, while less frequent symbols receive longer codes.

The compression process consists of the following steps:

1. **Frequency analysis:** Read the input file and count the frequency of each byte.
2. **Tree construction:** Build a Huffman tree using a priority queue.
3. **Code generation:** Traverse the tree to generate a binary code for each byte.
4. **Bit encoding:** Encode the input data using the generated codes and write the resulting bits to a binary file.
5. **Metadata storage:** Store the information required to reconstruct the coding tree and recover the original data.

During decompression, the application reads the metadata, reconstructs the Huffman tree, decodes the compressed bitstream, and verifies the integrity of the recovered data.

The fundamental correctness requirement is:

**Decompress(Compress(input)) = input**

The equality is defined at the byte level, so the recovered file does not need to be interpreted as text to verify correctness.

## 3. System Architecture

The application is organized into independent modules to separate algorithmic logic, file processing, user interaction, and testing.

| Module | Responsibility |
|---|---|
| `huffman_tree` | Builds the Huffman tree and generates symbol codes. |
| `bit_io` | Reads and writes individual bits and manages bit-level buffering. |
| `file_format` | Defines the compressed file header and metadata format. |
| `codec` | Coordinates compression and decompression operations. |
| `utils` | Provides shared utilities, such as timing and supporting functions. |
| `ui` | Implements the interactive console interface and operation reports. |
| `main` | Handles program entry, command-line arguments, and execution flow. |
| `tests` | Verifies compression, decompression, error handling, and edge cases. |
| `bench` | Measures compression and decompression performance. |

This modular design makes the application easier to maintain, test, and extend.

## 4. Technical Specifications

| Property | Description |
|---|---|
| Programming language | C++ |
| Language standard | C++17 |
| Compression algorithm | Huffman Coding |
| Compression type | Lossless |
| Input types | Text and binary files |
| Output format | Custom `.bin` archive |
| Processing strategy | Block-based I/O |
| Block size | 64 KB |
| Data integrity | Checksum verification |
| External dependencies | No third-party libraries required |
| Build systems | CMake / Make, depending on the project configuration |

**Note:** Huffman coding does not guarantee that every file will become smaller. For small files or data with an unfavorable frequency distribution, metadata and coding overhead may cause the compressed file to be larger than the original.

## 5. Requirements

To build and run the application, you need:

- A C++17-compatible compiler, such as GCC or a compatible compiler toolchain.
- CMake and/or Make, depending on the build configuration.
- A terminal or command prompt.

The project does not require third-party compression libraries.

## 6. Build and Run

Clone the repository:

```bash
git clone <repository-url>
cd HuffmanCompressionTool
```

### Build with CMake

If the repository contains a `CMakeLists.txt` file:

```bash
cmake -S . -B build
cmake --build build
```

Run the executable generated in the build directory. The exact executable path depends on the operating system and build configuration.

### Build with Make

If the repository provides a compatible `Makefile`:

```bash
make
make test
```

Use the build method supported by the checked-out project. The CMake and Make commands above assume their corresponding configuration files and targets are present.

## 7. Usage

Huffman Archiver supports command-line operations and an interactive console menu.

### Command-line interface

The following examples illustrate the intended command syntax.

**Compress a file**

```bash
huffman c input.txt -o output.bin
```

**Decompress a file**

```bash
huffman d output.bin -o restored.txt
```

**Inspect a compressed file**

```bash
huffman --info output.bin
```

Paths containing spaces should be enclosed in quotation marks:

```bash
huffman c "my document.txt" -o "compressed file.bin"
```

Replace `huffman` with the executable name or path produced by your build configuration. The commands assume that the corresponding CLI options are implemented in the current version.

### Interactive console interface

Launch the application and follow the on-screen menu to select an operation, specify file paths, and view the result.

Available operations depend on the features implemented in the current build.

## 8. Testing and Validation

Testing focuses on the correctness of the compression and decompression pipeline, as well as the application's ability to handle exceptional inputs.

Important test cases include:

- Empty files and single-byte files.
- Files containing all 256 possible byte values.
- Files containing repeated symbols.
- Text files and arbitrary binary data.
- Inputs smaller than one processing block.
- Large files and deeply structured Huffman trees.
- Truncated or malformed archive headers.
- Unsupported format versions.
- Corrupted compressed data and checksum mismatches.
- Files for which compression increases the output size.

The primary correctness criterion is byte-for-byte equality between the original file and the decompressed result.

Run the project's test suite using the supported test command, for example:

```bash
make test
```

A successful test run should report the actual results of all executed test cases. Test coverage and passing counts should be documented only after they have been verified against the current implementation.

## 9. Performance Evaluation

The `bench/` module is intended to evaluate compression and decompression throughput using representative input files.

Performance measurements can include:

- Input file size.
- Compressed file size.
- Compression ratio.
- Compression throughput in MB/s.
- Decompression throughput in MB/s.
- Execution time before and after optimization.

The compression ratio can be expressed as:

\[
R = \frac{S_c}{S_o}\times 100\%
\]

where:

- \(S_c\) is the compressed file size.
- \(S_o\) is the original file size.

A lower ratio indicates a smaller compressed output relative to the original file. Performance comparisons should use the same input data, hardware, compiler settings, and measurement procedure.

## 10. Limitations

- Huffman coding is a general-purpose lossless coding technique, not a universal guarantee of file size reduction.
- The compression ratio depends on the frequency distribution of the input bytes and the overhead of the archive format.
- The application can decompress only archives that conform to its supported file format versions.
- Checksum verification helps detect data corruption but does not provide cryptographic authentication.
- Compression and decompression performance depends on file characteristics, hardware, and compiler optimization settings.

## 11. Educational Value

Huffman Archiver provides a practical example of several fundamental computer science concepts:

- Greedy algorithms and priority queues.
- Binary trees and tree traversal.
- Frequency tables and symbol encoding.
- Bitwise operations and bit-level I/O.
- Binary file formats and metadata.
- Memory-efficient file processing.
- Error handling and data integrity verification.
- Modular software design and automated testing.

The project is suitable for studying data structures, algorithms, file processing, and the engineering principles behind a small compression utility.

## 12. Project Status

Huffman Archiver is developed as a modular C++17 project. Features, supported archive versions, test coverage, and performance results should be considered according to the implementation available in the current repository.

## License

Add the project's license information here if a license has been selected.

