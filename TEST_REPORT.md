# Vault Heist Unit Test Report

**Date:** 19 November 2026  
**Scope:** Model-level tests only; no GUI interaction is covered here.

## Build and run

Built from the project root with:

```powershell
C:\raylib\w64devkit\bin\g++.exe -std=c++17 -O2 -Wall -Wextra -Isrc -o unit_tests.exe tests\unit_test.cpp src\models\BinaryTree.cpp
```

Ran with:

```powershell
.\unit_tests.exe
```

## Result

**Pass:** all five test groups completed and the program printed `ALL UNIT TESTS PASSED` (exit code 0).

| Test group | Coverage |
|---|---|
| Traversals and reconstruction | Empty, single-node, full and skewed trees; recursive/iterative parity for preorder, inorder, postorder, and level order; preorder/inorder and postorder/inorder reconstruction; height and diameter. |
| Binary tree queries and mirror | General LCA, BST-order LCA, root-to-node paths and events, missing targets, symmetry, and mirror behavior. |
| BST operations and deletion | Duplicate rejection; search; min/max; predecessor/successor; range query; kth-smallest bounds; `lcaBST`; all deletion shapes; and validator checks for ordering, node counts, parent links, cycles, and shared nodes. |
| AVL operations | LL, RR, LR, RL insertion rotations; deletion rebalance; duplicate/missing keys; balance, height/size metadata, `kth`, `rank`, and range counts. Validator checks run throughout the cases, including a deliberate bad-size mutation. |
| Max Heap and Heap Sort | Empty, singleton, ordered, reverse, duplicate, negative, and deterministic random inputs; build property; insertion; extraction; underflow; both sort entry points; and valid/invalid heap checks. |

## Notes

- The build reports one existing warning: `BST::insertHelper` has an unused `parent` parameter. It does not prevent the test executable from building or passing. The GUI executable also built and linked successfully with Raylib available; launching and walking all four acts still needs a manual check.
- GUI level completion, rendering, undo behavior, alarm/star scoring, and save/load still require manual verification and are not covered by this model test report.
- The traversal methods added for parity are `preorderIterative`, `inorderIterative`, `postorderIterative`, and `levelOrderRecursive`.
