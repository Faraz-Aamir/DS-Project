# Vault Heist Viva Preparation

Use this sheet to rehearse the implementation in your own words. The code and live demonstration should support each answer.

## Short project introduction

Vault Heist is a C++ data-structures game with a 20-level campaign. Its four acts demonstrate general binary trees, binary search trees, AVL trees, and Max Heaps. The models are implemented from scratch, tested separately from the GUI, and measured by a fixed-seed benchmark program.

## Core questions

### What is the difference between a binary tree and a BST?

A general binary tree has at most two children per node and no ordering rule. A BST additionally keeps smaller keys in the left subtree and larger keys in the right. This ordering lets search follow one branch at each node, but search can still be O(n) when the BST becomes a chain.

### Why use an AVL tree?

An AVL tree maintains a height difference of at most one between each node's child subtrees. It restores that invariant with rotations after inserts and deletes, keeping height O(log n) and search/update operations O(log n).

### Explain LL, RR, LR, and RL rotations.

LL is repaired by one right rotation; RR by one left rotation. LR uses a left rotation at the left child, then a right rotation at the root. RL uses a right rotation at the right child, then a left rotation at the root.

### How does `kth` work in the AVL tree?

Each AVL node stores the size of its subtree. Compare k with the left subtree size plus one. If equal, the current node is the answer; if smaller, go left; otherwise subtract the left size and current node, then go right. This is O(log n) because the AVL tree is balanced.

### What does `rank` return?

It counts how many keys are less than or equal to the requested key. At each node, subtree sizes let the search add all keys known to be smaller without visiting them individually.

### What is a Max Heap?

It is a complete binary tree represented in an array where each parent is greater than or equal to its children. The root contains the maximum. With zero-based indexing, a node at i has children 2i+1 and 2i+2.

### Why is bottom-up heap construction O(n), not O(n log n)?

Most nodes are near the leaves and can move down only a small number of positions. Summed over all internal nodes, the sift-down work is linear even though a single sift-down can take O(log n).

### How does Heap Sort work and what are its costs?

Build a Max Heap, swap its root with the last element in the active heap, shrink the active region, and sift down the new root. Repeat until sorted. It takes O(n log n) time and O(1) auxiliary space when sorting the existing array in place.

### How did you verify correctness?

The unit suite covers traversal parity and reconstruction, BST deletion cases, all four AVL insertion rotations, deletion rebalancing, order statistics, and heap operations on edge and representative inputs. The benchmark also checks each sort result against a sorted reference.

### What did the benchmarks show?

For 10,000 sorted inserts, the BST height was 9,999 while the AVL height was 13. Successful-search comparisons averaged about 9,979 for the sorted BST versus 23.7 for AVL. The sorting comparison measures Heap Sort, Merge Sort, and three-way Quick Sort on four input patterns and three sizes.

## Demonstration route

1. In Act 1, show a traversal or tree reconstruction challenge.
2. In Act 2, show deletion of a node with two children and the successor replacement.
3. In Act 3, show LL/RR and LR/RL using the case selectors, explaining each pivot.
4. In Act 4, build a heap, heapify a disturbed input, extract a maximum, then show a sorted result.

## Implementation boundaries to explain honestly

The current executable contains the campaign level browser and its tree/heap challenges. The original brief's endless mode, sandbox, campaign map, general event-playback animation, and dedicated result screens are not implemented. The current GUI uses snapshot-based undo where supported. Broad 200-node pan/zoom performance and every fail-and-retry path have not been measured systematically.

## Team preparation

Both members should practise explaining at least one data structure, one complexity result, the test approach, and a live level. Be ready to point to the relevant model class and answer follow-up questions without relying only on the report.
