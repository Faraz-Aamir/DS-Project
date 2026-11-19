#include "BinaryTree.h"

BinaryTree::BinaryTree() : root(nullptr), nodeCount(0) {}

void clearTree(TreeNode* node) {
    if (!node) return;
    clearTree(node->left);
    clearTree(node->right);
    delete node;
}

BinaryTree::~BinaryTree() {
    clearTree(root);
}

// === TRAVERSALS ===

vector<int> BinaryTree::preorder() {
    vector<int> result;
    preorderHelper(root, result);
    return result;
}

void BinaryTree::preorderHelper(TreeNode* node, vector<int>& result) {
    if (!node) return;
    result.push_back(node->value);
    preorderHelper(node->left, result);
    preorderHelper(node->right, result);
}

vector<int> BinaryTree::inorder() {
    vector<int> result;
    inorderHelper(root, result);
    return result;
}

void BinaryTree::inorderHelper(TreeNode* node, vector<int>& result) {
    if (!node) return;
    inorderHelper(node->left, result);
    result.push_back(node->value);
    inorderHelper(node->right, result);
}

vector<int> BinaryTree::postorder() {
    vector<int> result;
    postorderHelper(root, result);
    return result;
}

void BinaryTree::postorderHelper(TreeNode* node, vector<int>& result) {
    if (!node) return;
    postorderHelper(node->left, result);
    postorderHelper(node->right, result);
    result.push_back(node->value);
}

vector<int> BinaryTree::levelOrder() {
    vector<int> result;
    if (!root) return result;
    Queue<TreeNode*> q;
    q.push(root);
    while (!q.isEmpty()) {
        TreeNode* curr = q.front();
        q.pop();
        result.push_back(curr->value);
        if (curr->left) q.push(curr->left);
        if (curr->right) q.push(curr->right);
    }
    return result;
}

vector<int> BinaryTree::preorderIterative() {
    vector<int> result;
    if (!root) return result;
    vector<TreeNode*> pending{root};
    while (!pending.empty()) {
        TreeNode* node = pending.back();
        pending.pop_back();
        result.push_back(node->value);
        if (node->right) pending.push_back(node->right);
        if (node->left) pending.push_back(node->left);
    }
    return result;
}

vector<int> BinaryTree::inorderIterative() {
    vector<int> result;
    vector<TreeNode*> pending;
    TreeNode* node = root;
    while (node || !pending.empty()) {
        while (node) {
            pending.push_back(node);
            node = node->left;
        }
        node = pending.back();
        pending.pop_back();
        result.push_back(node->value);
        node = node->right;
    }
    return result;
}

vector<int> BinaryTree::postorderIterative() {
    vector<int> result;
    vector<TreeNode*> pending;
    TreeNode* node = root;
    TreeNode* lastVisited = nullptr;
    while (node || !pending.empty()) {
        if (node) {
            pending.push_back(node);
            node = node->left;
        } else {
            TreeNode* peek = pending.back();
            if (peek->right && lastVisited != peek->right) {
                node = peek->right;
            } else {
                result.push_back(peek->value);
                lastVisited = peek;
                pending.pop_back();
            }
        }
    }
    return result;
}

vector<int> BinaryTree::levelOrderRecursive() {
    vector<int> result;
    int treeHeight = height();
    for (int depth = 0; depth <= treeHeight; ++depth) {
        levelOrderAtDepth(root, depth, result);
    }
    return result;
}

void BinaryTree::levelOrderAtDepth(TreeNode* node, int depth, vector<int>& result) {
    if (!node) return;
    if (depth == 0) {
        result.push_back(node->value);
        return;
    }
    levelOrderAtDepth(node->left, depth - 1, result);
    levelOrderAtDepth(node->right, depth - 1, result);
}

// === RECONSTRUCTION ===

TreeNode* BinaryTree::buildFromPreIn(const vector<int>& pre, const vector<int>& in) {
    unordered_map<int, int> inMap;
    for (int i = 0; i < (int)in.size(); i++) {
        inMap[in[i]] = i;
    }
    int preIndex = 0;
    function<TreeNode*(int, int, TreeNode*)> build = [&](int inStart, int inEnd, TreeNode* parent) -> TreeNode* {
        if (inStart > inEnd) return nullptr;
        int rootVal = pre[preIndex++];
        TreeNode* node = new TreeNode(rootVal, rootVal);
        node->parent = parent;
        if (inStart == inEnd) return node;
        int inIndex = inMap[rootVal];
        node->left = build(inStart, inIndex - 1, node);
        node->right = build(inIndex + 1, inEnd, node);
        return node;
    };
    return build(0, (int)in.size() - 1, nullptr);
}

