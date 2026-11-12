# Vault Heist Unit Test Report

**Date:** 11 November 2026  
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
| Binary tree queries and mirror | LCA, root-to-node path, missing target, symmetry, and mirror behavior. |
| BST operations and deletion | Duplicate rejection; search; min/max; predecessor/successor; range query; kth-smallest bounds; leaf, left-child, right-child, two-child root, one-child root, missing-key deletion, and parent-link updates. |
| AVL operations | LL, RR, LR, RL insertion rotations; deletion with a zero-balance heavy child; repeated deletions; duplicate and missing-key behavior; balance, size count, `kth`, `rank`, and range counts. |
| Max Heap and Heap Sort | Empty, singleton, ordered, reverse-ordered, duplicate, negative, and deterministic random inputs; build property; insertion; extraction order; underflow; both sort entry points. |

## Notes

- The build reports one existing warning: `BST::insertHelper` has an unused `parent` parameter. It does not prevent the test executable from building or passing.
- GUI level completion, rendering, undo behavior, and campaign save/load require separate manual verification and are not covered by this report.
- The traversal methods added for parity are `preorderIterative`, `inorderIterative`, `postorderIterative`, and `levelOrderRecursive`.
