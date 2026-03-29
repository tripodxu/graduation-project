#include "demo.cpp"  

// 测试代码
// int main() {
//     cout << "=== Resizable Array r=3 Implementation Test ===" << endl;
    
//     ResizableArrayR3 arr;
    
//     cout << "\n--- Test 1: Basic grow operations ---" << endl;
//     for (int i = 0; i < 65; i++) {
//         arr.grow(i * 10);
//         arr.printInfo();
//         arr.printAll();
//         cout <<"--------------------------"<<endl;
//     }
//     arr.printInfo();
//     arr.printAll();
    
    // cout << "\n--- Test 2: Access operations ---" << endl;
    // cout << "arr[5] = " << arr.access(5) << " (expected 50)" << endl;
    // cout << "arr[15] = " << arr.access(15) << " (expected 150)" << endl;
    
    // cout << "\n--- Test 3: Modify operations ---" << endl;
    // arr.modify(20, 985);
    // cout << "After modify arr[20] = 985: arr[20] = " << arr.access(20) << endl;
    // arr.printInfo();
    // arr.printAll();
    
    // cout << "\n--- Test 4: Shrink operations ---" << endl;
    // for (int i = 0; i < 5; i++) {
    //     arr.shrink();
    // }
    // cout << "After shrinking 5 elements:" << endl;
    // arr.printInfo();
    // arr.printAll();
    
    // cout << "\n--- Test 5: Large scale test (trigger rebuilds) ---" << endl;
    // ResizableArrayR3 arr2;
    // // 添加足够多的元素以触发重建
    // for (int i = 0; i < 1000; i++) {
    //     arr2.grow(i);
    // }
    // arr2.printInfo();
    
    // // 验证所有元素
    // bool correct = true;
    // for (int i = 0; i < 100; i++) {
    //     if (arr2.access(i) != i) {
    //         correct = false;
    //         cout << "Error at index " << i << ": got " << arr2.access(i) << ", expected " << i << endl;
    //         break;
    //     }
    // }
    // cout << "All elements correct: " << (correct ? "YES" : "NO") << endl;
    
    // cout << "\n--- Test 6: Shrink to trigger split ---" << endl;
    // // 先添加很多元素
    // ResizableArrayR3 arr3;
    // for (int i = 0; i < 500; i++) {
    //     arr3.grow(i);
    // }
    // cout << "Before massive shrink:" << endl;
    // arr3.printInfo();
    
    // // 删除大部分元素
    // for (int i = 0; i < 475; i++) {
    //     arr3.shrink();
    // }
    // cout << "After shrinking to 25 elements:" << endl;
    // arr3.printInfo();
    // arr3.printAll();
    
    // cout << "\n--- Test 7: Mixed operations ---" << endl;
    // ResizableArrayR3 arr4;
    // for (int i = 0; i < 30; i++) arr4.grow(i);
    // for (int i = 0; i < 10; i++) arr4.shrink();
    // for (int i = 100; i < 110; i++) arr4.grow(i);
    
    // cout << "Final array (should be 0-19, then 100-109):" << endl;
    // arr4.printAll();
    
    // // 验证
    // correct = true;
    // for (int i = 0; i < 20; i++) {
    //     if (arr4.access(i) != i) correct = false;
    // }
    // for (int i = 20; i < 30; i++) {
    //     if (arr4.access(i) != 80 + i) correct = false; // 100 + (i-20)
    // }
    // cout << "Mixed operations correct: " << (correct ? "YES" : "NO") << endl;
    
    // cout << "\n=== All Tests Completed ===" << endl;
    
    // return 0;
// }

int main() {
    ResizableArrayR3 arr;

    cout << "=== FORCE SplitBlocks Test ===" << endl;

    // Step 1: 填充
    for (int i = 0; i < 64; i++) {
        arr.grow(i);
        arr.printInfo();
    }

    cout << "After grow 65:" << endl;
    arr.printInfo();

    // Step 2: 一直 shrink，直到 n[1] == 0
    while (true) {
        arr.shrink();
        arr.printInfo();

        // ⭐关键：观察 small blocks
        // 你可以在 printInfo 里看到 n[1]

        // 手动 break 条件（建议你加 getter 更方便）
        if (arr.size() <= 16) break;
    }

    cout << "After forcing small blocks empty:" << endl;
    arr.printInfo();

    // Step 3: 再插入 → 必触发 SplitBlocks
    for (int i = 1000; i < 1010; i++) {
        arr.grow(i);
        arr.printInfo();
    }

    cout << "After grow again:" << endl;
    arr.printAll();

    // Step 4: 检查错误
    for (int i = 0; i < arr.size(); i++) {
        cout << arr.access(i) << " ";
    }
    cout << endl;

    return 0;
}