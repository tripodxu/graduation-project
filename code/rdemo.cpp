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
    vector<int> n;     // 每层已使用的块数
    bool debugMode;    // 调试开关（保留但本次未使用）

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
        // 记录 B 变化
        if (newB != B) {
            rebuildLog.push_back("B changed from " + to_string(B) + " to " + to_string(newB) + " (N = " + to_string(N) + ")");
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

            for (int j = 0; j < B; ++j) {
                int* blk = getBlock(i, j);
                copy(blk, 0, big, j * smallSize, smallSize);
                deallocateBlock(blk);
            }

            setBlock(i + 1, n[i + 1], big);

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
            if(!(n[i] >= 0 && n[i] <= 2 * B)){
                cout<<"n["<<i<<"]="<<n[i]<<endl;
            }
            assert(n[i] >= 0 && n[i] <= 2 * B);
        }
    }

public:
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
            // cout<<"rebuild"<<endl;
            rebuild(2 * B);
        }
        if (n[1] == 2 * B && n[0] == B) {
            // cout<<B<<endl;
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

// int main() {
//     ResizableArray arr(4, 1);   // 关闭过程调试输出，仅记录 B 变化

//     // 测试 grow
//     cout << "--- Growing to 1000 elements ---\n";
//     for (int i = 0; i < 255; ++i) {
//         arr.grow(i);
//         arr.print(false);
//     }
//     arr.grow(255);
//     arr.print(false);

//     arr.grow(256);
//     arr.print(false);    

//     // // 测试 shrink
//     // cout << "--- Shrinking back to 0 ---\n";
//     // for (int i = 0; i < 257; ++i) {
//     //     arr.shrink();
//     //     arr.print(false);
//     // }

//     // 输出 B 变化历史
//     // arr.printRebuildLog();

//     return 0;
// }