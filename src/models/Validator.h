#ifndef VAULT_HEIST_VALIDATOR_H
#define VAULT_HEIST_VALIDATOR_H

#include "AVLTree.h"
#include "MaxHeap.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_set>

struct ValidationResult {
    bool valid;
    std::string message;
};

class Validator {
public:
    static ValidationResult validateTreeLinks(const TreeNode* root, int expectedCount) {
        std::unordered_set<const TreeNode*> seen;
        int count = 0;
        if (!validateLinks(root, nullptr, seen, count))
            return {false, "Tree has a cycle, shared node, or invalid parent link."};
        if (count != expectedCount) return {false, "Tree node count does not match the structure."};
        return {true, "Tree links valid."};
    }

    static ValidationResult validateBST(const BST& tree) {
        std::unordered_set<const TreeNode*> seen;
        int count = 0;
        if (!validateLinks(tree.root, nullptr, seen, count))
            return {false, "Tree has a cycle, shared node, or invalid parent link."};
        if (count != tree.nodeCount) return {false, "BST node count does not match the structure."};
        if (!validateOrder(tree.root, nullptr, nullptr))
            return {false, "BST ordering invariant is violated."};
        return {true, "BST invariants valid."};
    }

    static ValidationResult validateAVL(const AVLTree& tree) {
        std::unordered_set<const TreeNode*> seen;
        int count = 0;
        if (!validateLinks(tree.root, nullptr, seen, count))
            return {false, "Tree has a cycle, shared node, or invalid parent link."};
        if (count != tree.nodeCount) return {false, "AVL node count does not match the structure."};
        if (!validateOrder(tree.root, nullptr, nullptr))
            return {false, "AVL ordering invariant is violated."};
        if (!validateAVLMetadata(tree.root).first)
            return {false, "AVL height, size, or balance invariant is violated."};
        return {true, "AVL invariants valid."};
    }

    static ValidationResult validateHeap(const MaxHeap& heap, int activeSize = -1) {
        size_t count = activeSize < 0 ? heap.heap.size()
                                      : std::min(heap.heap.size(), (size_t)std::max(0, activeSize));
        for (size_t child = 1; child < count; ++child) {
            size_t parent = (child - 1) / 2;
            if (heap.heap[parent] < heap.heap[child])
                return {false, "Max Heap ordering invariant is violated."};
        }
        return {true, "Max Heap invariant valid."};
    }

private:
    static bool validateLinks(const TreeNode* node, const TreeNode* parent,
                              std::unordered_set<const TreeNode*>& seen, int& count) {
        if (!node) return true;
        if (!seen.insert(node).second || node->parent != parent) return false;
        ++count;
        return validateLinks(node->left, node, seen, count) &&
               validateLinks(node->right, node, seen, count);
    }

    static bool validateOrder(const TreeNode* node, const int* lower, const int* upper) {
        if (!node) return true;
        int key = ((const BSTNode*)node)->key;
        if ((lower && key <= *lower) || (upper && key >= *upper)) return false;
        return validateOrder(node->left, lower, &key) &&
               validateOrder(node->right, &key, upper);
    }

    static std::pair<bool, std::pair<int, int>> validateAVLMetadata(const TreeNode* node) {
        if (!node) return {true, {0, 0}};
        auto left = validateAVLMetadata(node->left);
        auto right = validateAVLMetadata(node->right);
        const AVLNode* avl = (const AVLNode*)node;
        int expectedHeight = 1 + std::max(left.second.first, right.second.first);
        int expectedSize = 1 + left.second.second + right.second.second;
        bool valid = left.first && right.first && avl->height == expectedHeight &&
                     avl->size == expectedSize &&
                     std::abs(left.second.first - right.second.first) <= 1;
        return {valid, {expectedHeight, expectedSize}};
    }
};

#endif
