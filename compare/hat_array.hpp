#pragma once
#include <vector>
#include <cassert>
#include <cmath>
#include <algorithm>

class HATArray {
private:
    int N = 0;
    int B = 2;
    std::vector<int*> blocks;
    int** indexBlock = nullptr;
    int indexCap = 0;      // allocated pointer capacity in index block
    long long currentWords = 0;
    long long peakWords = 0;

    void noteAlloc(long long w) { currentWords += w; peakWords = std::max(peakWords, currentWords); }
    void noteFree(long long w) { currentWords -= w; if (currentWords < 0) currentWords = 0; }

    void rebuild(int newB) {
        std::vector<int> old;
        old.reserve(N);
        for (int i = 0; i < N; ++i) old.push_back(access(i));

        cleanup();
        B = std::max(2, newB);
        allocateIndexBlock(B);
        for (int x : old) grow(x);
    }

    void allocateIndexBlock(int cap) {
        indexCap = cap;
        indexBlock = new int*[indexCap];
        noteAlloc(indexCap);
        blocks.clear();
        blocks.reserve(indexCap);
    }

    void cleanup() {
        for (int* p : blocks) {
            delete[] p;
            noteFree(B);
        }
        blocks.clear();
        if (indexBlock) {
            delete[] indexBlock;
            noteFree(indexCap);
            indexBlock = nullptr;
            indexCap = 0;
        }
        N = 0;
    }

    int blockCountNeeded() const {
        return (N + B - 1) / B;
    }

    void maybeGrowIndex() {
        if ((int)blocks.size() < indexCap) return;
        int newCap = std::max(indexCap * 2, B);
        int** newIndex = new int*[newCap];
        noteAlloc(newCap);
        for (int i = 0; i < (int)blocks.size(); ++i) newIndex[i] = indexBlock[i];
        delete[] indexBlock;
        noteFree(indexCap);
        indexBlock = newIndex;
        indexCap = newCap;
    }

public:
    HATArray() {
        allocateIndexBlock(B);
    }

    ~HATArray() { cleanup(); }

    int size() const { return N; }
    int base() const { return B; }
    long long allocatedWords() const { return currentWords; }
    long long peakAllocatedWords() const { return peakWords; }
    long long extraWords() const { return currentWords - N; }
    double utilization() const { return currentWords == 0 ? 1.0 : double(N) / double(currentWords); }
    void resetPeak() { peakWords = currentWords; }

    void grow(int x) {
        if (N == B * B) {
            rebuild(B * 2);
        }

        int usedBlocks = blockCountNeeded();
        bool needNewBlock = (N == 0) || (N % B == 0);
        if (needNewBlock) {
            maybeGrowIndex();
            int* blk = new int[B];
            noteAlloc(B);
            indexBlock[blocks.size()] = blk;
            blocks.push_back(blk);
        }

        int bi = N / B;
        int off = N % B;
        indexBlock[bi][off] = x;
        ++N;
    }

    void shrink() {
        if (N == 0) return;
        if (B >= 4 && N == (B / 4) * (B / 4)) {
            rebuild(B / 2);
            if (N == 0) return;
        }

        --N;
        if (N % B == 0 && !blocks.empty()) {
            delete[] blocks.back();
            noteFree(B);
            blocks.pop_back();
        }
    }

    int access(int idx) const {
        assert(idx >= 0 && idx < N);
        int bi = idx / B;
        int off = idx % B;
        return indexBlock[bi][off];
    }

    void set(int idx, int value) {
        assert(idx >= 0 && idx < N);
        int bi = idx / B;
        int off = idx % B;
        indexBlock[bi][off] = value;
    }
};