TreeNode* BinaryTree::buildFromPostIn(const vector<int>& post, const vector<int>& in) {
    unordered_map<int, int> inMap;
    for (int i = 0; i < (int)in.size(); i++) {
        inMap[in[i]] = i;
    }
    int postIndex = (int)post.size() - 1;
    function<TreeNode*(int, int, TreeNode*)> build = [&](int inStart, int inEnd, TreeNode* parent) -> TreeNode* {
        if (inStart > inEnd) return nullptr;
        int rootVal = post[postIndex--];
        TreeNode* node = new TreeNode(rootVal, rootVal);
        node->parent = parent;
        if (inStart == inEnd) return node;
        int inIndex = inMap[rootVal];
        node->right = build(inIndex + 1, inEnd, node);
        node->left = build(inStart, inIndex - 1, node);
        return node;
    };
    return build(0, (int)in.size() - 1, nullptr);
}

// === HEIGHT & DIAMETER ===

int BinaryTree::height() {
    return heightHelper(root);
}

int BinaryTree::heightHelper(TreeNode* node) {
    if (!node) return -1; // Edges convention: leaf = 0, null = -1
    return 1 + max(heightHelper(node->left), heightHelper(node->right));
}

int BinaryTree::diameter() {
    int diam = 0;
    diameterHelper(root, diam);
    return diam;
}

int BinaryTree::diameterHelper(TreeNode* node, int& diam) {
    if (!node) return -1;
    int leftH = diameterHelper(node->left, diam);
    int rightH = diameterHelper(node->right, diam);
    diam = max(diam, leftH + rightH + 2); // 2 edges between left and right through root
    return 1 + max(leftH, rightH);
}

// === LCA & PATH ===

TreeNode* BinaryTree::lca(int idA, int idB) {
    return lcaHelper(root, idA, idB);
}

TreeNode* BinaryTree::lcaHelper(TreeNode* node, int idA, int idB) {
    if (!node) return nullptr;
    if (node->id == idA || node->id == idB) return node;
    TreeNode* leftLCA = lcaHelper(node->left, idA, idB);
    TreeNode* rightLCA = lcaHelper(node->right, idA, idB);
    if (leftLCA && rightLCA) return node;
    return leftLCA ? leftLCA : rightLCA;
}

vector<int> BinaryTree::pathTo(int targetId) {
    vector<int> path;
    pathToHelper(root, targetId, path);
    return path;
}

vector<Event> BinaryTree::pathToWithEvents(int targetId) {
    vector<Event> events;
    if (!eventRecordingEnabled) return events;
    for (int id : pathTo(targetId)) {
        Event event{};
        event.type = EventType::PATH_TO;
        event.nodeId = id;
        events.push_back(event);
    }
    return events;
}

bool BinaryTree::pathToHelper(TreeNode* node, int targetId, vector<int>& path) {
    if (!node) return false;
    path.push_back(node->id);
    if (node->id == targetId) return true;
    if (pathToHelper(node->left, targetId, path) || pathToHelper(node->right, targetId, path)) {
        return true;
    }
    path.pop_back();
    return false;
}

// === MIRROR ===

vector<Event> BinaryTree::mirror() {
    vector<Event> events;
    mirrorHelper(root);
    // Emit a MIRROR event for the root
    if (eventRecordingEnabled && root) {
        Event e{};
        e.type = EventType::MIRROR;
        e.nodeId = root->id;
        e.oldHeight = heightHelper(root);
        events.push_back(e);
    }
    return events;
}

void BinaryTree::mirrorHelper(TreeNode* node) {
    if (!node) return;
    TreeNode* temp = node->left;
    node->left = node->right;
    node->right = temp;
    mirrorHelper(node->left);
    mirrorHelper(node->right);
}

bool BinaryTree::isSameTree(BinaryTree* other) {
    return isSameTreeHelper(this->root, other ? other->root : nullptr);
}

bool BinaryTree::isSameTreeHelper(TreeNode* a, TreeNode* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return (a->value == b->value) &&
           isSameTreeHelper(a->left, b->left) &&
           isSameTreeHelper(a->right, b->right);
}

bool BinaryTree::isSymmetric() {
    return isSymmetricHelper(root);
}

bool BinaryTree::isSymmetricHelper(TreeNode* node) {
    if (!node) return true;
    return isMirror(node->left, node->right);
}

bool BinaryTree::isMirror(TreeNode* a, TreeNode* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    return (a->value == b->value) &&
           isMirror(a->left, b->right) &&
           isMirror(a->right, b->left);
}
