#include <iostream>
#include <vector>
#include <cassert>
#include <string>
using namespace std;

// ====== 切换 rebuild 模式 ======
// #define OPTIMIZED_REBUILD 1
#define NAIVE_REBUILD 1

class ResizableArray {
private:
    int r;
    int B;
    int N;

    vector<void*> A;
    vector<int> n;

    // ===== 空间统计 =====
    long long rebuildExtra = 0;
    long long rebuildPeak = 0;

    // ===== 工具 =====
    int blockSize(int level) const {
        int res = 1;
        for (int i = 0; i < level; ++i) res *= B;
        return res;
    }

    int* allocateBlock(int size) {
        return new int[size];
    }

    void deallocateBlock(void* p) {
        delete[] (int*)p;
    }

    int* getBlock(int level, int idx) {
        return ((int**)A[level])[idx];
    }

    void setBlock(int level, int idx, int* blk) {
        ((int**)A[level])[idx] = blk;
    }

    void copy(int* src, int s, int* dst, int d, int len) {
        for (int i = 0; i < len; ++i)
            dst[d + i] = src[s + i];
    }

    void cleanup() {
        for (int i = 1; i < r; ++i) {
            for (int j = 0; j < n[i]; ++j)
                deallocateBlock(getBlock(i, j));
        }
        for (int i = 1; i < r; ++i) {
            delete[] (void**)A[i];
        }
    }

    // ===== rebuild =====
    void rebuild(int newB) {

#ifdef NAIVE_REBUILD
        // ❌ O(N) 额外空间
        rebuildExtra = N;
        rebuildPeak = max(rebuildPeak, rebuildExtra);

        vector<int> old;
        old.reserve(N);
        for (int i = 0; i < N; ++i) old.push_back(access(i));

        cleanup();

        B = newB;
        N = 0;
        n.assign(r, 0);

        A.resize(r);
        A[0] = nullptr;
        for (int i = 1; i < r; ++i)
            A[i] = new void*[2 * B];

        for (int x : old) grow(x);

        rebuildExtra = 0;
#endif

#ifdef OPTIMIZED_REBUILD
        // ✅ O(1) 额外空间
        rebuildExtra = 0;

        auto oldA = A;
        auto oldn = n;
        int oldB = B;

        B = newB;
        N = 0;
        n.assign(r, 0);

        A.resize(r);
        A[0] = nullptr;
        for (int i = 1; i < r; ++i)
            A[i] = new void*[2 * B];

        for (int level = r - 1; level >= 1; --level) {
            int sz = 1;
            for (int k = 0; k < level; ++k) sz *= oldB;

            for (int j = 0; j < oldn[level]; ++j) {
                int* blk = (int*)(((void**)oldA[level])[j]);

                for (int k = 0; k < sz; ++k) {
                    grow(blk[k]);
                }

                delete[] blk;
            }
        }

        for (int i = 1; i < r; ++i)
            delete[] (void**)oldA[i];
#endif
    }

    void combineBlocks() {
        int k = -1;
        for (int i = 1; i < r; ++i) {
            if (n[i] < 2 * B) {
                k = i;
                break;
            }
        }

        for (int i = k - 1; i >= 1; --i) {
            int newSize = blockSize(i + 1);
            int smallSize = blockSize(i);

            int* big = allocateBlock(newSize);

            for (int j = 0; j < B; ++j) {
                int* blk = getBlock(i, j);
                copy(blk, 0, big, j * smallSize, smallSize);
                deallocateBlock(blk);
            }

            setBlock(i + 1, n[i + 1], big);

            for (int j = 0; j < B && (B + j) < n[i]; ++j)
                setBlock(i, j, getBlock(i, B + j));

            n[i] = B;
            n[i + 1]++;
        }
    }

public:
    ResizableArray(int r_ = 4) {
        r = r_;
        B = 2;
        N = 0;

        n.assign(r, 0);
        A.resize(r);

        A[0] = nullptr;
        for (int i = 1; i < r; ++i)
            A[i] = new void*[2 * B];
    }

    ~ResizableArray() {
        cleanup();
    }

    void grow(int x) {
        long long limit = 1;
        for (int i = 0; i < r; ++i) limit *= B;

        if (N == limit) rebuild(2 * B);

        if (n[1] == 2 * B && n[0] == B)
            combineBlocks();

        if (n[1] == 0 || n[0] == B) {
            int* blk = new int[blockSize(1)];
            setBlock(1, n[1], blk);
            n[1]++;
            n[0] = 0;
        }

        int* last = getBlock(1, n[1] - 1);
        last[n[0]] = x;

        n[0]++;
        N++;
    }

    int access(int idx) const {
        int remaining = idx;

        for (int level = r - 1; level >= 1; --level) {
            int sz = blockSize(level);
            int total = n[level] * sz;

            if (remaining < total) {
                int b = remaining / sz;
                int off = remaining % sz;
                return ((int**)A[level])[b][off];
            }
            remaining -= total;
        }
        return -1;
    }

    // ===== 空间统计 =====

    long long totalCapacity() const {
        long long cap = 0;
        for (int i = 1; i < r; ++i)
            cap += (long long)n[i] * blockSize(i);
        return cap;
    }

    long long slack() const {
        return totalCapacity() - N;
    }

    long long pointerSpace() const {
        return (long long)(r - 1) * (2 * B);
    }

    void printStats() const {
        cout << "N=" << N
             << " | Cap=" << totalCapacity()
             << " | Slack=" << slack()
             << " | Ptr=" << pointerSpace()
             << " | RebuildPeak=" << rebuildPeak
             << " | Ratio=" << (double)totalCapacity() / max(1, N)
             << endl;
    }
};

// ===== 主程序 =====

int main() {
    ResizableArray arr(4);

    cout << "=== GROW TEST ===\n";

    for (int i = 0; i < 5000000; ++i) {
        arr.grow(i);

        if (i % 1000 == 0)
            arr.printStats();
    }

    return 0;
}