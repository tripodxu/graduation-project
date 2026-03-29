#include <iostream>
#include <vector>
#include <cassert>
using namespace std;

class ResizableArray {
private:
    int r;          // 层数
    int B;          // 基数
    int N;          // 元素总数

    vector<void*> A;   // 每层的指针数组
    vector<int> n;     // 每层已使用的块数
    bool debugMode;    // 调试开关

    // ========== 工具函数 ==========
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

    // 非 const 版本：返回可修改的指针
    int* getBlock(int level, int idx) {
        return ((int**)A[level])[idx];
    }

    // const 版本：返回指向常量的指针
    const int* getBlock(int level, int idx) const {
        return (const int*)(((const void* const*)A[level])[idx]);
    }

    void setBlock(int level, int idx, int* blk) {
        ((int**)A[level])[idx] = blk;
    }

    void copy(int* src, int s, int* dst, int d, int len) {
        for (int i = 0; i < len; ++i)
            dst[d + i] = src[s + i];
    }

    void printN(const string& prefix) const {
        if (!debugMode) return;
        cout << prefix << " n=[";
        for (int i = 0; i < r; ++i) {
            if (i) cout << ",";
            cout << n[i];
        }
        cout << "]\n";
    }

    // ========== 清理 ==========
    void cleanup() {
        for (int i = 1; i < r; ++i) {
            for (int j = 0; j < n[i]; ++j)
                deallocateBlock(getBlock(i, j));
        }
        for (int i = 1; i < r; ++i) {
            if (A[i]) delete[] (void**)A[i];
        }
    }

    // ========== rebuild ==========
    void rebuild(int newB) {
        if (debugMode) {
            cout << "=== REBUILD from B=" << B << " to B=" << newB << " ===\n";
            printN("Before rebuild");
        }

        // 保存旧数据
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

        // 重新插入所有元素
        for (int x : old) grow(x);

        if (debugMode) {
            printN("After rebuild");
            cout << "==================\n";
        }
    }

    // ========== combine ==========
    void combineBlocks() {
        if (debugMode) {
            cout << "--- COMBINE ---\n";
            printN("Before combine");
        }

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

            // 合并前 B 个小块
            for (int j = 0; j < B; ++j) {
                int* blk = getBlock(i, j);
                copy(blk, 0, big, j * smallSize, smallSize);
                deallocateBlock(blk);
            }

            setBlock(i + 1, n[i + 1], big);

            // 将剩余的小块前移
            for (int j = 0; j < B && (B + j) < n[i]; ++j) {
                setBlock(i, j, getBlock(i, B + j));
            }

            n[i] = B;
            n[i + 1]++;
        }

        if (debugMode) {
            printN("After combine");
            cout << "---------------\n";
        }
    }

    // ========== split ==========
    void splitBlocks() {
        if (debugMode) {
            cout << "--- SPLIT ---\n";
            printN("Before split");
        }

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

            // 将一个大块拆分成 B 个小块
            for (int j = 0; j < B; ++j) {
                int* blk = allocateBlock(smallSize);
                copy(big, j * smallSize, blk, 0, smallSize);
                setBlock(i, j, blk);
            }

            n[i] = B;
            deallocateBlock(big);
        }

        if (n[1] > 0) n[0] = B;   // 保证 level0 计数器正确

        if (debugMode) {
            printN("After split");
            cout << "-------------\n";
        }
    }

    // ========== 不变式检查 ==========
    void check() const {
        assert(n[0] >= 0 && n[0] <= B);
        for (int i = 1; i < r; ++i) {
            assert(n[i] >= 0 && n[i] <= 2 * B);
        }
    }

public:
    // 构造函数
    ResizableArray(int r_ = 3, bool debug = false)
        : r(r_), debugMode(debug) {
        B = 2;
        N = 0;
        n.assign(r, 0);

        A.resize(r);
        A[0] = nullptr;
        for (int i = 1; i < r; ++i)
            A[i] = new void*[2 * B];
    }

    // 析构函数
    ~ResizableArray() {
        cleanup();
    }

    void setDebug(bool on) { debugMode = on; }

    // ========== grow ==========
    void grow(int x) {
        long long limit = 1;
        for (int i = 0; i < r; ++i) limit *= B;

        if (N == limit) {
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

    // ========== shrink ==========
    void shrink() {
        if (N == 0) return;

        // 判断是否需要缩小 B
        if (B > 2) {
            long long threshold = 1;
            for (int i = 0; i < r; ++i) threshold *= (B / 4);
            if (N == threshold) {
                rebuild(B / 2);
            }
        }

        // 如果 level1 没有块，需要拆分
        if (n[1] == 0) {
            splitBlocks();
        }

        // 删除最后一个元素
        n[0]--;
        N--;

        // 如果当前块空了，释放
        if (n[0] == 0) {
            if (n[1] > 0) {
                deallocateBlock(getBlock(1, n[1] - 1));
                n[1]--;
                if (n[1] > 0) {
                    n[0] = blockSize(1);   // = B
                } else {
                    n[0] = 0;
                }
            }
        }

        check();
    }

    // ========== access (const) ==========
    int access(int idx) const {
        assert(idx >= 0 && idx < N);

        int remaining = idx;
        for (int level = r - 1; level >= 1; --level) {
            int sz = blockSize(level);
            int total = n[level] * sz;

            if (remaining < total) {
                int b = remaining / sz;
                int off = remaining % sz;
                return getBlock(level, b)[off];
            }
            remaining -= total;
        }

        assert(false);
        return -1;
    }

    // ========== set (非 const) ==========
    void set(int idx, int value) {
        assert(idx >= 0 && idx < N);

        int remaining = idx;
        for (int level = r - 1; level >= 1; --level) {
            int sz = blockSize(level);
            int total = n[level] * sz;

            if (remaining < total) {
                int b = remaining / sz;
                int off = remaining % sz;
                getBlock(level, b)[off] = value;
                return;
            }
            remaining -= total;
        }

        assert(false);
    }

    int size() const { return N; }

    void print(bool showElements = false) const {
        cout << "N=" << N << ", B=" << B << ", n=[";
        for (int i = 0; i < r; ++i) {
            if (i) cout << ",";
            cout << n[i];
        }
        cout << "]\n";

        if (showElements) {
            cout << "Elements: ";
            for (int i = 0; i < N; ++i) {
                cout << access(i) << " ";
            }
            cout << endl;
        }
    }
};

// ========== 测试示例 ==========
int main() {
    ResizableArray arr(4, true);   // 开启调试模式

    // 测试 grow
    cout << "\n--- Testing grow ---\n";
    for (int i = 0; i < 1000; ++i) {
        arr.grow(i);
        arr.print(false);
    }

    // 测试 shrink
    cout << "\n--- Testing shrink ---\n";
    for (int i = 0; i < 1000; ++i) {
        arr.shrink();
        arr.print(false);
    }

    // // 测试 set 赋值
    // cout << "\n--- Testing set ---\n";
    // arr.print(true);
    // cout << "Set index 5 to 999\n";
    // arr.set(5, 999);
    // arr.print(true);

    // // 再次 grow 观察 combine
    // cout << "\n--- Continue grow to trigger combine ---\n";
    // for (int i = 20; i < 50; ++i) {
    //     arr.grow(i);
    // }
    // arr.print(true);

    return 0;
}