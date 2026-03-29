#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

using namespace std;

// (O(r*N^{1/r}), O(N^{1-1/r})) 可调整大小数组实现，r=3
// 使用块大小 B 和 B^2
class ResizableArrayR3 {
private:
    int r = 3;           // 层级数
    int B;               // 基础块大小参数，满足 N^{1/3} <= B < 4*N^{1/3}
    int N;               // 当前元素总数
    
    // 索引块
    // A[0] 未使用（为方便索引）
    // A[1] 指向小块（大小B）的指针数组，大小为 2B
    // A[2] 指向大块（大小B^2）的指针数组，大小为 2B
    vector<void*> A;     // 指向索引块的指针，A[i] 是第i级索引块
    
    // 计数数组 n[0..r-1]
    // n[0]: 最后一个小块中的元素数量（如果n[1]>0）
    // n[1]: 小块（大小B）的数量，范围 [0, 2B]
    // n[2]: 大块（大小B^2）的数量，范围 [0, 2B]
    vector<int> n;
    
    // 辅助函数：分配新块
    int* allocateBlock(int size) {
        int* block = new int[size];
        return block;
    }
    
    // 辅助函数：释放块
    void deallocateBlock(void* block) {
        delete[] static_cast<int*>(block);
    }
    
    // 获取第i级第j个数据块
    int* getBlock(int level, int idx) {
        if (level == 1) {
            void** indexBlock = static_cast<void**>(A[1]);
            return static_cast<int*>(indexBlock[idx]);
        } else {
            void** indexBlock = static_cast<void**>(A[2]);
            return static_cast<int*>(indexBlock[idx]);
        }
    }
    
    // 设置第i级第j个数据块
    void setBlock(int level, int idx, int* block) {
        if (level == 1) {
            void** indexBlock = static_cast<void**>(A[1]);
            indexBlock[idx] = block;
        } else {
            void** indexBlock = static_cast<void**>(A[2]);
            indexBlock[idx] = block;
        }
    }
    
    // 复制元素
    void copy(int* src, int srcStart, int* dst, int dstStart, int len) {
        for (int i = 0; i < len; i++) {
            dst[dstStart + i] = src[srcStart + i];
        }
    }
    
    // 重建整个数据结构，使用新的 B'
    void rebuild(int newB) {
        // 保存旧数据
        vector<int> oldData;
        oldData.reserve(N);
        for (int i = 0; i < N; i++) {
            oldData.push_back(access(i));
        }
        
        // 释放旧结构
        cleanup();
        
        // 重新初始化
        B = newB;
        N = 0;
        n = vector<int>(r, 0);
        
        // 分配索引块
        A.resize(r);
        A[0] = nullptr; // 未使用
        A[1] = new void*[2 * B]; // 小块索引块
        A[2] = new void*[2 * B]; // 大块索引块
        
        // 重新插入所有元素
        for (int x : oldData) {
            grow(x);
        }
    }
    
    // 清理所有内存
    void cleanup() {
        // 释放所有小块
        for (int i = 0; i < n[1]; i++) {
            deallocateBlock(getBlock(1, i));
        }
        // 释放所有大块
        for (int i = 0; i < n[2]; i++) {
            deallocateBlock(getBlock(2, i));
        }
        // 释放索引块
        if (A.size() > 1 && A[1]) delete[] static_cast<void**>(A[1]);
        if (A.size() > 2 && A[2]) delete[] static_cast<void**>(A[2]);
    }
    
    // Combine-Blocks: 当小块满时（n[1]=2B 且 n[0]=B），合并B个块为一个大块
    void combineBlocks() {
        // 找到最小的 k 使得 n[k] < 2B
        int k = -1;
        for (int i = 1; i < r; i++) {
            if (n[i] < 2 * B) {
                k = i;
                break;
            }
        }
        assert(k != -1 && "No space found for combining");
        
        // 从 k-1 级向下合并到 1 级
        for (int i = k - 1; i >= 1; i--) {
            // 分配新的大块（大小 B^{i+1}）
            int newBlockSize = (i == 1) ? B * B : B * B * B; // B^2 或 B^3，但r=3时最大B^2
            if (i == 1) newBlockSize = B * B; // B^2
            
            int* newBlock = allocateBlock(newBlockSize);
            
            // 将前B个大小为 B^i 的块复制到新块
            int blockSize = (i == 1) ? B : B * B;
            for (int j = 0; j < B; j++) {
                int* oldBlock = getBlock(i, j);
                copy(oldBlock, 0, newBlock, j * blockSize, blockSize);
                deallocateBlock(oldBlock);
            }
            
            // 设置新块
            setBlock(i + 1, n[i + 1], newBlock);
            
            // 移动指针：后B个块移到前面
            for (int j = 0; j < B && (B + j) < n[i]; j++) {
                setBlock(i, j, getBlock(i, B + j));
            }
            
            n[i] = B;
            n[i + 1]++;
        }
    }
    
