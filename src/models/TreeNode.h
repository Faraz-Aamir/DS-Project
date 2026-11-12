#ifndef TREE_NODE_H
#define TREE_NODE_H

using namespace std;

// TreeNode is the fundamental building block for all tree structures.
// It represents a single room in the vault.
//
// WHY WE NEED UNIQUE IDs:
// The GUI needs to track nodes across operations like rotations and value swaps.
// Without IDs, when two nodes swap values, the GUI wouldn't know which visual
// node to animate. IDs give each node a stable identity that survives structural changes.
//
// WHY PARENT POINTERS ARE RECOMMENDED:
// 1. Successor/predecessor lookups become O(1) instead of O(n)
// 2. AVL rotations can update subtree links without searching from the root
// 3. The layout engine can compute node positions without a full tree traversal
// 4. The validator can check parent-child consistency
//
// DESIGN CHOICE: We use integer IDs (not memory addresses) because:
// - Memory addresses change between runs (not reproducible for save/load)
// - IDs are stable across serialization/deserialization
// - IDs make debugging and event logging readable
// - IDs allow the GUI to track nodes through rotations (which change pointers)
struct TreeNode {
    int id;         // Unique identifier for this node (room)
    int value;      // The treasure value stored in this room
    TreeNode* left;
    TreeNode* right;
    TreeNode* parent;

    TreeNode(int nodeId, int nodeValue)
        : id(nodeId), value(nodeValue), left(nullptr), right(nullptr), parent(nullptr) {}
};

#endif