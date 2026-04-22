#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <algorithm>

class ResizableArrayR {
private:
    int r;          // number of levels
    int B;          // base
    int N;          // number of stored elements

    std::vector<void*> A;   // A[level] is an array of int* pointers
    std::vector<int> n;     // n[0]: fill count of last level-1 block; n[i>=1]: number of blocks at level i
    bool debugMode;

    std::vector<std::string> rebuildLog;

    long long currentWords = 0; // tracked data-structure words currently allocated
    long long peakWords = 0;    // max tracked words ever allocated

    int blockSize(int level) const {
        int res = 1;
        for (int i = 0; i < level; ++i) res *= B;
        return res;
    }

    void noteAlloc(long long words) {
        currentWords += words;
        if (currentWords > peakWords) peakWords = currentWords;
    }

    void noteFree(long long words) {
        currentWords -= words;
        if (currentWords < 0) currentWords = 0;
    }

    int* allocateBlock(int size) {
        noteAlloc(size);
        return new int[size];
    }

    void deallocateBlock(void* p, int size) {
        if (p) {
            delete[] (int*)p;
            noteFree(size);
        }
    }

    void** allocateIndexArray(int len) {
        noteAlloc(len);
        return new void*[len];
    }

    void deallocateIndexArray(void* p, int len) {
        if (p) {
            delete[] (void**)p;
            noteFree(len);
        }
    }

    int* getBlock(int level, int idx) {
        return ((int**)A[level])[idx];
    }

    const int* getBlock(int level, int idx) const {
        return ((int* const*)A[level])[idx];
    }

    void setBlock(int level, int idx, int* blk) {
        ((int**)A[level])[idx] = blk;
    }

    void copy(int* src, int s, int* dst, int d, int len) {
        for (int i = 0; i < len; ++i) dst[d + i] = src[s + i];
    }

    int levelItemCount(int level) const {
        if (level == 1) {
            if (n[1] == 0) return 0;
            return (n[1] - 1) * blockSize(1) + n[0];
        }
        return n[level] * blockSize(level);
    }

    int computedSize() const {
        int total = 0;
        for (int level = 1; level < r; ++level) total += levelItemCount(level);
        return total;
    }

    void cleanup() {
        for (int i = 1; i < r; ++i) {
            if (A[i]) {
                for (int j = 0; j < n[i]; ++j) {
                    deallocateBlock(getBlock(i, j), blockSize(i));
                }
                deallocateIndexArray(A[i], 2 * B);
                A[i] = nullptr;
            }
        }
    }

    void rebuild(int newB) {
        if (newB != B) {
            rebuildLog.push_back(
                "B changed from " + std::to_string(B) +
                " to " + std::to_string(newB) +
                " (N = " + std::to_string(N) + ")"
            );
        }

        std::vector<int> old;
        old.reserve(N);
        for (int i = 0; i < N; ++i) old.push_back(access(i));

        cleanup();

        B = newB;
        N = 0;
        n.assign(r, 0);

        A.assign(r, nullptr);
        A[0] = nullptr;
        for (int i = 1; i < r; ++i) A[i] = allocateIndexArray(2 * B);

        for (int x : old) grow(x);
    }

    void combineBlocks() {
        int k = -1;
        for (int i = 1; i < r; ++i) {
            if (n[i] < 2 * B) {
                k = i;
                break;
            }
        }
        assert(k != -1);

        for (int i = k - 1; i >= 1; --i) {
            int newSize = blockSize(i + 1);
            int smallSize = blockSize(i);
            int* big = allocateBlock(newSize);

            for (int j = 0; j < B; ++j) {
                int* blk = getBlock(i, j);
                copy(blk, 0, big, j * smallSize, smallSize);
                deallocateBlock(blk, smallSize);
            }

            setBlock(i + 1, n[i + 1], big);
            for (int j = 0; j < B && (B + j) < n[i]; ++j) setBlock(i, j, getBlock(i, B + j));

            n[i] = B;
            n[i + 1]++;
        }

        if (n[1] > 0) n[0] = B;
    }

