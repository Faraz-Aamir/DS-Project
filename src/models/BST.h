#ifndef BST_H
#define BST_H

#include "BinaryTree.h"
#include <vector>
#include <iostream>

using namespace std;

struct BSTNode : public TreeNode {
    int key;
    BSTNode(int k, int val = 0) : TreeNode(k, val == 0 ? k : val), key(k) {}
};

class BST : public BinaryTree {
public:
    BST() {}
    virtual ~BST() {}

    virtual bool insert(int key, int value = 0) {
        if (!root) {
            root = new BSTNode(key, value);
            nodeCount++;
            return true;
        }
        return insertHelper(root, key, value, nullptr);
    }

    BSTNode* search(int key) {
        TreeNode* curr = root;
        while (curr) {
            BSTNode* bstCurr = (BSTNode*)curr;
            if (key == bstCurr->key) return bstCurr;
            if (key < bstCurr->key) curr = curr->left;
            else curr = curr->right;
        }
        return nullptr;
    }

    BSTNode* lcaBST(int keyA, int keyB) {
        if (!search(keyA) || !search(keyB)) return nullptr;
        TreeNode* node = root;
        while (node) {
            int key = ((BSTNode*)node)->key;
            if (keyA < key && keyB < key) node = node->left;
            else if (keyA > key && keyB > key) node = node->right;
            else return (BSTNode*)node;
        }
        return nullptr;
    }

    virtual bool remove(int key) {
        bool deleted = false;
        root = deleteHelper(root, key, deleted);
        if (deleted) nodeCount--;
        return deleted;
    }

    BSTNode* findMin(TreeNode* node = nullptr) {
        TreeNode* curr = node ? node : root;
        if (!curr) return nullptr;
        while (curr->left) curr = curr->left;
        return (BSTNode*)curr;
    }

    BSTNode* findMax(TreeNode* node = nullptr) {
        TreeNode* curr = node ? node : root;
        if (!curr) return nullptr;
        while (curr->right) curr = curr->right;
        return (BSTNode*)curr;
    }

    BSTNode* successor(int key) {
        BSTNode* target = search(key);
        if (!target) return nullptr;
        if (target->right) {
            return findMin(target->right);
        }
        TreeNode* curr = target;
        TreeNode* succ = nullptr;
        while (curr && curr->parent) {
            if (curr == curr->parent->left) {
                succ = curr->parent;
                break;
            }
            curr = curr->parent;
        }
        return (BSTNode*)succ;
    }

    BSTNode* predecessor(int key) {
        BSTNode* target = search(key);
        if (!target) return nullptr;
        if (target->left) {
            return findMax(target->left);
        }
        TreeNode* curr = target;
        TreeNode* pred = nullptr;
        while (curr && curr->parent) {
            if (curr == curr->parent->right) {
                pred = curr->parent;
                break;
            }
            curr = curr->parent;
        }
        return (BSTNode*)pred;
    }

    vector<int> rangeQuery(int lo, int hi) {
        vector<int> result;
        rangeQueryHelper(root, lo, hi, result);
        return result;
    }

    int kthSmallest(int k) {
        vector<int> in = inorder();
        if (k < 1 || k > (int)in.size()) return -1;
        return in[k - 1];
    }

protected:
    bool insertHelper(TreeNode* node, int key, int value, TreeNode* parent) {
        BSTNode* bstNode = (BSTNode*)node;
        if (key == bstNode->key) {
            cout << "[Duplicate Error] Key " << key << " already exists in BST. Rejected." << endl;
            return false;
        }
        if (key < bstNode->key) {
            if (!node->left) {
                BSTNode* newNode = new BSTNode(key, value);
                newNode->parent = node;
                node->left = newNode;
                nodeCount++;
                return true;
            }
            return insertHelper(node->left, key, value, node);
        } else {
            if (!node->right) {
                BSTNode* newNode = new BSTNode(key, value);
                newNode->parent = node;
                node->right = newNode;
                nodeCount++;
                return true;
            }
            return insertHelper(node->right, key, value, node);
        }
    }

    TreeNode* deleteHelper(TreeNode* node, int key, bool& deleted) {
        if (!node) return nullptr;
        BSTNode* bstNode = (BSTNode*)node;
        if (key < bstNode->key) {
            node->left = deleteHelper(node->left, key, deleted);
            if (node->left) node->left->parent = node;
        } else if (key > bstNode->key) {
            node->right = deleteHelper(node->right, key, deleted);
            if (node->right) node->right->parent = node;
        } else {
            deleted = true;
            // Case 1 & 2: Leaf or one child
            if (!node->left) {
                TreeNode* temp = node->right;
                if (temp) temp->parent = node->parent;
                delete node;
                return temp;
            } else if (!node->right) {
                TreeNode* temp = node->left;
                if (temp) temp->parent = node->parent;
                delete node;
                return temp;
            }
            // Case 3: Two children - find inorder successor (min of right subtree)
            BSTNode* succ = findMin(node->right);
            bstNode->key = succ->key;
            bstNode->value = succ->value;
            node->right = deleteHelper(node->right, succ->key, deleted);
            if (node->right) node->right->parent = node;
        }
        return node;
    }

    void rangeQueryHelper(TreeNode* node, int lo, int hi, vector<int>& result) {
        if (!node) return;
        BSTNode* bstNode = (BSTNode*)node;
        if (bstNode->key > lo) {
            rangeQueryHelper(node->left, lo, hi, result);
        }
        if (bstNode->key >= lo && bstNode->key <= hi) {
            result.push_back(bstNode->key);
        }
        if (bstNode->key < hi) {
            rangeQueryHelper(node->right, lo, hi, result);
        }
    }
};

#endif
