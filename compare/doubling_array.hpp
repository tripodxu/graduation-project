#pragma once
#include <cassert>
#include <algorithm>

class DoublingArray {
private:
    int* arr = nullptr;
    int N = 0;
    int cap = 0;
    long long currentWords = 0;
    long long peakWords = 0;

    void noteAlloc(long long w) { currentWords += w; peakWords = std::max(peakWords, currentWords); }
    void noteFree(long long w) { currentWords -= w; if (currentWords < 0) currentWords = 0; }

    void reserveExact(int newCap) {
        int* next = new int[newCap];
        noteAlloc(newCap);
        for (int i = 0; i < N; ++i) next[i] = arr[i];
        if (arr) {
            delete[] arr;
            noteFree(cap);
        }
        arr = next;
        cap = newCap;
    }

public:
    DoublingArray() = default;
    ~DoublingArray() {
        if (arr) {
            delete[] arr;
            noteFree(cap);
        }
    }

    int size() const { return N; }
    int capacity() const { return cap; }
    long long allocatedWords() const { return currentWords; }
    long long peakAllocatedWords() const { return peakWords; }
    long long extraWords() const { return currentWords - N; }
    double utilization() const { return currentWords == 0 ? 1.0 : double(N) / double(currentWords); }
    void resetPeak() { peakWords = currentWords; }

    void grow(int x) {
        if (N == cap) {
            int newCap = (cap == 0 ? 1 : cap * 2);
            reserveExact(newCap);
        }
        arr[N++] = x;
    }

    void shrink() {
        if (N == 0) return;
        --N;
        if (cap > 1 && N <= cap / 4) {
            reserveExact(cap / 2);
        }
    }

    int access(int idx) const {
        assert(idx >= 0 && idx < N);
        return arr[idx];
    }

    void set(int idx, int value) {
        assert(idx >= 0 && idx < N);
        arr[idx] = value;
    }
};
