#include <iostream>
#include <vector>
#include <cassert>
using namespace std;

class ResizableArray {
private:
    int r;
    int B;
    int N;

    vector<void*> A;   // index blocks
    vector<int> n;     // counters

    // ===== 工具 =====

    int blockSize(int level) {
        int res = 1;
        for (int i = 0; i < level; i++) res *= B;
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
        for (int i = 0; i < len; i++)
            dst[d + i] = src[s + i];
    }

    // ===== 清理 =====
    void cleanup() {
        for (int i = 1; i < r; i++) {
            for (int j = 0; j < n[i]; j++)
                deallocateBlock(getBlock(i, j));
        }
        for (int i = 1; i < r; i++) {
            if (A[i]) delete[] (void**)A[i];
        }
    }

    // ===== rebuild =====
    void rebuild(int newB) {
        vector<int> old;
        old.reserve(N);
        for (int i = 0; i < N; i++) old.push_back(access(i));

        cleanup();

        B = newB;
        N = 0;
        n.assign(r, 0);

        A.resize(r);
        A[0] = nullptr;
        for (int i = 1; i < r; i++)
            A[i] = new void*[2 * B];

        for (int x : old) grow(x);
    }

    // ===== combine =====
    void combineBlocks() {
        int k = -1;
        for (int i = 1; i < r; i++) {
            if (n[i] < 2 * B) {
                k = i;
                break;
            }
        }
        assert(k != -1);

        for (int i = k - 1; i >= 1; i--) {
            int newSize = blockSize(i + 1);
            int smallSize = blockSize(i);

            int* big = allocateBlock(newSize);

            for (int j = 0; j < B; j++) {
                int* blk = getBlock(i, j);
                copy(blk, 0, big, j * smallSize, smallSize);
                deallocateBlock(blk);
            }

            setBlock(i + 1, n[i + 1], big);

            // move tail blocks forward
            for (int j = 0; j < B && (B + j) < n[i]; j++) {
                setBlock(i, j, getBlock(i, B + j));
            }

            n[i] = B;
            n[i + 1]++;
        }
        cout << "combineBlocks" << " ";
        //打印所有参数n
        cout << "n=[" ;
        for (int i = 0; i < r; i++) {
             cout<< n[i] <<",";
        }
        cout << "]" << endl;
    }

    // ===== split（修复版，关键）=====
    void splitBlocks() {
        int k = -1;
        for (int i = 2; i < r; i++) {
            if (n[i] > 0) {
                k = i;
                break;
            }
        }
        assert(k != -1);

        for (int i = k - 1; i >= 1; i--) {
            n[i + 1]--;

            int* big = getBlock(i + 1, n[i + 1]);
            int smallSize = blockSize(i);

            // ⭐关键：覆盖写，保证 packed
            for (int j = 0; j < B; j++) {
                int* blk = allocateBlock(smallSize);
                copy(big, j * smallSize, blk, 0, smallSize);
                setBlock(i, j, blk);
            }

            n[i] = B;  // ⭐不能 +=，必须重置
            deallocateBlock(big);
        }

        if (n[1] > 0) n[0] = B;
    }

    // ===== invariant check（强烈建议保留）=====
    void check() {
        assert(n[0] >= 0 && n[0] <= B);
        for (int i = 1; i < r; i++) {
            assert(n[i] >= 0 && n[i] <= 2 * B);
        }
    }

public:
    ResizableArray(int r_ = 3) : r(r_) {
        B = 2;
        N = 0;
        n.assign(r, 0);

        A.resize(r);
        A[0] = nullptr;
        for (int i = 1; i < r; i++)
            A[i] = new void*[2 * B];
    }

    ~ResizableArray() {
        cleanup();
    }

    // ===== grow =====
    void grow(int x) {
        long long limit = 1;
        for (int i = 0; i < r; i++) limit *= B;

        if (N == limit) {
            cout << "rebuild to B=" << 2 * B << endl;
            rebuild(2 * B);
        }
        else if (n[1] == 2 * B) {
            combineBlocks();
        }

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

        // ===== 1️⃣ 先判断是否需要缩小 B =====
        if (B > 2) {
            long long threshold = 1;
            for (int i = 0; i < r; i++) threshold *= (B / 4);
            if (N == threshold) {
                rebuild(B / 2);
                return; // ⭐ rebuild 已经完成 shrink
            }
        }

        // ===== 2️⃣ 如果 level1 没有 block，需要拆分 =====
        if (n[1] == 0) {
            splitBlocks();
        }

        // ===== 3️⃣ 删除一个元素 =====
        n[0]--;
        N--;

        // ===== 4️⃣ 如果当前 block 空了，释放 =====
        if (n[0] == 0) {
            if (n[1] > 0) {
                deallocateBlock(getBlock(1, n[1] - 1));
                n[1]--;

                // ⭐关键修复：不能盲目设为 B
                if (n[1] > 0) {
                    n[0] = blockSize(1);  // = B
                } else {
                    n[0] = 0;
                }
            }
        }

        check();
    }

    // ===== access =====
    int access(int idx) {
        assert(idx >= 0 && idx < N);

        for (int level = r - 1; level >= 1; level--) {
            int sz = blockSize(level);
            int total = n[level] * sz;

            if (idx < total) {
                int b = idx / sz;
                int off = idx % sz;
                return getBlock(level, b)[off];
            }
            idx -= total;
        }

        assert(false);
        return -1;
    }

    int size() { return N; }

    void print() {
        cout << "N=" << N << ", B=" << B << ", n=[";
        for (int i = 0; i < r; i++) {
            if (i) cout << ",";
            cout << n[i];
        }
        cout << "]\n";
    }
};
int main() {
    ResizableArray arr(4); // 测试 r=3

    for (int i = 0; i < 1000; i++) {
        arr.grow(i);
        arr.print();
    }
    // for (int i = 0; i < 100; i++){
    //     arr.shrink();
    //     arr.print();
    // }
    // for (int i = 0; i < 50; i++) {arr.grow(1000+i);arr.print();}

    // for (int i = 0; i < arr.size(); i++)
    //     cout << arr.access(i) << " ";
}