#include <iostream>
#include <vector>
#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <random>
#include "models/BinaryTree.h"
#include "models/BST.h"
#include "models/AVLTree.h"
#include "models/MaxHeap.h"
#include "models/Validator.h"

using namespace std;

static void checkTraversalParity(BinaryTree& tree) {
    assert(tree.preorder() == tree.preorderIterative());
    assert(tree.inorder() == tree.inorderIterative());
    assert(tree.postorder() == tree.postorderIterative());
    assert(tree.levelOrder() == tree.levelOrderRecursive());
}

static void testTraversalsAndReconstruction() {
    BinaryTree empty;
    checkTraversalParity(empty);
    assert(empty.preorder().empty() && empty.height() == -1 && empty.diameter() == 0);

    BinaryTree single;
    single.root = new TreeNode(1, 42);
    checkTraversalParity(single);
    assert(single.preorder() == vector<int>{42});

    vector<int> pre{50, 30, 10, 40, 70, 60, 80};
    vector<int> in{10, 30, 40, 50, 60, 70, 80};
    vector<int> post{10, 40, 30, 60, 80, 70, 50};
    BinaryTree fromPre;
    fromPre.root = fromPre.buildFromPreIn(pre, in);
    assert(fromPre.preorder() == pre && fromPre.inorder() == in && fromPre.postorder() == post);
    assert(fromPre.levelOrder() == vector<int>({50, 30, 70, 10, 40, 60, 80}));
    assert(fromPre.height() == 2 && fromPre.diameter() == 4);
    checkTraversalParity(fromPre);

    BinaryTree fromPost;
    fromPost.root = fromPost.buildFromPostIn(post, in);
    assert(fromPre.isSameTree(&fromPost));
    checkTraversalParity(fromPost);

    BinaryTree skewed;
    skewed.root = new TreeNode(1, 1);
    skewed.root->right = new TreeNode(2, 2);
    skewed.root->right->parent = skewed.root;
    skewed.root->right->right = new TreeNode(3, 3);
    skewed.root->right->right->parent = skewed.root->right;
    assert(skewed.height() == 2);
    checkTraversalParity(skewed);
}

static void testBinaryTreeQueriesAndMirror() {
    BinaryTree tree;
    tree.root = tree.buildFromPreIn({4, 2, 1, 3, 6, 5, 7}, {1, 2, 3, 4, 5, 6, 7});
    int nextId = 1;
    vector<TreeNode*> pending{tree.root};
    while (!pending.empty()) {
        TreeNode* node = pending.back(); pending.pop_back();
        node->id = nextId++;
        if (node->right) pending.push_back(node->right);
        if (node->left) pending.push_back(node->left);
    }
    TreeNode* one = tree.root->left->left;
    TreeNode* three = tree.root->left->right;
    assert(tree.lca(one->id, three->id) == tree.root->left);
    assert(tree.pathTo(one->id) == vector<int>({tree.root->id, tree.root->left->id, one->id}));
    assert(tree.pathTo(-1).empty());
    assert(tree.pathToWithEvents(one->id).size() == 3);
    BinaryTree::setEventRecordingEnabled(false);
    assert(tree.pathToWithEvents(one->id).empty());
    BinaryTree::setEventRecordingEnabled(true);

    BinaryTree mirrorTree;
    mirrorTree.root = new TreeNode(1, 1);
    mirrorTree.root->left = new TreeNode(2, 2);
    mirrorTree.root->right = new TreeNode(3, 8);
    mirrorTree.root->left->left = new TreeNode(4, 3);
    mirrorTree.root->right->right = new TreeNode(5, 9);
    assert(!mirrorTree.isSymmetric());
    BinaryTree original;
    original.root = new TreeNode(1, 1);
    original.root->left = new TreeNode(2, 2);
    original.root->right = new TreeNode(3, 8);
    original.root->left->left = new TreeNode(4, 3);
    original.root->right->right = new TreeNode(5, 9);
    original.mirror();
    mirrorTree.mirror();
    assert(mirrorTree.isSameTree(&original));
    mirrorTree.mirror();
    assert(!mirrorTree.isSymmetric());
}

