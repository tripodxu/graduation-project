#include <iostream>
#include <vector>
#include <cassert>
#include <string>
using namespace std;

class ResizableArray {
private:
    int r;          // 层数
    int B;          // 基数
    int N;          // 元素总数

    vector<void*> A;   // 每层的指针数组
    vector<int> n;     // n[0]：第1层最后一个块的已用元素数；n[i] (i>=1)：第i层块数
    bool debugMode;    // 调试开关

    vector<string> rebuildLog;   // 记录 B 变化历史

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

    // 每层真实元素数
    int levelItemCount(int level) const {
        if (level == 1) {
            if (n[1] == 0) return 0;
            return (n[1] - 1) * blockSize(1) + n[0];
        }
        return n[level] * blockSize(level);
    }

    // 根据当前结构反推元素总数
    int computedSize() const {
        int total = 0;
        for (int level = 1; level < r; ++level) {
            total += levelItemCount(level);
        }
        return total;
    }

    // ========== 清理 ==========
    void cleanup() {
        for (int i = 1; i < r; ++i) {
            if (A[i]) {
                for (int j = 0; j < n[i]; ++j) {
                    deallocateBlock(getBlock(i, j));
                }
                delete[] (void**)A[i];
                A[i] = nullptr;
            }
        }
    }

    // ========== rebuild ==========
    void rebuild(int newB) {
        if (newB != B) {
            rebuildLog.push_back(
                "B changed from " + to_string(B) +
                " to " + to_string(newB) +
                " (N = " + to_string(N) + ")"
            );
        }

        // 保存旧数据
        vector<int> old;
        old.reserve(N);
        for (int i = 0; i < N; ++i) old.push_back(access(i));

        cleanup();

        B = newB;
        N = 0;
        n.assign(r, 0);

        A.assign(r, nullptr);
        A[0] = nullptr;
        for (int i = 1; i < r; ++i)
            A[i] = new void*[2 * B];

        // 重新插入所有元素
        for (int x : old) grow(x);
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

            // 合并前 B 个块
            for (int j = 0; j < B; ++j) {
                int* blk = getBlock(i, j);
                copy(blk, 0, big, j * smallSize, smallSize);
                deallocateBlock(blk);
            }

            setBlock(i + 1, n[i + 1], big);

            // 后 B 个块前移
            for (int j = 0; j < B && (B + j) < n[i]; ++j) {
                setBlock(i, j, getBlock(i, B + j));
            }

            n[i] = B;
            n[i + 1]++;
        }

        // combine 调用条件保证第1层原来是 2B 个满块
        // 合并后第1层剩下 B 个满块
        if (n[1] > 0) n[0] = B;

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

            for (int j = 0; j < B; ++j) {
                int* blk = allocateBlock(smallSize);
                copy(big, j * smallSize, blk, 0, smallSize);
                setBlock(i, j, blk);
            }

            n[i] = B;
            deallocateBlock(big);
        }

        if (n[1] > 0) n[0] = B;

        if (debugMode) {
            printN("After split");
            cout << "-------------\n";
        }
    }

    void check() const {
        assert(n[0] >= 0 && n[0] <= B);
        for (int i = 1; i < r; ++i) {
            assert(n[i] >= 0 && n[i] <= 2 * B);
        }
        assert(computedSize() == N);
    }

public:
    ResizableArray(int r_ = 3, bool debug = false)
        : r(r_), debugMode(debug) {
        assert(r >= 2);
        B = 2;
        N = 0;
        n.assign(r, 0);

        A.assign(r, nullptr);
        A[0] = nullptr;
        for (int i = 1; i < r; ++i)
            A[i] = new void*[2 * B];
    }

    ~ResizableArray() {
        cleanup();
    }

    void setDebug(bool on) { debugMode = on; }

    void printRebuildLog() const {
        cout << "\n=== B change history ===\n";
        for (const auto& entry : rebuildLog) {
            cout << entry << '\n';
        }
        cout << "========================\n";
    }

    int totalCapacity() const {
        int cap = 0;
        for (int i = 1; i < r; ++i) {
            cap += n[i] * blockSize(i);
        }
        return cap;
    }

    void grow(int x) {
        long long limit = 1;
        for (int i = 0; i < r; ++i) limit *= B;

        if (N == limit) {
            rebuild(2 * B);
        }

        if (n[1] == 2 * B && n[0] == B) {
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

        if (B > 2) {
            long long threshold = 1;
            for (int i = 0; i < r; ++i) threshold *= (B / 4);
            if (N == threshold) {
                rebuild(B / 2);
            }
        }

        if (n[1] == 0) {
            splitBlocks();
        }

        n[0]--;
        N--;

        if (n[0] == 0) {
            if (n[1] > 0) {
                deallocateBlock(getBlock(1, n[1] - 1));
                n[1]--;
                if (n[1] > 0) {
                    n[0] = blockSize(1);
                } else {
                    n[0] = 0;
                }
            }
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

                if (level == 1 && b == n[1] - 1) {
                    assert(off < n[0]);
                }

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

                if (level == 1 && b == n[1] - 1) {
                    assert(off < n[0]);
                }

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

int main() {
    ResizableArray arr(4, true);

    cout << "--- Growing ---\n";
    for (int i = 0; i < 300; ++i) {
        arr.grow(i);
        arr.print(false);
    }

    // arr.print(true);

    cout << "\nCheck access:\n";
    cout << "arr[0]   = " << arr.access(0) << "\n";
    cout << "arr[10]  = " << arr.access(10) << "\n";
    cout << "arr[299] = " << arr.access(299) << "\n";

    arr.set(10, 9999);
    cout << "after set, arr[10] = " << arr.access(10) << "\n";

    cout << "\n--- Shrinking ---\n";
    for (int i = 0; i < 120; ++i) {
        arr.shrink();
    }

    // arr.print(true);
    arr.printRebuildLog();

    return 0;
}