    // Split-Blocks: 当没有小块时（n[1]=0），分裂一个大块
    void splitBlocks() {
        int k = -1;
        for (int i = 1; i < r; i++) {
            if (n[i] > 0) {
                k = i;
                break;
            }
        }
        assert(k != -1);

        for (int i = k - 1; i >= 1; i--) {
            n[i + 1]--;
            int* bigBlock = getBlock(i + 1, n[i + 1]);

            int blockSize = (i == 1) ? B : B * B;

            for (int j = 0; j < B; j++) {
                int* newBlock = allocateBlock(blockSize);
                copy(bigBlock, j * blockSize, newBlock, 0, blockSize);
                // setBlock(i, j, newBlock); // 好像没啥区别
                setBlock(i, n[i] + j, newBlock); // ✅ append
            }

            n[i] += B;
            deallocateBlock(bigBlock);
        }

        // ⭐关键：恢复 invariant
        if (n[1] > 0) {
            n[0] = B;
        }
    }

public:
    ResizableArrayR3() {
        // 初始 B = 2（满足 N^{1/3} <= B，当N较小时）
        B = 2;
        N = 0;
        n = vector<int>(r, 0);
        
        A.resize(r);
        A[0] = nullptr;
        A[1] = new void*[2 * B]; // 小块索引
        A[2] = new void*[2 * B]; // 大块索引
    }
    
    ~ResizableArrayR3() {
        cleanup();
    }
    
    // 添加元素
    void grow(int a) {
        // 检查是否需要重建（当 N = B^3 时）
        long long Bcubed = 1LL * B * B * B;
        if (N == Bcubed) {
            rebuild(2 * B);
        }
        // 检查是否需要合并块（当 n[1]=2B 且 n[0]=B 时）
        else if (n[1] == 2 * B && n[0] == B) {
            combineBlocks();
        }
        
        // 检查是否需要分配新的小块
        if (n[1] == 0 || n[0] == B) {
            // 分配新的小块
            int* newBlock = allocateBlock(B);
            setBlock(1, n[1], newBlock);
            n[1]++;
            n[0] = 0;
        }
        
        // 插入元素
        int* lastBlock = getBlock(1, n[1] - 1);
        lastBlock[n[0]] = a;
        n[0]++;
        N++;
    }
    
    // 删除最后一个元素
    void shrink() {
        if (N == 0) return;
        
        // 检查是否需要重建（当 N = (B/4)^3 且 B >= 4 时）
        if (B >= 4) {
            int Bdiv4 = B / 4;
            long long threshold = 1LL * Bdiv4 * Bdiv4 * Bdiv4;
            if (N == threshold) {
                rebuild(B / 2);
            }
        }
        
        // 如果没有小块，需要分裂大块
        if (n[1] == 0) {
            splitBlocks();
        }
        
        // 删除元素
        n[0]--;
        N--;
        
        // 如果小块变空，释放它
        if (n[0] == 0) {
            deallocateBlock(getBlock(1, n[1] - 1));
            n[1]--;
            if (n[1] > 0) {
                n[0] = B; // 前一个块是满的
            }
        }
    }
    
    // 访问第 i 个元素（0索引），O(1) 时间
    int access(int idx) {
        assert(idx >= 0 && idx < N && "Index out of bounds");
        
        // 计算在大块中的位置
        int largeBlockSize = B * B;
        int numInLargeBlocks = n[2] * largeBlockSize;
        
        if (idx < numInLargeBlocks) {
            // 在大块中
            int blockIdx = idx / largeBlockSize;
            int offset = idx % largeBlockSize;
            int* block = getBlock(2, blockIdx);
            return block[offset];
        } else {
            // 在小块中
            int remaining = idx - numInLargeBlocks;
            int smallBlockSize = B;
            int blockIdx = remaining / smallBlockSize;
            int offset = remaining % smallBlockSize;
            int* block = getBlock(1, blockIdx);
            return block[offset];
        }
    }
    
    // 修改第 i 个元素
    void modify(int idx, int value) {
        assert(idx >= 0 && idx < N && "Index out of bounds");
        
        int largeBlockSize = B * B;
        int numInLargeBlocks = n[2] * largeBlockSize;
        
        if (idx < numInLargeBlocks) {
            int blockIdx = idx / largeBlockSize;
            int offset = idx % largeBlockSize;
            int* block = getBlock(2, blockIdx);
            block[offset] = value;
        } else {
            int remaining = idx - numInLargeBlocks;
            int smallBlockSize = B;
            int blockIdx = remaining / smallBlockSize;
            int offset = remaining % smallBlockSize;
            int* block = getBlock(1, blockIdx);
            block[offset] = value;
        }
    }
    
    // 获取大小
    int size() const {
        return N;
    }
    
    // 获取当前 B 值（用于调试）
    int getB() const {
        return B;
    }
    
    // 打印结构信息（用于调试）
    void printInfo() {
        cout << "N=" << N << ", B=" << B << ", n=[" << n[0] << "," << n[1] << "," << n[2] << "]" << endl;
        cout << "  Small blocks (size " << B << "): " << n[1] << endl;
        cout << "  Large blocks (size " << B*B << "): " << n[2] << endl;
        cout << "  Space overhead: ~" << (2*r-1) + 2*(r-1)*B + B << " + pointers" << endl;
    }
    
    // 打印所有元素
    void printAll() {
        cout << "Elements: [";
        for (int i = 0; i < N; i++) {
            if (i > 0) cout << ", ";
            cout << access(i);
        }
        cout << "]" << endl;
    }
};