static void testBSTOperationsAndDeletionCases() {
    BST bst;
    for (int key : {50, 30, 70, 20, 40, 60, 80}) assert(bst.insert(key));
    assert(!bst.insert(30) && bst.nodeCount == 7);
    assert(bst.search(40) && bst.search(40)->key == 40 && bst.search(99) == nullptr);
    assert(bst.findMin()->key == 20 && bst.findMax()->key == 80);
    assert(bst.successor(50)->key == 60 && bst.predecessor(50)->key == 40);
    assert(bst.lcaBST(20, 40) == bst.search(30));
    assert(bst.lcaBST(20, 80) == bst.search(50));
    assert(bst.lcaBST(20, 999) == nullptr);
    assert(Validator::validateBST(bst).valid);
    TreeNode* savedChild = bst.root->left;
    bst.root->left->parent = bst.root->left;
    assert(!Validator::validateBST(bst).valid);
    bst.root->left->parent = bst.root;
    bst.root->left = bst.root;
    assert(!Validator::validateBST(bst).valid);
    bst.root->left = savedChild;
    BSTNode* leftNode = (BSTNode*)bst.root->left;
    int savedKey = leftNode->key;
    leftNode->key = 60;
    assert(!Validator::validateBST(bst).valid);
    leftNode->key = savedKey;
    TreeNode* savedRight = bst.root->right;
    bst.root->right = bst.root->left;
    assert(!Validator::validateBST(bst).valid);
    bst.root->right = savedRight;
    assert(bst.rangeQuery(25, 65) == vector<int>({30, 40, 50, 60}));
    assert(bst.rangeQuery(65, 25).empty());
    assert(bst.kthSmallest(1) == 20 && bst.kthSmallest(7) == 80);
    assert(bst.kthSmallest(0) == -1 && bst.kthSmallest(8) == -1);
    assert(bst.remove(20) && !bst.remove(20)); // leaf
    assert(Validator::validateBST(bst).valid);

    BST oneLeft;
    for (int key : {10, 5, 3}) oneLeft.insert(key);
    assert(oneLeft.remove(5));
    assert(((BSTNode*)oneLeft.root->left)->key == 3 && oneLeft.root->left->parent == oneLeft.root);
    assert(Validator::validateBST(oneLeft).valid);

    BST oneRight;
    for (int key : {10, 5, 7}) oneRight.insert(key);
    assert(oneRight.remove(5));
    assert(((BSTNode*)oneRight.root->left)->key == 7 && oneRight.root->left->parent == oneRight.root);
    assert(Validator::validateBST(oneRight).valid);

    BST rootCases;
    for (int key : {10, 5, 15, 12}) rootCases.insert(key);
    assert(rootCases.remove(10)); // two children; successor replaces root
    assert(((BSTNode*)rootCases.root)->key == 12 && rootCases.root->parent == nullptr);
    assert(Validator::validateBST(rootCases).valid);
    assert(rootCases.remove(12)); // root with one child
    assert(((BSTNode*)rootCases.root)->key == 15 && rootCases.root->parent == nullptr);
    assert(Validator::validateBST(rootCases).valid);
    assert(!rootCases.remove(99));
}

static void checkAvl(AVLTree& avl, vector<int> expected) {
    sort(expected.begin(), expected.end());
    assert(avl.inorder() == expected && avl.isBalanced());
    assert(avl.nodeCount == (int)expected.size());
    assert(Validator::validateAVL(avl).valid);
    for (int i = 0; i < (int)expected.size(); ++i) {
        assert(avl.kth(i + 1) == expected[i]);
        assert(avl.rank(expected[i]) == i + 1);
    }
    assert(avl.kth(0) == -1 && avl.kth((int)expected.size() + 1) == -1);
}