    void splitBlocks() {
        int k = -1;
        for (int i = 2; i < r; ++i) {
            if (n[i] > 0) {
                k = i;
                break;
            }
        }
        assert(k != -1);

        for (int i = k - 1; i >= 1; --i) {
            n[i + 1]--;
            int* big = getBlock(i + 1, n[i + 1]);
            int smallSize = blockSize(i);

            for (int j = 0; j < B; ++j) {
                int* blk = allocateBlock(smallSize);
                copy(big, j * smallSize, blk, 0, smallSize);
                setBlock(i, j, blk);
            }

            n[i] = B;
            deallocateBlock(big, blockSize(i + 1));
        }

        if (n[1] > 0) n[0] = B;
    }

    void check() const {
        assert(n[0] >= 0 && n[0] <= B);
        for (int i = 1; i < r; ++i) assert(n[i] >= 0 && n[i] <= 2 * B);
        assert(computedSize() == N);
    }

public:
    explicit ResizableArrayR(int r_ = 3, bool debug = false)
        : r(r_), debugMode(debug) {
        assert(r >= 2);
        B = 2;
        N = 0;
        n.assign(r, 0);
        A.assign(r, nullptr);
        A[0] = nullptr;
        for (int i = 1; i < r; ++i) A[i] = allocateIndexArray(2 * B);
    }

    ~ResizableArrayR() { cleanup(); }

    void resetPeak() { peakWords = currentWords; }
    long long allocatedWords() const { return currentWords; }
    long long peakAllocatedWords() const { return peakWords; }
    long long extraWords() const { return currentWords - N; }
    double utilization() const { return currentWords == 0 ? 1.0 : double(N) / double(currentWords); }
    int size() const { return N; }
    int base() const { return B; }
    const std::vector<std::string>& getRebuildLog() const { return rebuildLog; }

    void grow(int x) {
        long long limit = 1;
        for (int i = 0; i < r; ++i) limit *= B;

        if (N == limit) rebuild(2 * B);
        if (n[1] == 2 * B && n[0] == B) combineBlocks();

        if (n[1] == 0 || n[0] == B) {
            int* blk = allocateBlock(blockSize(1));
            setBlock(1, n[1], blk);
            n[1]++;
            n[0] = 0;
        }

        int* last = getBlock(1, n[1] - 1);
        last[n[0]] = x;
        n[0]++;
        N++;
        check();
    }

    void shrink() {
        if (N == 0) return;

        if (B > 2) {
            long long threshold = 1;
            for (int i = 0; i < r; ++i) threshold *= (B / 4);
            if (N == threshold) rebuild(B / 2);
        }

        if (n[1] == 0) splitBlocks();

        n[0]--;
        N--;

        if (n[0] == 0 && n[1] > 0) {
            deallocateBlock(getBlock(1, n[1] - 1), blockSize(1));
            n[1]--;
            if (n[1] > 0) n[0] = blockSize(1);
            else n[0] = 0;
        }

        check();
    }

    int access(int idx) const {
        assert(idx >= 0 && idx < N);
        int remaining = idx;
        for (int level = r - 1; level >= 1; --level) {
            int sz = blockSize(level);
            int total = levelItemCount(level);
            if (remaining < total) {
                int b = remaining / sz;
                int off = remaining % sz;
                if (level == 1 && b == n[1] - 1) assert(off < n[0]);
                return getBlock(level, b)[off];
            }
            remaining -= total;
        }
        assert(false);
        return -1;
    }

    void set(int idx, int value) {
        assert(idx >= 0 && idx < N);
        int remaining = idx;
        for (int level = r - 1; level >= 1; --level) {
            int sz = blockSize(level);
            int total = levelItemCount(level);
            if (remaining < total) {
                int b = remaining / sz;
                int off = remaining % sz;
                if (level == 1 && b == n[1] - 1) assert(off < n[0]);
                getBlock(level, b)[off] = value;
                return;
            }
            remaining -= total;
        }
        assert(false);
    }
};
