#include <iostream>
#include <vector>
#include <cassert>
#include <string>
#include <iomanip>
#include <cmath>
using namespace std;

class ResizableArray {
private:
    int r;
    int B;
    int N;
    vector<void*> A;
    vector<int> n; // n[0]: used elems in last level-1 block; n[i>=1]: number of blocks in level i
    bool debugMode;

    int ipow(int base, int exp) const {
        int res = 1;
        for (int i = 0; i < exp; ++i) res *= base;
        return res;
    }

    int blockSize(int level) const {
        return ipow(B, level);
    }

    int* allocateBlock(int size) { return new int[size]; }
    void deallocateBlock(void* p) { delete[] (int*)p; }

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
                for (int j = 0; j < n[i]; ++j) deallocateBlock(getBlock(i, j));
                delete[] (void**)A[i];
                A[i] = nullptr;
            }
        }
    }

    void rebuild(int newB) {
        vector<int> old;
        old.reserve(N);
        for (int i = 0; i < N; ++i) old.push_back(access(i));

        cleanup();
        B = newB;
        N = 0;
        n.assign(r, 0);
        A.assign(r, nullptr);
        for (int i = 1; i < r; ++i) A[i] = new void*[2 * B]();

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
                deallocateBlock(blk);
            }
            setBlock(i + 1, n[i + 1], big);

            for (int j = B; j < n[i]; ++j) {
                setBlock(i, j - B, getBlock(i, j));
            }
            n[i] -= B;
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
            deallocateBlock(big);
        }
        if (n[1] > 0) n[0] = B;
    }

    void check() const {
        assert(n[0] >= 0 && n[0] <= B);
        for (int i = 1; i < r; ++i) {
            assert(n[i] >= 0 && n[i] <= 2 * B);
        }
        assert(computedSize() == N);
    }

public:
    explicit ResizableArray(int r_ = 3, bool debug = false)
        : r(r_), B(2), N(0), A(r_, nullptr), n(r_, 0), debugMode(debug) {
        assert(r >= 2);
        for (int i = 1; i < r; ++i) A[i] = new void*[2 * B]();
    }

    ~ResizableArray() { cleanup(); }

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
            deallocateBlock(getBlock(1, n[1] - 1));
            n[1]--;
            n[0] = (n[1] > 0 ? blockSize(1) : 0);
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

    int size() const { return N; }
    int base() const { return B; }

    long long totalElementCapacity() const {
        long long cap = 0;
        for (int i = 1; i < r; ++i) cap += 1LL * n[i] * blockSize(i);
        return cap;
    }

    long long totalBytesApprox() const {
        long long bytes = 0;
        for (int i = 1; i < r; ++i) {
            bytes += 1LL * n[i] * blockSize(i) * (long long)sizeof(int);
            bytes += 1LL * 2 * B * (long long)sizeof(void*); // pointer table at each level
        }
        return bytes;
    }
};

struct Row {
    int r;
    int N;
    int B;
    long long elemCap;
    double elemUtil;
    long long bytes;
    double byteUtil;
};

static Row measure(int r, int N) {
    ResizableArray arr(r);
    for (int i = 0; i < N; ++i) arr.grow(i);
    Row row;
    row.r = r;
    row.N = N;
    row.B = arr.base();
    row.elemCap = arr.totalElementCapacity();
    row.elemUtil = row.elemCap ? (100.0 * N / row.elemCap) : 100.0;
    row.bytes = arr.totalBytesApprox();
    row.byteUtil = row.bytes ? (100.0 * N * (double)sizeof(int) / row.bytes) : 100.0;
    return row;
}

int main() {
    vector<int> Ns = {1,2,3,4,5,6,7,8,10,12,16,20,24,32,40,48,64,80,96,128,160,192,256,320,384,512,640,768,1024,1536,2048,3072,4096};

    cout << fixed << setprecision(2);
    cout << "r,N,B,element_capacity,element_util_percent,approx_bytes,byte_util_percent\n";
    for (int r : {2,3,4}) {
        for (int N : Ns) {
            Row x = measure(r, N);
            cout << x.r << ',' << x.N << ',' << x.B << ',' << x.elemCap << ','
                 << x.elemUtil << ',' << x.bytes << ',' << x.byteUtil << "\n";
        }
    }
    return 0;
}
