#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <string>
#include "TreeNode.h"
#include "../utils/CustomContainers.h"

using namespace std;

// BinaryTree implements a general binary tree (Act 1).
// Every operation returns a list of Event objects that the GUI can animate.
// This gives us: testability, undo/replay, and step-by-step visualization.

// Event types define what happened during a tree operation
enum class EventType {
    VISIT, COMPARE, INSERT, UNLINK, SWAP_VALUE, ROTATE, MIRROR, PATH_TO,
    HIGHLIGHT_SUBTREE, BALANCE_ALERT, UPDATE_HEIGHT, UPDATE_SIZE,
    HEAP_BUILD, HEAP_SWAP, HEAPIFY_START, HEAPIFY_DONE, EXTRACT_MAX,
    MARK_SORTED, HEAP_SIZE_CHANGE, HEAP_SORT_COMPLETE, UNDO, ANSWER
};

// Event stores all data needed for undo/replay/animation
struct Event {
    EventType type;
    int nodeId;
    int otherNodeId;
    string side;
    int oldHeight;
    int oldSize;
    int balanceFactor;
    int pivotId;
    int traversalStep;
    bool isLeft;
};

class BinaryTree {
public:
    inline static bool eventRecordingEnabled = true;
    TreeNode* root;
    int nodeCount;

    BinaryTree();
    ~BinaryTree();

    // === TRAVERSALS ===
    vector<int> preorder();
    vector<int> inorder();
    vector<int> postorder();
    vector<int> levelOrder();
    vector<int> preorderIterative();
    vector<int> inorderIterative();
    vector<int> postorderIterative();
    vector<int> levelOrderRecursive();

    // === RECONSTRUCTION ===
    TreeNode* buildFromPreIn(const vector<int>& pre, const vector<int>& in);
    TreeNode* buildFromPostIn(const vector<int>& post, const vector<int>& in);

    // === HEIGHT & DIAMETER ===
    int height();
    int diameter();

    // === LCA & PATH ===
    TreeNode* lca(int idA, int idB);
    vector<int> pathTo(int targetId);
    vector<Event> pathToWithEvents(int targetId);
    static void setEventRecordingEnabled(bool enabled) { eventRecordingEnabled = enabled; }

    // === MIRROR ===
    vector<Event> mirror();
    bool isSameTree(BinaryTree* other);
    bool isSymmetric();

    // === ACCESSORS ===
    TreeNode* getRoot() { return root; }
    int getNodeCount() { return nodeCount; }

private:
    void preorderHelper(TreeNode* node, vector<int>& result);
    void inorderHelper(TreeNode* node, vector<int>& result);
    void postorderHelper(TreeNode* node, vector<int>& result);
    void levelOrderHelper(TreeNode* node, vector<int>& result);
    void levelOrderAtDepth(TreeNode* node, int depth, vector<int>& result);
    int heightHelper(TreeNode* node);
    int diameterHelper(TreeNode* node, int& diam);
    TreeNode* lcaHelper(TreeNode* node, int idA, int idB);
    bool pathToHelper(TreeNode* node, int targetId, vector<int>& path);
    void mirrorHelper(TreeNode* node);
    bool isSameTreeHelper(TreeNode* a, TreeNode* b);
    bool isSymmetricHelper(TreeNode* node);
    bool isMirror(TreeNode* a, TreeNode* b);
};

#endif
