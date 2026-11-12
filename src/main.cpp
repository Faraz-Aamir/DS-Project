#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include "models/BinaryTree.h"

// Helper: Print a vector of ints
void printVector(const vector<int>& v, const string& label) {
    cout << label << ": [";
    for (size_t i = 0; i < v.size(); i++) {
        cout << v[i];
        if (i < v.size() - 1) cout << ", ";
    }
    cout << "]" << endl;
}

// Helper: Create a sample tree for Act 1 testing
// Tree structure:
//       50
//      /  \
//     30   70
//    / \   /
//   10 40 60
BinaryTree* createSampleTree() {
    BinaryTree* tree = new BinaryTree();

    // Build using preorder+inorder reconstruction
    // Preorder: 50, 30, 10, 40, 70, 60
    // Inorder: 10, 30, 40, 50, 60, 70
    vector<int> pre = {50, 30, 10, 40, 70, 60};
    vector<int> in = {10, 30, 40, 50, 60, 70};

    TreeNode* root = tree->buildFromPreIn(pre, in);
    tree->root = root;

    // Count nodes
    function<int(TreeNode*)> countNodes = [&](TreeNode* node) -> int {
        if (!node) return 0;
        return 1 + countNodes(node->left) + countNodes(node->right);
    };
    tree->nodeCount = countNodes(root);

    return tree;
}

int main() {
    cout << "=== VAULT HEIST: Phase 1 - Binary Tree Fundamentals ===" << endl << endl;

    // Create and test the tree
    BinaryTree* tree = createSampleTree();

    cout << "Sample vault tree created with " << tree->getNodeCount() << " rooms" << endl << endl;

    // 1. TRAVERSALS
    cout << "=== 1. TRAVERSALS ===" << endl;
    printVector(tree->preorder(), "Preorder (Root → Left → Right)");
    printVector(tree->inorder(), "Inorder (Left → Root → Right)");
    printVector(tree->postorder(), "Postorder (Left → Right → Root)");
    printVector(tree->levelOrder(), "Level-Order (Breadth-First)");

    cout << endl << "=== 2. HEIGHT & DIAMETER ===" << endl;
    cout << "Tree height (edges): " << tree->height() << endl;
    cout << "Tree diameter: " << tree->diameter() << endl;

    cout << endl << "=== 3. LCA (Lowest Common Ancestor) ===" << endl;
    TreeNode* lcaNode = tree->lca(10, 60);
    cout << "LCA of rooms 10 and 60: Room " << (lcaNode ? lcaNode->id : -1) << endl;
    lcaNode = tree->lca(10, 40);
    cout << "LCA of rooms 10 and 40: Room " << (lcaNode ? lcaNode->id : -1) << endl;

    cout << endl << "=== 4. PATH TO NODE ===" << endl;
    vector<int> path = tree->pathTo(60);
    cout << "Path to room 60: ";
    for (size_t i = 0; i < path.size(); i++) {
        cout << path[i];
        if (i < path.size() - 1) cout << " → ";
    }
    cout << endl;

    cout << endl << "=== 5. MIRROR (Tree Inversion) ===" << endl;
    cout << "Original inorder: ";
    printVector(tree->inorder(), "");

    tree->mirror();

    cout << "After mirror inorder: ";
    printVector(tree->inorder(), "");

    cout << endl << "=== 6. IS SYMMETRIC ===" << endl;

    // Build a symmetric tree manually:
    //     1
    //    / \
    //   2   2
    //  / \ / \
    // 3  4 4  3
    TreeNode* s_left3 = new TreeNode(3, 3);
    TreeNode* s_right3 = new TreeNode(4, 4);
    TreeNode* s_left4 = new TreeNode(4, 4);
    TreeNode* s_right4 = new TreeNode(3, 3);
    TreeNode* s_root2left = new TreeNode(2, 2);
    s_root2left->left = s_left3;
    s_root2left->right = s_right3;
    TreeNode* s_root2right = new TreeNode(2, 2);
    s_root2right->left = s_left4;
    s_root2right->right = s_right4;
    TreeNode* symRoot = new TreeNode(1, 1);
    symRoot->left = s_root2left;
    symRoot->right = s_root2right;
    BinaryTree* symmetricTree = new BinaryTree();
    symmetricTree->root = symRoot;
    symmetricTree->nodeCount = 7;

    cout << "Symmetric tree - isSymmetric: " << (symmetricTree->isSymmetric() ? "YES" : "NO") << endl;

    // Asymmetric tree: root 1, left 2 with left 3, right 4
    BinaryTree* asymTree = new BinaryTree();
    TreeNode* a2 = new TreeNode(2, 2);
    TreeNode* a3 = new TreeNode(3, 3);
    a2->left = a3;
    TreeNode* a4 = new TreeNode(4, 4);
    TreeNode* a1 = new TreeNode(1, 1);
    a1->left = a2;
    a1->right = a4;
    asymTree->root = a1;
    asymTree->nodeCount = 3;

    cout << "Asymmetric tree - isSymmetric: " << (asymTree->isSymmetric() ? "YES" : "NO") << endl;

    // Cleanup
    delete tree;
    delete symmetricTree;
    delete asymTree;

    cout << endl << "=== PHASE 1 COMPLETE: Binary Tree Fundamentals ===" << endl;
    cout << "Concepts mastered:" << endl;
    cout << "  ✓ Preorder, inorder, postorder traversals" << endl;
    cout << "  ✓ Level-order (breadth-first) traversal" << endl;
    cout << "  ✓ Height computation (edges convention)" << endl;
    cout << "  ✓ Diameter (longest path between any two nodes)" << endl;
    cout << "  ✓ LCA (lowest common ancestor)" << endl;
    cout << "  ✓ Path from root to any node" << endl;
    cout << "  ✓ Tree mirror/inversion" << endl;
    cout << "  ✓ Structural comparison (isSameTree, isSymmetric)" << endl;

    return 0;
}