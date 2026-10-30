#include "huffmanTree.h"
#include "canonicalHuffman.h"
#include "compressor.h"
#include "fileInspector.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

struct Recorder final : TreeBuildListener {
    unsigned seeds = 0, merges = 0, pops = 0, ties = 0;
    std::uint64_t finalFrequency = 0;
    void onSeed(const HuffmanNode&) override { ++seeds; }
    void onPop(const HuffmanNode& a, const HuffmanNode& b) override {
        assert(a.freq <= b.freq);
        ++pops;
    }
    void onTieBreak(const HuffmanNode& a, const HuffmanNode& b) override {
        assert(a.freq == b.freq && a.minSymbol < b.minSymbol);
        ++ties;
    }
    void onMerge(const HuffmanNode& node) override {
        assert(!node.isLeaf && node.freq == node.left->freq + node.right->freq);
        finalFrequency = node.freq; ++merges;
    }
};
int main() {
    std::uint64_t freq[256]{};
    freq['A']=4; freq['B']=4; freq['C']=2; freq['D']=1;
    std::uint64_t count=0;
    Recorder trace;
    auto root=buildHuffmanTree(freq, count, &trace);
    assert(root && count == 11 && root->freq == trace.finalFrequency);
    assert(trace.seeds == 4 && trace.pops == 3 && trace.merges == 3);
    assert(trace.ties >= 1);
    std::uint8_t lengths[256]{};
    assert(getHuffmanCodeLengths(root,lengths));
    std::vector<CanonicalCode> codes;
    assert(buildCanonicalCodes(lengths,codes) && codes.size() == 4);
    std::uint64_t one[256]{}; one['X']=12;
    Recorder single;
    auto oneRoot=buildHuffmanTree(one,count,&single);
    assert(oneRoot->isLeaf && single.seeds==1 && single.merges==0);
    assert(getHuffmanCodeLengths(oneRoot,lengths) && lengths['X']==1);
    const auto dir=std::filesystem::temp_directory_path()/"type_foundry_inspector_test";
    std::filesystem::create_directories(dir);
    auto src=dir/"sample.txt", dst=dir/"sample.bin";
    {std::ofstream out(src,std::ios::binary); for(int i=0;i<3000;++i) out<<"AAB";}
    Compressor compressor;
    assert(compressor.compress(src.string(),dst.string()));
    auto info=inspectFile(dst.string());
    assert(info.readable && info.validHeader && info.version==3);
    assert(info.originalLength==9000 && info.fileSize==std::filesystem::file_size(dst));
    assert(info.mode==0 && info.symbolCount==2);
    assert(info.codeLengths['A']>0 && info.codeLengths['B']>0);
    assert(info.segments.size()==5+2); // magic, mode, length, crc, unique, two pairs
    {std::ofstream bad(dir/"bad.huf",std::ios::binary); bad<<"HUF3";}
    auto corrupt=inspectFile((dir/"bad.huf").string());
    assert(corrupt.readable && !corrupt.validHeader);
    std::filesystem::remove_all(dir);
}