static void testAVLRotationsDeletionAndOrderStats() {
    const vector<vector<int>> cases{{30, 20, 10}, {10, 20, 30}, {30, 10, 20}, {10, 30, 20}};
    const vector<long long> expectedRotations{1, 1, 2, 2};
    for (size_t i = 0; i < cases.size(); ++i) {
        AVLTree avl;
        for (int value : cases[i]) avl.insert(value);
        assert(avl.getRotationCount() == expectedRotations[i]);
        assert(((BSTNode*)avl.root)->key == 20);
        assert(((BSTNode*)avl.root->left)->key == 10 && ((BSTNode*)avl.root->right)->key == 30);
        assert(avl.root->parent == nullptr);
        checkAvl(avl, {10, 20, 30});
        AVLNode* avlRoot = (AVLNode*)avl.root;
        ++avlRoot->size;
        assert(!Validator::validateAVL(avl).valid);
        --avlRoot->size;
    }

    // Deletion with a heavy child whose balance factor is zero.
    AVLTree avl;
    for (int value : {40, 20, 60, 10, 30, 70, 5, 15, 25, 35}) avl.insert(value);
    assert(avl.remove(70) && ((BSTNode*)avl.root)->key == 20);
    checkAvl(avl, {5, 10, 15, 20, 25, 30, 35, 40, 60});
    assert(!avl.remove(99) && !avl.insert(20));
    for (int value : {5, 15, 10, 30, 25, 35, 20, 40, 60}) {
        assert(avl.remove(value));
        vector<int> remaining;
        for (int candidate : {5, 10, 15, 20, 25, 30, 35, 40, 60})
            if (avl.search(candidate)) remaining.push_back(candidate);
        checkAvl(avl, remaining);
    }
    assert(avl.root == nullptr && avl.isBalanced());

    AVLTree ranks;
    vector<int> sorted{5, 10, 20, 25, 30};
    for (int value : {20, 5, 30, 10, 25}) ranks.insert(value);
    for (int query = 0; query <= 35; ++query)
        assert(ranks.rank(query) == (int)(upper_bound(sorted.begin(), sorted.end(), query) - sorted.begin()));
    assert(ranks.countInRange(10, 25) == 3 && ranks.countInRange(26, 19) == 0);
}

static void testMaxHeapAndHeapSort() {
    vector<vector<int>> cases{{}, {42}, {5, 4, 3, 2, 1}, {1, 2, 3, 4, 5},
                              {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5}, {-5, -1, -10, 0, 3}};
    mt19937 random(2026);
    uniform_int_distribution<int> values(-1000, 1000);
    vector<int> randomCase(257);
    for (int& value : randomCase) value = values(random);
    cases.push_back(randomCase);
    for (const auto& values : cases) {
        MaxHeap candidate(values);
        assert(candidate.isMaxHeap());
        assert(Validator::validateHeap(candidate).valid);
    }

    MaxHeap heap;
    for (int value : {3, 10, 10, -4}) heap.insert(value);
    assert(heap.isMaxHeap());
    vector<int> extracted;
    while (!heap.getHeap().empty()) {
        extracted.push_back(heap.extractMax());
        assert(heap.isMaxHeap());
    }
    assert(extracted == vector<int>({10, 10, 3, -4}));
    bool underflow = false;
    try { heap.extractMax(); } catch (const runtime_error&) { underflow = true; }
    assert(underflow);

    for (vector<int> values : cases) {
        vector<int> expected = values;
        sort(expected.begin(), expected.end());
        MaxHeap heapSort(values);
        heapSort.heapSort();
        assert(heapSort.getHeap() == expected);
        MaxHeap::heapSortArray(values);
        assert(values == expected);
    }
    MaxHeap invalidHeap;
    invalidHeap.heap = {1, 2};
    assert(!Validator::validateHeap(invalidHeap).valid);
}

int main() {
    cout << "=== VAULT HEIST: Unit Test Suite ===" << endl;
    testTraversalsAndReconstruction(); cout << "PASS: traversals and reconstruction" << endl;
    testBinaryTreeQueriesAndMirror(); cout << "PASS: LCA, paths, symmetry, and mirror" << endl;
    testBSTOperationsAndDeletionCases(); cout << "PASS: BST operations and deletion cases" << endl;
    testAVLRotationsDeletionAndOrderStats(); cout << "PASS: AVL rotations, deletion, and order statistics" << endl;
    testMaxHeapAndHeapSort(); cout << "PASS: heap operations and heap sort" << endl;
    cout << "=== ALL UNIT TESTS PASSED ===" << endl;
    return 0;
}
