#ifndef AVL_TREE_H
#define AVL_TREE_H

#include "BST.h"
#include <algorithm>

using namespace std;

struct AVLNode : public BSTNode {
    int height;
    int size;

    AVLNode(int k, int val = 0) : BSTNode(k, val), height(1), size(1) {}
};

class AVLTree : public BST {
public:
    AVLTree() : BST() {}
    ~AVLTree() {}

    bool insert(int key, int value = 0) override {
        if (search(key)) {
            cout << "[Duplicate Error] Key " << key << " already exists in AVL. Rejected." << endl;
            return false;
        }
        root = insertAVLHelper(root, key, value);
        nodeCount++;
        return true;
    }

    bool remove(int key) override {
        bool deleted = false;
        root = deleteAVLHelper(root, key, deleted);
        if (deleted) nodeCount--;
        return deleted;
    }

    int kth(int k) {
        if (!root || k < 1 || k > nodeCount) return -1;
        return kthHelper(root, k);
    }

    int rank(int key) {
        return rankHelper(root, key);
    }

    int countInRange(int lo, int hi) {
        if (lo > hi) return 0;
        return rank(hi) - rank(lo - 1);
    }

    bool isBalanced() {
        return isBalancedHelper(root);
    }

    long long getRotationCount() const { return rotationCount; }
    void resetRotationCount() { rotationCount = 0; }

private:
    long long rotationCount = 0;

    int heightAVL(TreeNode* node) {
        return node ? ((AVLNode*)node)->height : 0;
    }

    int sizeAVL(TreeNode* node) {
        return node ? ((AVLNode*)node)->size : 0;
    }

    void updateNode(TreeNode* node) {
        if (!node) return;
        AVLNode* avl = (AVLNode*)node;
        avl->height = 1 + max(heightAVL(node->left), heightAVL(node->right));
        avl->size = 1 + sizeAVL(node->left) + sizeAVL(node->right);
    }

    int balanceFactor(TreeNode* node) {
        if (!node) return 0;
        return heightAVL(node->left) - heightAVL(node->right);
    }

    TreeNode* rotateRight(TreeNode* y) {
        ++rotationCount;
        TreeNode* x = y->left;
        TreeNode* T2 = x->right;

        x->right = y;
        x->parent = y->parent;
        y->left = T2;
        if (T2) T2->parent = y;
        y->parent = x;

        updateNode(y);
        updateNode(x);
        return x;
    }

    TreeNode* rotateLeft(TreeNode* x) {
        ++rotationCount;
        TreeNode* y = x->right;
        TreeNode* T2 = y->left;

        y->left = x;
        y->parent = x->parent;
        x->right = T2;
        if (T2) T2->parent = x;
        x->parent = y;

        updateNode(x);
        updateNode(y);
        return y;
    }

    TreeNode* rebalance(TreeNode* node) {
        updateNode(node);
        int bf = balanceFactor(node);

        if (bf > 1 && balanceFactor(node->left) >= 0) return rotateRight(node);
        if (bf > 1 && balanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
            if (node->left) node->left->parent = node;
            return rotateRight(node);
        }
        if (bf < -1 && balanceFactor(node->right) <= 0) return rotateLeft(node);
        if (bf < -1 && balanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
            if (node->right) node->right->parent = node;
            return rotateLeft(node);
        }
        return node;
    }

    TreeNode* insertAVLHelper(TreeNode* node, int key, int value) {
        if (!node) {
            AVLNode* newNode = new AVLNode(key, value);
            return newNode;
        }
        AVLNode* avl = (AVLNode*)node;
        if (key < avl->key) {
            node->left = insertAVLHelper(node->left, key, value);
            if (node->left) node->left->parent = node;
        } else if (key > avl->key) {
            node->right = insertAVLHelper(node->right, key, value);
            if (node->right) node->right->parent = node;
        }
        updateNode(node);
        return rebalance(node);
    }

    TreeNode* deleteAVLHelper(TreeNode* node, int key, bool& deleted) {
        if (!node) {
            deleted = false;
            return nullptr;
        }
        AVLNode* avl = (AVLNode*)node;
        if (key < avl->key) {
            node->left = deleteAVLHelper(node->left, key, deleted);
            if (node->left) node->left->parent = node;
        } else if (key > avl->key) {
            node->right = deleteAVLHelper(node->right, key, deleted);
            if (node->right) node->right->parent = node;
        } else {
            deleted = true;
            if (!node->left || !node->right) {
                TreeNode* temp = node->left ? node->left : node->right;
                if (temp) temp->parent = node->parent;
                delete node;
                return temp;
            } else {
                BSTNode* succ = findMin(node->right);
                avl->key = succ->key;
                avl->value = succ->value;
                node->right = deleteAVLHelper(node->right, succ->key, deleted);
                if (node->right) node->right->parent = node;
            }
        }

        if (!node) return nullptr;
        updateNode(node);
        return rebalance(node);
    }

    int kthHelper(TreeNode* node, int k) {
        if (!node) return -1;
        int leftSize = sizeAVL(node->left);
        if (k == leftSize + 1) {
            return ((BSTNode*)node)->key;
        } else if (k <= leftSize) {
            return kthHelper(node->left, k);
        } else {
            return kthHelper(node->right, k - leftSize - 1);
        }
    }

    int rankHelper(TreeNode* node, int key) {
        if (!node) return 0;
        BSTNode* bstNode = (BSTNode*)node;
        if (key < bstNode->key) {
            return rankHelper(node->left, key);
        } else if (key == bstNode->key) {
            return sizeAVL(node->left) + 1;
        } else {
            return sizeAVL(node->left) + 1 + rankHelper(node->right, key);
        }
    }

    bool isBalancedHelper(TreeNode* node) {
        if (!node) return true;
        int bf = balanceFactor(node);
        if (abs(bf) > 1) return false;
        return isBalancedHelper(node->left) && isBalancedHelper(node->right);
    }

    BSTNode* findMin(TreeNode* node) {
        if (!node) return nullptr;
        while (node->left) node = node->left;
        return (BSTNode*)node;
    }
};

#endif
