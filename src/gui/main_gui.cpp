#include "raylib.h"
#include "../models/BST.h"
#include "../models/MaxHeap.h"
#include "LayoutEngine.h"
#include "Renderer.h"
#include "../level/LevelLoader.h"
#include <string>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <random>
#include <fstream>

// ---------------------------------------------------------------
// Helpers (data side, no GUI)
// ---------------------------------------------------------------
static void renumberNodes(TreeNode* node, int& nextId) {
    if (!node) return;
    renumberNodes(node->left, nextId);
    node->id = nextId++;
    renumberNodes(node->right, nextId);
}

// Preorder keys of the current tree
static void collectKeys(TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    out.push_back(((BSTNode*)n)->key);
    collectKeys(n->left, out);
    collectKeys(n->right, out);
}

static void collectInorderKeys(TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    collectInorderKeys(n->left, out);
    out.push_back(((BSTNode*)n)->key);
    collectInorderKeys(n->right, out);
}

static void collectPreorderIds(TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    out.push_back(n->id);
    collectPreorderIds(n->left, out);
    collectPreorderIds(n->right, out);
}

static void collectInternalIds(TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    if (n->left || n->right) out.push_back(n->id);
    collectInternalIds(n->left, out);
    collectInternalIds(n->right, out);
}

static void collectRangeIds(TreeNode* n, int low, int high, std::vector<int>& out) {
    if (!n) return;
    int key = ((BSTNode*)n)->key;
    if (key > low) collectRangeIds(n->left, low, high, out);
    if (key >= low && key <= high) out.push_back(n->id);
    if (key < high) collectRangeIds(n->right, low, high, out);
}

static int subtreeHeight(TreeNode* n) {
    if (!n) return -1;
    return 1 + std::max(subtreeHeight(n->left), subtreeHeight(n->right));
}

static int depthOfNode(TreeNode* node, int id, int depth = 0) {
    if (!node) return -1;
    if (node->id == id) return depth;
    int leftDepth = depthOfNode(node->left, id, depth + 1);
    return leftDepth >= 0 ? leftDepth : depthOfNode(node->right, id, depth + 1);
}

static void collectUnbalancedIds(TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    if (std::abs(subtreeHeight(n->left) - subtreeHeight(n->right)) > 1) out.push_back(n->id);
    collectUnbalancedIds(n->left, out);
    collectUnbalancedIds(n->right, out);
}

static void appendMedianOrder(const std::vector<int>& sorted, int low, int high, std::vector<int>& out) {
    if (low > high) return;
    int mid = low + (high - low) / 2;
    out.push_back(sorted[mid]);
    appendMedianOrder(sorted, low, mid - 1, out);
    appendMedianOrder(sorted, mid + 1, high, out);
}

static TreeNode* findNodeById(TreeNode* n, int id) {
    if (!n || n->id == id) return n;
    TreeNode* found = findNodeById(n->left, id);
    return found ? found : findNodeById(n->right, id);
}

static void clearNodes(TreeNode* n) {
    if (!n) return;
    clearNodes(n->left);
    clearNodes(n->right);
    delete n;
}

static TreeNode* buildHeapView(const std::vector<int>& values, int index, TreeNode* parent, int& nextId) {
    if (index >= (int)values.size()) return nullptr;
    BSTNode* node = new BSTNode(values[index], values[index]);
    node->id = nextId++;
    node->parent = parent;
    node->left = buildHeapView(values, index * 2 + 1, node, nextId);
    node->right = buildHeapView(values, index * 2 + 2, node, nextId);
    return node;
}

static bool rotateRightAt(BST& tree, int key) {
    BSTNode* pivot = tree.search(key);
    if (!pivot || !pivot->left) return false;
    TreeNode* promoted = pivot->left;
    TreeNode* parent = pivot->parent;
    TreeNode* middle = promoted->right;
    promoted->parent = parent;
    if (!parent) tree.root = promoted;
    else if (parent->left == pivot) parent->left = promoted;
    else parent->right = promoted;
    promoted->right = pivot;
    pivot->parent = promoted;
    pivot->left = middle;
    if (middle) middle->parent = pivot;
    return true;
}

static bool rotateLeftAt(BST& tree, int key) {
    BSTNode* pivot = tree.search(key);
    if (!pivot || !pivot->right) return false;
    TreeNode* promoted = pivot->right;
    TreeNode* parent = pivot->parent;
    TreeNode* middle = promoted->left;
    promoted->parent = parent;
    if (!parent) tree.root = promoted;
    else if (parent->left == pivot) parent->left = promoted;
    else parent->right = promoted;
    promoted->left = pivot;
    pivot->parent = promoted;
    pivot->right = middle;
    if (middle) middle->parent = pivot;
    return true;
}

static bool collectSearchPath(TreeNode* n, int key, std::vector<int>& out) {
    if (!n) return false;
    out.push_back(n->id);
    int nodeKey = ((BSTNode*)n)->key;
    if (nodeKey == key) return true;
    if (collectSearchPath(key < nodeKey ? n->left : n->right, key, out)) return true;
    out.pop_back();
    return false;
}

// Snapshot = preorder with null markers, e.g. "50 30 20 # # # 70 # #"
static void serializeNode(TreeNode* n, std::ostringstream& oss) {
    if (!n) { oss << "# "; return; }
    oss << ((BSTNode*)n)->key << " ";
    serializeNode(n->left, oss);
    serializeNode(n->right, oss);
}
static std::string serializeTree(TreeNode* root) {
    std::ostringstream oss;
    serializeNode(root, oss);
    return oss.str();
}

static TreeNode* deserializeNode(std::istringstream& input, TreeNode* parent, int& nextId) {
    std::string token;
    if (!(input >> token) || token == "#") return nullptr;
    int key = std::stoi(token);
    BSTNode* node = new BSTNode(key, key);
    node->id = nextId++;
    node->parent = parent;
    node->left = deserializeNode(input, node, nextId);
    node->right = deserializeNode(input, node, nextId);
    return node;
}

static TreeNode* deserializeTree(const std::string& data, int& nodeCount) {
    std::istringstream input(data);
    int nextId = 1;
    TreeNode* root = deserializeNode(input, nullptr, nextId);
    nodeCount = nextId - 1;
    return root;
}

// Keys of a snapshot in preorder. Re-inserting them in this order
// rebuilds exactly the same BST shape.
static std::vector<int> snapshotKeys(const std::string& data) {
    std::vector<int> keys;
    std::istringstream iss(data);
    std::string t;
    while (iss >> t) {
        if (t != "#") keys.push_back(std::stoi(t));
    }
    return keys;
}

int main() {
    LevelLoader loader;
    std::string levelsPath = std::string(GetApplicationDirectory()) + "data/levels.json";
    bool levelsLoaded = loader.load(levelsPath);
    if (!levelsLoaded) levelsLoaded = loader.load("data/levels.json");
    int totalLevels = 0;
    for (const Act& act : loader.acts) totalLevels += (int)act.levels.size();

    int actIdx = 0;
    int levelIdx = 0;
    const std::string activeStatePath = std::string(GetApplicationDirectory()) + "data/current_state.txt";
    {
        std::ifstream stateFile(activeStatePath);
        std::string savedLevelId;
        if (stateFile >> savedLevelId) {
            for (int a = 0; a < (int)loader.acts.size(); ++a) {
                for (int l = 0; l < (int)loader.acts[a].levels.size(); ++l) {
                    if (loader.acts[a].levels[l].id == savedLevelId) {
                        actIdx = a;
                        levelIdx = l;
                    }
                }
            }
        }
    }

    BST tree;
    std::vector<int> currentLevelKeys;
    std::vector<int> patrolOrder;
    std::vector<int> patrolVisited;
    std::vector<int> preorderClue;
    std::vector<int> inorderClue;
    bool reconstructionComplete = false;
    int heightTarget = 0;
    int diameterTarget = 0;
    int heightDiameterStep = 0;
    std::vector<int> lcaSelected;
    int lcaExpectedId = -1;
    bool lcaComplete = false;
    std::vector<int> mirrorTargets;
    std::vector<int> mirrorFlipped;
    int rangeLow = 0;
    int rangeHigh = 0;
    std::vector<int> rangeExpected;
    std::vector<int> rangeVisited;
    std::vector<int> unbalancedTargets;
    std::vector<int> unbalancedSelected;
    bool singleRotationComplete = false;
    int doubleRotationStep = 0;
    std::string singleRotationCase = "LL";
    std::string doubleRotationCase = "LR";
    bool insertPredictComplete = false;
    int orderStatisticK = 5;
    int orderStatisticTarget = -1;
    int orderStatisticTargetId = -1;
    bool orderStatisticComplete = false;
    MaxHeap heapState;
    std::vector<int> extractedValues;
    int activeHeapSize = 0;
    bool heapTaskComplete = false;
    std::unordered_set<std::string> completedLevels;
    std::unordered_map<std::string, int> bestScores;
    std::unordered_map<std::string, int> bestStars;
    int mistakesThisLevel = 0;
    int alarmSteps = 0;
    int alarmCapacity = 12;
    int scorePenalties = 0;
    int starsEarned = 0;
    int starCap = 3;
    bool alarmFailed = false;
    std::string lastCountedError;
    const std::string progressPath = std::string(GetApplicationDirectory()) + "data/progress.txt";
    std::unordered_set<std::string> validLevelIds;
    for (const Act& act : loader.acts) {
        for (const Level& level : act.levels) validLevelIds.insert(level.id);
    }
    {
        std::ifstream progressFile(progressPath);
        std::string line;
        while (std::getline(progressFile, line)) {
            std::istringstream entry(line);
            std::string savedId;
            int savedScore = 1000;
            if (entry >> savedId && validLevelIds.count(savedId)) {
                entry >> savedScore;
                completedLevels.insert(savedId);
                bestScores[savedId] = savedScore;
                int savedStars = 0;
                if (entry >> savedStars) bestStars[savedId] = savedStars;
            }
        }
    }
    auto saveCampaignProgress = [&]() {
        std::ofstream progressFile(progressPath, std::ios::trunc);
        if (!progressFile.is_open()) return;
        for (const std::string& id : completedLevels)
            progressFile << id << ' ' << (bestScores.count(id) ? bestScores[id] : 0) << ' '
                         << (bestStars.count(id) ? bestStars[id] : 0) << '\n';
    };
    std::vector<int> searchPath;
    std::vector<int> searchVisited;
    int searchTarget = -1;
    int insertTarget = -1;
    bool insertCompleted = false;
    int deleteTarget = -1;
    bool deleteCompleted = false;

    auto isPatrolLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "1.1";
    };
    auto isReconstructionLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "1.2";
    };
    auto isHeightDiameterLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "1.3";
    };
    auto isLcaLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "1.4";
    };
    auto isSearchLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "2.1";
    };
    auto isMirrorLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "1.5";
    };
    auto isRangeLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "2.4";
    };
    auto isDegenerationLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "2.5";
    };
    auto isBalanceLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "3.1";
    };
    auto isSingleRotateLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "3.2";
    };
    auto isDoubleRotateLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "3.3";
    };
    auto isInsertPredictLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "3.4";
    };
    auto isOrderStatisticLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "3.5";
    };
    auto isHeapLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id.size() > 0 &&
               loader.acts[actIdx].levels[levelIdx].id[0] == '4';
    };

    auto isCurrentLevelComplete = [&]() {
        if (!levelsLoaded || loader.acts.empty()) return false;
        const std::string& id = loader.acts[actIdx].levels[levelIdx].id;
        if (id == "1.1") return patrolVisited.size() == patrolOrder.size() && !patrolOrder.empty();
        if (id == "1.2") return reconstructionComplete;
        if (id == "1.3") return heightDiameterStep >= 2;
        if (id == "1.4") return lcaComplete;
        if (id == "1.5") return !mirrorTargets.empty() && mirrorFlipped.size() == mirrorTargets.size();
        if (id == "2.1" || id == "2.5") return !searchPath.empty() && searchVisited.size() == searchPath.size();
        if (id == "2.2") return insertCompleted;
        if (id == "2.3") return deleteCompleted;
        if (id == "2.4") return !rangeExpected.empty() && rangeVisited.size() == rangeExpected.size();
        if (id == "3.1") return !unbalancedTargets.empty() && unbalancedSelected.size() == unbalancedTargets.size();
        if (id == "3.2") return singleRotationComplete;
        if (id == "3.3") return doubleRotationStep >= 2;
        if (id == "3.4") return insertPredictComplete;
        if (id == "3.5") return orderStatisticComplete;
        if (id[0] == '4') return heapTaskComplete;
        return false;
    };
    auto optimalSteps = [&]() {
        const std::string& id = loader.acts[actIdx].levels[levelIdx].id;
        if (id == "1.1") return (int)patrolOrder.size();
        if (id == "1.2" || id == "2.2" || id == "2.3" || id == "3.2" || id == "3.4" || id == "3.5" || id == "4.1" || id == "4.2") return 1;
        if (id == "1.3") return 2;
        if (id == "1.4") return 3;
        if (id == "1.5") return std::max(1, (int)mirrorTargets.size());
        if (id == "2.1" || id == "2.5") return std::max(1, (int)searchPath.size());
        if (id == "2.4") return std::max(1, (int)rangeExpected.size());
        if (id == "3.1") return std::max(1, (int)unbalancedTargets.size());
        if (id == "3.3") return 2;
        if (id == "4.3") return (int)currentLevelKeys.size();
        if (id == "4.4" || id == "4.5") return std::max(1, (int)currentLevelKeys.size() - 1);
        return std::max(1, (int)currentLevelKeys.size());
    };
    auto isGuidedLevel = [&]() {
        return isPatrolLevel() || isSearchLevel() || isLcaLevel() || isMirrorLevel() ||
               isRangeLevel() || isDegenerationLevel() || isBalanceLevel() || isOrderStatisticLevel();
    };
    auto isInsertLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "2.2";
    };
    auto isDeleteLevel = [&]() {
        return levelsLoaded && !loader.acts.empty() &&
               loader.acts[actIdx].levels[levelIdx].id == "2.3";
    };

    auto generateLevel = [&](int nodes, int seed) {
        std::vector<int> keys;
        std::mt19937 gen(seed);
        std::uniform_int_distribution<> dis(10, 99);
        while ((int)keys.size() < nodes) {
            int val = dis(gen);
            bool dup = false;
            for (int k : keys) if (k == val) dup = true;
            if (!dup) keys.push_back(val);
        }
        return keys;
    };

    auto resetToLevel = [&]() {
        if (!levelsLoaded || loader.acts.empty()) return;
        mistakesThisLevel = 0;
        alarmSteps = 0;
        alarmCapacity = 12;
        scorePenalties = 0;
        starsEarned = 0;
        starCap = 3;
        alarmFailed = false;
        lastCountedError.clear();
        const auto& lvl = loader.acts[actIdx].levels[levelIdx];
        int itemCount = lvl.nodes > 0 ? lvl.nodes : (lvl.elements > 0 ? lvl.elements : 7);
        if (lvl.id == "2.3") {
            currentLevelKeys = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
        } else if (lvl.id == "2.5" || lvl.id == "3.1") {
            currentLevelKeys.clear();
            for (int i = 0; i < itemCount; ++i) currentLevelKeys.push_back(10 + i * 6);
        } else if (lvl.id == "3.2") {
            currentLevelKeys = singleRotationCase == "LL"
                ? std::vector<int>{50, 30, 70, 20, 60, 80, 10}
                : std::vector<int>{50, 30, 70, 80, 90};
        } else if (lvl.id == "3.3") {
            currentLevelKeys = doubleRotationCase == "LR"
                ? std::vector<int>{50, 20, 70, 40, 30, 45, 60}
                : std::vector<int>{50, 30, 70, 60, 65, 80};
        } else if (lvl.id == "3.4") {
            currentLevelKeys = {30, 20};
        } else if (lvl.id == "3.5") {
            std::vector<int> sorted;
            for (int i = 1; i <= itemCount; ++i) sorted.push_back(i * 10);
            currentLevelKeys.clear();
            appendMedianOrder(sorted, 0, (int)sorted.size() - 1, currentLevelKeys);
        } else if (lvl.id == "4.2") {
            currentLevelKeys = {1, 20, 3, 18, 16, 2, 10, 14, 8, 9, 5, 4};
        } else {
            currentLevelKeys = generateLevel(itemCount, lvl.seed);
        }

        // Replace the current tree, including mirrored (non-BST) shapes.
        clearNodes(tree.getRoot());
        tree.root = nullptr;
        tree.nodeCount = 0;

        // Load the level's tree or complete-binary-tree heap view.
        if (isHeapLevel()) {
            heapState.heap = currentLevelKeys;
            extractedValues.clear();
            activeHeapSize = (int)heapState.heap.size();
            heapTaskComplete = false;
            if (lvl.id == "4.3" || lvl.id == "4.4" || lvl.id == "4.5") {
                std::vector<int> input = heapState.heap;
                heapState.buildMaxHeap(input);
            }
            int nextId = 1;
            tree.root = buildHeapView(heapState.heap, 0, nullptr, nextId);
            tree.nodeCount = nextId - 1;
        } else {
            for (int k : currentLevelKeys) tree.insert(k);
        }
        patrolOrder.clear();
        patrolVisited.clear();
        preorderClue.clear();
        inorderClue.clear();
        reconstructionComplete = false;
        heightDiameterStep = 0;
        lcaSelected.clear();
        lcaExpectedId = -1;
        lcaComplete = false;
        mirrorTargets.clear();
        mirrorFlipped.clear();
        rangeExpected.clear();
        rangeVisited.clear();
        unbalancedTargets.clear();
        unbalancedSelected.clear();
        singleRotationComplete = false;
        doubleRotationStep = 0;
        insertPredictComplete = false;
        orderStatisticTarget = -1;
        orderStatisticTargetId = -1;
        orderStatisticComplete = false;
        rangeLow = rangeHigh = 0;
        searchPath.clear();
        searchVisited.clear();
        searchTarget = -1;
        insertTarget = -1;
        insertCompleted = false;
        deleteTarget = isDeleteLevel() ? 50 : -1;
        deleteCompleted = false;
        if (isPatrolLevel()) collectPreorderIds(tree.getRoot(), patrolOrder);
        if ((isSearchLevel() || isDegenerationLevel()) && !currentLevelKeys.empty()) {
            std::vector<int> sortedKeys = currentLevelKeys;
            std::sort(sortedKeys.begin(), sortedKeys.end());
            searchTarget = isDegenerationLevel() ? sortedKeys.back() : sortedKeys[sortedKeys.size() / 2];
        }
        if (isInsertLevel()) {
            for (int candidate = 42; candidate <= 99 && insertTarget < 0; candidate++) {
                if (std::find(currentLevelKeys.begin(), currentLevelKeys.end(), candidate) == currentLevelKeys.end())
                    insertTarget = candidate;
            }
            for (int candidate = 10; candidate < 42 && insertTarget < 0; candidate++) {
                if (std::find(currentLevelKeys.begin(), currentLevelKeys.end(), candidate) == currentLevelKeys.end())
                    insertTarget = candidate;
            }
        }
        if (isReconstructionLevel()) {
            collectKeys(tree.getRoot(), preorderClue);
            collectInorderKeys(tree.getRoot(), inorderClue);
        }
        if (isHeightDiameterLevel()) {
            heightTarget = tree.height();
            diameterTarget = tree.diameter();
        }
        if (isOrderStatisticLevel() && currentLevelKeys.size() >= (size_t)orderStatisticK) {
            std::vector<int> sortedKeys = currentLevelKeys;
            std::sort(sortedKeys.begin(), sortedKeys.end());
            orderStatisticTarget = sortedKeys[orderStatisticK - 1];
        }
    };

    resetToLevel();

    std::vector<std::string> undoStack;
    LayoutEngine layout; Renderer renderer;

    const int WIN_W = 1100, WIN_H = 850;
    InitWindow(WIN_W, WIN_H, "Vault Heist");
    SetTargetFPS(60);

    // ---- Layout: every row is computed from the previous row ----
    const int titleH   = 50;
    const int pad      = 10;
    const int rowH     = 30;
    const int row1Y    = titleH + 15;                 // inputs + Insert/Delete
    const int row2Y    = row1Y + rowH + pad;          // Undo / Reset / Undo steps
    const int instrY   = row2Y + rowH + 12;           // instruction line
    const int statusY  = instrY + 16 + 8;             // status line
    const int panelBot = statusY + 18 + 12;           // bottom of grey panel
    const int canvasX  = 20;
    const int canvasY  = panelBot + pad;
    const int canvasW  = WIN_W - 2 * canvasX;
    const int canvasH  = WIN_H - canvasY - 20;
    Rectangle canvas = { (float)canvasX, (float)canvasY, (float)canvasW, (float)canvasH };
    float cameraZoom = 1.0f;
    Vector2 cameraPan = {0, 0};
    Vector2 previousMouse = {0, 0};
    bool panning = false;

    const Color OK_C  = Color{0, 120, 60, 255};
    const Color ERR_C = Color{170, 30, 30, 255};

    char insB[16] = "\0", delB[16] = "\0";
    int active = 0, iL = 0, dL = 0;
    std::string stat = "Ready.";
    Color statC = DARKGRAY;

    auto chargeAlarm = [&](int amount = 1) {
        if (alarmFailed || isCurrentLevelComplete()) return;
        alarmSteps += amount;
        alarmCapacity = std::max(1, (3 * optimalSteps() + 1) / 2 + 2);
        if (alarmSteps >= alarmCapacity) {
            alarmSteps = alarmCapacity;
            alarmFailed = true;
            stat = "Alarm full. Reset this level to try again.";
            statC = ERR_C;
        }
    };

    auto relayout = [&]() {
        int id = 1; renumberNodes(tree.getRoot(), id);
        layout.calculateLayout(tree.getRoot(), canvas, 50, 80, 80.0f);
        if (isPatrolLevel()) {
            patrolOrder.clear();
            collectPreorderIds(tree.getRoot(), patrolOrder);
        }
        if (isSearchLevel() || isDegenerationLevel()) {
            searchPath.clear();
            collectSearchPath(tree.getRoot(), searchTarget, searchPath);
        }
        if (isMirrorLevel()) {
            mirrorTargets.clear();
            collectInternalIds(tree.getRoot(), mirrorTargets);
        }
        if (isRangeLevel() && currentLevelKeys.size() >= 3) {
            std::vector<int> sortedKeys = currentLevelKeys;
            std::sort(sortedKeys.begin(), sortedKeys.end());
            size_t mid = sortedKeys.size() / 2;
            rangeLow = sortedKeys[mid - 1];
            rangeHigh = sortedKeys[mid + 1];
            rangeExpected.clear();
            collectRangeIds(tree.getRoot(), rangeLow, rangeHigh, rangeExpected);
        }
        if (isOrderStatisticLevel()) {
            BSTNode* target = tree.search(orderStatisticTarget);
            orderStatisticTargetId = target ? target->id : -1;
        }
        if (isBalanceLevel()) {
            unbalancedTargets.clear();
            collectUnbalancedIds(tree.getRoot(), unbalancedTargets);
        }
    };

    auto refreshHeapView = [&]() {
        clearNodes(tree.getRoot());
        int nextId = 1;
        tree.root = buildHeapView(heapState.heap, 0, nullptr, nextId);
        tree.nodeCount = nextId - 1;
        relayout();
    };

    {
        std::ifstream stateFile(activeStatePath);
        std::string savedId, kind;
        if (stateFile >> savedId >> kind && savedId == loader.acts[actIdx].levels[levelIdx].id) {
            if (kind == "T") {
                std::string snapshot;
                std::getline(stateFile, snapshot);
                std::getline(stateFile, snapshot);
                clearNodes(tree.getRoot());
                tree.root = deserializeTree(snapshot, tree.nodeCount);
                relayout();
            } else if (kind == "H") {
                int count = 0, extractedCount = 0;
                stateFile >> activeHeapSize >> count >> extractedCount;
                heapState.heap.resize(std::max(0, count));
                for (int& value : heapState.heap) stateFile >> value;
                extractedValues.resize(std::max(0, extractedCount));
                for (int& value : extractedValues) stateFile >> value;
                refreshHeapView();
            }
        }
    }

    auto saveActiveState = [&]() {
        std::ofstream stateFile(activeStatePath, std::ios::trunc);
        if (!stateFile.is_open()) return;
        stateFile << loader.acts[actIdx].levels[levelIdx].id << '\n';
        if (isHeapLevel()) {
            stateFile << "H " << activeHeapSize << ' ' << heapState.heap.size() << ' '
                      << extractedValues.size();
            for (int value : heapState.heap) stateFile << ' ' << value;
            for (int value : extractedValues) stateFile << ' ' << value;
            stateFile << '\n';
        } else {
            stateFile << "T\n" << serializeTree(tree.getRoot()) << '\n';
        }
    };

    // Replace the whole tree with the given keys (uses only insert/remove)
    auto loadKeys = [&](const std::vector<int>& keys) {
        std::vector<int> cur;
        collectKeys(tree.getRoot(), cur);
        for (int k : cur) tree.remove(k);
        for (int k : keys) tree.insert(k);
        relayout();
    };

    auto doUndo = [&]() {
        if (isOrderStatisticLevel()) {
            orderStatisticComplete = false;
            stat = "Target selection undone."; statC = OK_C;
            return;
        }
        if (isInsertPredictLevel() && !undoStack.empty()) insertPredictComplete = false;
        if (isDoubleRotateLevel() && !undoStack.empty() && doubleRotationStep > 0)
            --doubleRotationStep;
        if (isSingleRotateLevel() && !undoStack.empty()) singleRotationComplete = false;
        if (isBalanceLevel()) {
            if (!unbalancedSelected.empty()) {
                unbalancedSelected.pop_back();
                stat = "Last room selection undone."; statC = OK_C;
            } else {
                stat = "No room selections to undo."; statC = ERR_C;
            }
            return;
        }
        if (isRangeLevel()) {
            if (!rangeVisited.empty()) {
                rangeVisited.pop_back();
                stat = "Last range selection undone."; statC = OK_C;
            } else {
                stat = "No range selections to undo."; statC = ERR_C;
            }
            return;
        }
        if (isMirrorLevel()) {
            if (!mirrorFlipped.empty()) {
                int id = mirrorFlipped.back();
                mirrorFlipped.pop_back();
                TreeNode* node = findNodeById(tree.getRoot(), id);
                if (node) std::swap(node->left, node->right);
                layout.calculateLayout(tree.getRoot(), canvas, 50, 80, 80.0f);
                stat = "Last mirror action undone."; statC = OK_C;
            } else {
                stat = "No mirror actions to undo."; statC = ERR_C;
            }
            return;
        }
        if (isLcaLevel()) {
            if (lcaComplete) {
                lcaComplete = false;
                stat = "LCA answer cleared."; statC = OK_C;
            } else if (!lcaSelected.empty()) {
                lcaSelected.pop_back();
                stat = "Last room selection undone."; statC = OK_C;
            } else {
                stat = "No room selections to undo."; statC = ERR_C;
            }
            TreeNode* ancestor = lcaSelected.size() == 2
                                     ? tree.lca(lcaSelected[0], lcaSelected[1])
                                     : nullptr;
            lcaExpectedId = ancestor ? ancestor->id : -1;
            return;
        }
        if (isHeightDiameterLevel()) {
            if (heightDiameterStep > 0) --heightDiameterStep;
            insB[0] = '\0'; iL = 0;
            stat = "Last answer undone."; statC = OK_C;
            return;
        }
        if (isSearchLevel()) {
            if (searchVisited.empty()) {
                stat = "No search steps to undo"; statC = ERR_C;
            } else {
                searchVisited.pop_back();
                stat = "Last search choice undone"; statC = OK_C;
            }
            return;
        }
        if (isPatrolLevel()) {
            if (patrolVisited.empty()) {
                stat = "No patrol steps to undo"; statC = ERR_C;
            } else {
                patrolVisited.pop_back();
                stat = "Last patrol choice undone"; statC = OK_C;
            }
            return;
        }
        if (undoStack.empty()) {
            stat = "Nothing to undo"; statC = ERR_C; return;
        }
        std::string snap = undoStack.back();
        undoStack.pop_back();
        loadKeys(snapshotKeys(snap));
        if (isInsertLevel()) insertCompleted = false;
        if (isDeleteLevel()) deleteCompleted = false;
        stat = "Undone"; statC = OK_C;
    };

    auto doReset = [&]() {
        mistakesThisLevel = 0;
        lastCountedError.clear();
        alarmSteps = 0;
        scorePenalties = 0;
        starsEarned = 0;
        starCap = 3;
        alarmFailed = false;
        if (isHeapLevel()) {
            resetToLevel();
            relayout();
            stat = "Heap level reset."; statC = OK_C;
            return;
        }
        if (isOrderStatisticLevel()) {
            orderStatisticComplete = false;
            stat = "Level reset. Find the requested order statistic."; statC = OK_C;
            return;
        }
        if (isInsertPredictLevel()) {
            loadKeys(currentLevelKeys);
            undoStack.clear();
            insertPredictComplete = false;
            stat = "Level reset. Predict the insertion rotation."; statC = OK_C;
            return;
        }
        if (isDoubleRotateLevel()) {
            loadKeys(currentLevelKeys);
            undoStack.clear();
            doubleRotationStep = 0;
            stat = "Level reset. Begin the " + doubleRotationCase + " rotation sequence."; statC = OK_C;
            return;
        }
        if (isSingleRotateLevel()) {
            loadKeys(currentLevelKeys);
            undoStack.clear();
            singleRotationComplete = false;
            stat = "Level reset. Repair the " + singleRotationCase + " imbalance."; statC = OK_C;
            return;
        }
        if (isBalanceLevel()) {
            unbalancedSelected.clear();
            stat = "Level reset. Select the unbalanced rooms."; statC = OK_C;
            return;
        }
        if (isRangeLevel()) {
            rangeVisited.clear();
            stat = "Range query reset."; statC = OK_C;
            return;
        }
        if (isMirrorLevel()) {
            resetToLevel();
            relayout();
            stat = "Vault restored. Mirror each branch."; statC = OK_C;
            return;
        }
        if (isLcaLevel()) {
            lcaSelected.clear();
            lcaExpectedId = -1;
            lcaComplete = false;
            stat = "Level reset. Select two rooms."; statC = OK_C;
            return;
        }
        if (isHeightDiameterLevel()) {
            heightDiameterStep = 0;
            insB[0] = '\0'; iL = 0;
            stat = "Level reset. Find the height first."; statC = OK_C;
            return;
        }
        if (isReconstructionLevel()) {
            reconstructionComplete = false;
            insB[0] = '\0'; iL = 0;
            stat = "Blueprint reset. Find the root from the traversals."; statC = OK_C;
            return;
        }
        if (isInsertLevel()) {
            loadKeys(currentLevelKeys);
            undoStack.clear();
            insertCompleted = false;
            stat = "Level reset. Insert the target key."; statC = OK_C;
            return;
        }
        if (isSearchLevel()) {
            searchVisited.clear();
            stat = "Search reset. Start at the root."; statC = OK_C;
            return;
        }
        if (isPatrolLevel()) {
            patrolVisited.clear();
            stat = "Patrol reset. Start at the root."; statC = OK_C;
            return;
        }
        loadKeys(currentLevelKeys);
        undoStack.clear();
        if (isDeleteLevel()) deleteCompleted = false;
        stat = "Level Reset"; statC = OK_C;
    };

    auto doOp = [&](bool ins) {
        if (alarmFailed) return;
        chargeAlarm();
        if (alarmFailed) return;
        if (ins && iL > 0) {
            int v = std::stoi(insB);
            if (isInsertLevel() && insertCompleted) {
                stat = "Target already inserted. Reset to retry."; statC = ERR_C;
            } else if (isInsertLevel() && v != insertTarget) {
                stat = "Insert the target key " + std::to_string(insertTarget); statC = ERR_C;
            } else {
                undoStack.push_back(serializeTree(tree.getRoot()));
                if (tree.insert(v)) {
                    if (isInsertLevel()) {
                        insertCompleted = true;
                        stat = "Target secured! Insert level complete.";
                    } else {
                        stat = "Inserted " + std::to_string(v);
                    }
                    statC = OK_C;
                } else {
                    undoStack.pop_back();
                    stat = "Duplicate " + std::to_string(v) + " rejected"; statC = ERR_C;
                }
            }
            insB[0] = '\0'; iL = 0;
        } else if (!ins && dL > 0) {
            int v = std::stoi(delB);
            if (isDeleteLevel() && deleteCompleted) {
                stat = "Target already removed. Reset to retry."; statC = ERR_C;
            } else if (isDeleteLevel() && v != deleteTarget) {
                stat = "Remove the highlighted target node: " + std::to_string(deleteTarget); statC = ERR_C;
            } else if (!tree.search(v)) {
                stat = "Key " + std::to_string(v) + " not found"; statC = ERR_C;
            } else {
                undoStack.push_back(serializeTree(tree.getRoot()));
                tree.remove(v);
                if (isDeleteLevel()) {
                    deleteCompleted = true;
                    stat = "Target removed. Successor replacement complete.";
                } else {
                    stat = "Deleted " + std::to_string(v);
                }
                statC = OK_C;
            }
            delB[0] = '\0'; dL = 0;
        } else {
            stat = "Type a number first"; statC = ERR_C;
        }
        relayout();
    };

    auto doSingleRotation = [&]() {
        if (alarmFailed) return;
        chargeAlarm();
        if (alarmFailed) return;
        if (singleRotationComplete) {
            stat = "Rotation complete. Reset to try again."; statC = OK_C;
            return;
        }
        undoStack.push_back(serializeTree(tree.getRoot()));
        const bool rotated = singleRotationCase == "LL"
            ? rotateRightAt(tree, 30) : rotateLeftAt(tree, 70);
        if (rotated) {
            relayout();
            std::vector<int> remaining;
            collectUnbalancedIds(tree.getRoot(), remaining);
            if (remaining.empty()) {
                singleRotationComplete = true;
                stat = (singleRotationCase == "LL" ? "Right" : "Left") +
                       std::string(" rotation repaired the imbalance."); statC = OK_C;
            } else {
                stat = "Rotation applied, but an imbalance remains."; statC = ERR_C;
            }
        } else {
            undoStack.pop_back();
            stat = "This node cannot rotate right."; statC = ERR_C;
        }
    };

    auto doDoubleRotation = [&]() {
        if (alarmFailed) return;
        chargeAlarm();
        if (alarmFailed) return;
        if (doubleRotationStep >= 2) {
            stat = "Double rotation complete. Reset to try again."; statC = OK_C;
            return;
        }
        undoStack.push_back(serializeTree(tree.getRoot()));
        bool rotated;
        if (doubleRotationCase == "LR")
            rotated = doubleRotationStep == 0 ? rotateLeftAt(tree, 20) : rotateRightAt(tree, 50);
        else
            rotated = doubleRotationStep == 0 ? rotateRightAt(tree, 70) : rotateLeftAt(tree, 50);
        if (!rotated) {
            undoStack.pop_back();
            stat = "The required pivot is not available."; statC = ERR_C;
            return;
        }
        ++doubleRotationStep;
        relayout();
        if (doubleRotationStep == 1) {
            stat = doubleRotationCase == "LR"
                ? "Left rotation complete. Now rotate right at room 50."
                : "Right rotation complete. Now rotate left at room 50."; statC = OK_C;
        } else {
            std::vector<int> remaining;
            collectUnbalancedIds(tree.getRoot(), remaining);
            stat = remaining.empty() ? doubleRotationCase + " rotation complete. Tree balanced."
                                     : "Rotation sequence left an imbalance.";
            statC = remaining.empty() ? OK_C : ERR_C;
        }
    };

    auto predictInsertion = [&](const std::string& choice) {
        if (alarmFailed) return;
        chargeAlarm(choice == "LL" ? 1 : 2);
        if (alarmFailed) return;
        if (insertPredictComplete) {
            stat = "Insertion complete. Reset to try again."; statC = OK_C;
        } else if (choice != "LL") {
            scorePenalties += 50;
            stat = "Not this case. Follow the insertion path from room 30."; statC = ERR_C;
        } else {
            undoStack.push_back(serializeTree(tree.getRoot()));
            tree.insert(10);
            if (rotateRightAt(tree, 30)) {
                relayout();
                insertPredictComplete = true;
                stat = "Correct prediction. LL rotation applied automatically."; statC = OK_C;
            } else {
                undoStack.pop_back();
                stat = "Could not apply the predicted rotation."; statC = ERR_C;
            }
        }
    };

    auto doHeapAction = [&]() {
        if (alarmFailed) return;
        chargeAlarm();
        if (alarmFailed) return;
        const std::string& levelId = loader.acts[actIdx].levels[levelIdx].id;
        if (levelId == "4.1" || levelId == "4.2") {
            std::vector<int> input = heapState.heap;
            heapState.buildMaxHeap(input);
            refreshHeapView();
            heapTaskComplete = heapState.isMaxHeap();
            stat = heapTaskComplete ? "Max Heap property restored." : "Heap construction failed.";
            statC = heapTaskComplete ? OK_C : ERR_C;
        } else if (levelId == "4.3") {
            if (heapState.heap.empty()) {
                heapTaskComplete = true;
                stat = "All treasures extracted."; statC = OK_C;
            } else {
                extractedValues.push_back(heapState.extractMax());
                activeHeapSize = (int)heapState.heap.size();
                refreshHeapView();
                heapTaskComplete = heapState.heap.empty();
                stat = heapTaskComplete ? "All maxima extracted." : "Maximum extracted. Continue.";
                statC = OK_C;
            }
        } else if (activeHeapSize > 1) {
            std::swap(heapState.heap[0], heapState.heap[activeHeapSize - 1]);
            --activeHeapSize;
            heapState.siftDown(0, activeHeapSize);
            refreshHeapView();
            heapTaskComplete = activeHeapSize <= 1;
            stat = heapTaskComplete ? "Heap Sort complete. Array is sorted." : "Sort step complete. Continue.";
            statC = OK_C;
        }
    };

    auto heapArrayText = [&]() {
        std::string result = "Array: ";
        for (int i = 0; i < (int)heapState.heap.size(); ++i) {
            if (i) result += " ";
            if ((loader.acts[actIdx].levels[levelIdx].id == "4.4" ||
                 loader.acts[actIdx].levels[levelIdx].id == "4.5") && i == activeHeapSize)
                result += "| ";
            result += std::to_string(heapState.heap[i]);
        }
        if (loader.acts[actIdx].levels[levelIdx].id == "4.3" && !extractedValues.empty()) {
            result += "   Extracted: ";
            for (int value : extractedValues) result += std::to_string(value) + " ";
        }
        return result;
    };

    auto checkReconstructionRoot = [&]() {
        if (alarmFailed) return;
        chargeAlarm();
        if (alarmFailed) return;
        if (iL == 0 || preorderClue.empty()) {
            stat = "Enter a root value first."; statC = ERR_C;
            return;
        }
        int answer = std::stoi(insB);
        if (answer != preorderClue.front()) {
            stat = "Not the root. Use the traversal clues and try again."; statC = ERR_C;
            return;
        }

        BinaryTree rebuilt;
        rebuilt.root = rebuilt.buildFromPreIn(preorderClue, inorderClue);
        rebuilt.nodeCount = (int)preorderClue.size();
        if (rebuilt.preorder() == preorderClue && rebuilt.inorder() == inorderClue) {
            reconstructionComplete = true;
            stat = "Blueprint decoded. The tree is reconstructed."; statC = OK_C;
        } else {
            stat = "Those clues do not form a valid blueprint."; statC = ERR_C;
        }
    };

    auto checkHeightDiameterAnswer = [&]() {
        if (alarmFailed) return;
        chargeAlarm();
        if (alarmFailed) return;
        if (iL == 0) {
            stat = "Enter a number first."; statC = ERR_C;
            return;
        }
        if (heightDiameterStep >= 2) {
            stat = "Both measurements are correct. Reset to try again."; statC = OK_C;
            return;
        }
        int answer = std::stoi(insB);
        int expected = heightDiameterStep == 0 ? heightTarget : diameterTarget;
        if (answer != expected) {
            stat = heightDiameterStep == 0 ? "That height is not correct. Count edges from root to deepest leaf."
                                           : "That diameter is not correct. Find the longest path in edges.";
            statC = ERR_C;
            return;
        }
        ++heightDiameterStep;
        insB[0] = '\0'; iL = 0;
        if (heightDiameterStep == 2) {
            stat = "Height and diameter correct. Level complete."; statC = OK_C;
        } else {
            stat = "Correct height. Now enter the diameter in edges."; statC = OK_C;
        }
    };

    auto valuesToText = [](const std::vector<int>& values) {
        std::string result;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i) result += ", ";
            result += std::to_string(values[i]);
        }
        return result;
    };

    relayout();

    double lastStateSave = GetTime();
    while (!WindowShouldClose()) {
        alarmCapacity = std::max(1, (3 * optimalSteps() + 1) / 2 + 2);
        Vector2 m = GetMousePosition();
        auto toScreen = [&](const NodePosition& p) {
            return Vector2{canvas.x + canvas.width / 2.0f + (p.x - (canvas.x + canvas.width / 2.0f)) * cameraZoom + cameraPan.x,
                           canvas.y + canvas.height / 2.0f + (p.y - (canvas.y + canvas.height / 2.0f)) * cameraZoom + cameraPan.y};
        };
        bool inCanvas = CheckCollisionPointRec(m, canvas);
        float wheelMove = GetMouseWheelMove();
        if (inCanvas && wheelMove != 0.0f)
            cameraZoom = std::max(0.5f, std::min(2.5f, cameraZoom + wheelMove * 0.1f));
        TreeNode* hoveredNode = nullptr;
        if (inCanvas) {
            for (const auto& entry : layout.positions) {
                Vector2 p = toScreen(entry.second);
                float radius = entry.second.radius * cameraZoom;
                float dx = m.x - p.x, dy = m.y - p.y;
                if (dx * dx + dy * dy <= radius * radius) {
                    hoveredNode = findNodeById(tree.getRoot(), entry.first);
                    break;
                }
            }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inCanvas && !hoveredNode) panning = true;
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) panning = false;
        if (panning && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            cameraPan.x += m.x - previousMouse.x;
            cameraPan.y += m.y - previousMouse.y;
        }
        previousMouse = m;
        auto drawBtn = [&](Rectangle r, const std::string& t) {
            bool h = CheckCollisionPointRec(m, r);
            bool p = h && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            DrawRectangleRec(r, p ? GRAY : (h ? LIGHTGRAY : RAYWHITE));
            DrawRectangleLinesEx(r, 1.0f, DARKGRAY);
            DrawText(t.c_str(), (int)r.x + (int)(r.width - MeasureText(t.c_str(), 16))/2, (int)r.y + 7, 16, DARKGRAY);
            return p;
        };

        // ---- Row 1 geometry (needed for click-to-focus before drawing) ----
        float x = 30, y = (float)row1Y;
        float insLabelX = x;            x += 70;
        Rectangle iR  = { x, y, 60, 30 }; x += 70;
        Rectangle iBt = { x, y, 70, 30 }; x += 90;
        float delLabelX = x;            x += 70;
        Rectangle dR  = { x, y, 60, 30 }; x += 70;
        Rectangle dBt = { x, y, 70, 30 }; x += 100;

        // Level Navigation Buttons
        Rectangle pActBt  = { x, y, 80, 30 }; x += 90;
        Rectangle nActBt  = { x, y, 80, 30 }; x += 90;
        Rectangle pLvlBt  = { x, y, 80, 30 }; x += 90;
        Rectangle nLvlBt  = { x, y, 80, 30 };
        Rectangle rootAnswerRect = { 520, (float)row2Y, 60, 30 };
        Rectangle measureAnswerRect = { 100, (float)row2Y, 60, 30 };

        // ---- Input handling ----
        if (!alarmFailed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isGuidedLevel() &&
            CheckCollisionPointRec(m, { (float)canvasX, (float)canvasY, (float)canvasW, (float)canvasH })) {
            for (const auto& entry : layout.positions) {
                const NodePosition& pos = entry.second;
                Vector2 screenPos = toScreen(pos);
                float hitRadius = pos.radius * cameraZoom;
                float dx = m.x - screenPos.x, dy = m.y - screenPos.y;
                if (dx * dx + dy * dy <= hitRadius * hitRadius) {
                    chargeAlarm();
                    if (alarmFailed) break;
                    if (isOrderStatisticLevel()) {
                        if (entry.first == orderStatisticTargetId) {
                            orderStatisticComplete = true;
                            stat = "Correct. The k-th smallest key is " + std::to_string(orderStatisticTarget) + ".";
                            statC = OK_C;
                        } else {
                            stat = "That room is not the requested order statistic."; statC = ERR_C;
                        }
                    } else if (isBalanceLevel()) {
                        if (std::find(unbalancedTargets.begin(), unbalancedTargets.end(), entry.first) ==
                            unbalancedTargets.end()) {
                            stat = "This room is balanced. Check its subtree heights."; statC = ERR_C;
                        } else if (std::find(unbalancedSelected.begin(), unbalancedSelected.end(), entry.first) !=
                                   unbalancedSelected.end()) {
                            stat = "Room already selected."; statC = OK_C;
                        } else {
                            unbalancedSelected.push_back(entry.first);
                            stat = unbalancedSelected.size() == unbalancedTargets.size()
                                       ? "All unbalanced rooms found. Level complete."
                                       : "Violation found. Check the remaining rooms.";
                            statC = OK_C;
                        }
                    } else if (isRangeLevel()) {
                        if (rangeVisited.size() < rangeExpected.size() &&
                            entry.first == rangeExpected[rangeVisited.size()]) {
                            rangeVisited.push_back(entry.first);
                            stat = rangeVisited.size() == rangeExpected.size()
                                       ? "Range query complete. All matching keys found."
                                       : "Matching key found. Continue in sorted order.";
                            statC = OK_C;
                        } else if (rangeVisited.size() < rangeExpected.size()) {
                            stat = "That key is outside the range or out of order."; statC = ERR_C;
                        } else {
                            stat = "Range query already complete. Reset to retry."; statC = OK_C;
                        }
                    } else if (isMirrorLevel()) {
                        auto flipped = std::find(mirrorFlipped.begin(), mirrorFlipped.end(), entry.first);
                        if (flipped == mirrorFlipped.end()) {
                            if (std::find(mirrorTargets.begin(), mirrorTargets.end(), entry.first) == mirrorTargets.end()) {
                                stat = "This room has no branches to mirror."; statC = ERR_C;
                            } else {
                                std::swap(findNodeById(tree.getRoot(), entry.first)->left,
                                          findNodeById(tree.getRoot(), entry.first)->right);
                                mirrorFlipped.push_back(entry.first);
                                stat = mirrorFlipped.size() == mirrorTargets.size()
                                           ? "Vault mirrored. Level complete."
                                           : "Branch mirrored. Continue with the remaining rooms.";
                                statC = OK_C;
                            }
                        } else {
                            std::swap(findNodeById(tree.getRoot(), entry.first)->left,
                                      findNodeById(tree.getRoot(), entry.first)->right);
                            mirrorFlipped.erase(flipped);
                            stat = "Branch restored. Mirror it again to include it."; statC = OK_C;
                        }
                        layout.calculateLayout(tree.getRoot(), canvas, 50, 80, 80.0f);
                    } else if (isLcaLevel()) {
                        if (lcaComplete) {
                            stat = "LCA already found. Reset to try again."; statC = OK_C;
                        } else if (lcaSelected.size() < 2) {
                            lcaSelected.push_back(entry.first);
                            stat = lcaSelected.size() == 1 ? "First room selected. Choose a second room."
                                                           : "Second room selected. Now choose their lowest common ancestor.";
                            if (lcaSelected.size() == 2) {
                                TreeNode* ancestor = tree.lca(lcaSelected[0], lcaSelected[1]);
                                lcaExpectedId = ancestor ? ancestor->id : -1;
                            }
                            statC = OK_C;
                        } else if (entry.first == lcaExpectedId) {
                            lcaComplete = true;
                            stat = "Correct meeting room. LCA challenge complete."; statC = OK_C;
                        } else {
                            stat = "That is not the lowest common ancestor. Try again."; statC = ERR_C;
                        }
                    } else {
                        std::vector<int>& expected = isPatrolLevel() ? patrolOrder : searchPath;
                        std::vector<int>& visited = isPatrolLevel() ? patrolVisited : searchVisited;
                        if (visited.size() < expected.size() && entry.first == expected[visited.size()]) {
                            visited.push_back(entry.first);
                            if (visited.size() == expected.size()) {
                            stat = isPatrolLevel() ? "Patrol complete! Preorder mastered."
                                                   : (isDegenerationLevel()
                                                          ? "Treasure found after a long chain of comparisons."
                                                          : "Treasure found! BST search complete.");
                                statC = OK_C;
                            } else {
                                stat = isPatrolLevel() ? "Correct room. Continue the patrol."
                                                       : "Correct comparison. Continue the search.";
                                statC = OK_C;
                            }
                        } else if (visited.size() < expected.size()) {
                            stat = isPatrolLevel() ? "Wrong room. Preorder visits root, then left, then right."
                                                   : "Wrong room. Compare the key and choose the correct branch.";
                            statC = ERR_C;
                        } else {
                            stat = "Level already complete. Reset to try again."; statC = OK_C;
                        }
                    }
                    break;
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isGuidedLevel() && !isHeapLevel()) {
            if (CheckCollisionPointRec(m, iR)) active = 0;
            else if (CheckCollisionPointRec(m, dR)) active = 1;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isReconstructionLevel() &&
            CheckCollisionPointRec(m, rootAnswerRect)) active = 0;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHeightDiameterLevel() &&
            CheckCollisionPointRec(m, measureAnswerRect)) active = 0;
        if (IsKeyPressed(KEY_TAB) && !isGuidedLevel() && !isHeapLevel()) active = 1 - active;

        int k = (isGuidedLevel() || isHeapLevel()) ? 0 : GetCharPressed();
        while (k > 0) {
            if (active == 0 && iL < 5 && k >= '0' && k <= '9') { insB[iL++] = (char)k; insB[iL] = '\0'; }
            else if (active == 1 && dL < 5 && k >= '0' && k <= '9') { delB[dL++] = (char)k; delB[dL] = '\0'; }
            k = GetCharPressed();
        }
        if (!isGuidedLevel() && !isHeapLevel() && IsKeyPressed(KEY_BACKSPACE)) {
            if (active == 0 && iL > 0) insB[--iL] = '\0';
            else if (active == 1 && dL > 0) delB[--dL] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER) && !alarmFailed) {
            if (isReconstructionLevel()) checkReconstructionRoot();
            else if (isHeightDiameterLevel()) checkHeightDiameterAnswer();
            else if (isHeapLevel()) doHeapAction();
            else if (!isGuidedLevel()) doOp(active == 0);
        }
        if (IsKeyPressed(KEY_H) && !alarmFailed && !isCurrentLevelComplete()) {
            starCap = std::min(starCap, 2);
            const std::string& id = loader.acts[actIdx].levels[levelIdx].id;
            std::string hint = id == "1.1" ? "Hint: preorder visits root, then left subtree, then right subtree."
                : (id == "1.2" ? "Hint: the first preorder value is the root."
                : (id == "2.1" || id == "2.5" ? "Hint: compare with the current key and follow the BST ordering."
                : (id == "3.2" ? "Hint: LL uses a right rotation; RR uses a left rotation."
                : (id == "3.3" ? "Hint: the inner child rotates first, then the unbalanced root."
                : (id[0] == '4' ? "Hint: keep the largest value at the heap root." : "Hint: follow the level objective one step at a time.")))));
            stat = hint;
            statC = OK_C;
        }
        if (IsKeyPressed(KEY_Z)) doUndo();
        if (isCurrentLevelComplete()) {
            const std::string& completedId = loader.acts[actIdx].levels[levelIdx].id;
            int score = std::max(0, 1000 - 10 * alarmSteps - scorePenalties);
            starsEarned = std::min(starCap, alarmSteps <= optimalSteps() && scorePenalties == 0 ? 3
                        : alarmSteps <= (5 * optimalSteps()) / 4 && scorePenalties <= 1 ? 2 : 1);
            bool newCompletion = completedLevels.insert(completedId).second;
            auto best = bestScores.find(completedId);
            bool scoreImproved = newCompletion || best == bestScores.end() || score > best->second;
            bool starsImproved = starsEarned > bestStars[completedId];
            if (scoreImproved || starsImproved) {
                if (scoreImproved) bestScores[completedId] = score;
                if (starsImproved) bestStars[completedId] = starsEarned;
                saveCampaignProgress();
            }
        }
        if (GetTime() - lastStateSave >= 1.0) {
            saveActiveState();
            lastStateSave = GetTime();
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            // Title bar
            DrawRectangle(0, 0, WIN_W, titleH, Color{0, 60, 130, 255});
            std::string titleText = "VAULT HEIST";
            if (levelsLoaded && !loader.acts.empty()) {
                titleText += " - Act " + std::to_string(loader.acts[actIdx].act) + ": " + loader.acts[actIdx].title;
                titleText += " | Level " + loader.acts[actIdx].levels[levelIdx].id + ": " + loader.acts[actIdx].levels[levelIdx].name;
            }
            DrawText(titleText.c_str(), 20, 15, 20, RAYWHITE);
            std::string campaignText = "Complete: " + std::to_string(completedLevels.size()) +
                                       " / " + std::to_string(totalLevels);
            DrawText(campaignText.c_str(), WIN_W - 160, 17, 16, RAYWHITE);

            // Toolbar panel
            DrawRectangle(0, titleH, WIN_W, panelBot - titleH, LIGHTGRAY);

            // Row 1: Patrol instructions or Insert / Delete + Level Nav
            if (isReconstructionLevel()) {
                std::string clue = "Preorder: " + valuesToText(preorderClue);
                DrawText(clue.c_str(), 30, (int)y + 7, 18, DARKGRAY);
            } else if (isHeightDiameterLevel()) {
                DrawText("Measure the visible tree: height and diameter (edges).",
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isLcaLevel()) {
                DrawText("Select two rooms, then find their LCA.",
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isRangeLevel()) {
                DrawText(("Range Raid: click keys from " + std::to_string(rangeLow) + " to " +
                          std::to_string(rangeHigh) + " in order").c_str(),
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isBalanceLevel()) {
                DrawText("Click rooms whose balance factor is outside -1, 0, 1.",
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isSingleRotateLevel()) {
                const char* prompt = singleRotationCase == "LL"
                    ? "LL imbalance at room 30. Apply a right rotation."
                    : "RR imbalance at room 70. Apply a left rotation.";
                DrawText(prompt,
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isDoubleRotateLevel()) {
                const char* prompt = doubleRotationCase == "LR"
                    ? "LR case: rotate left at 20, then right at 50."
                    : "RL case: rotate right at 70, then left at 50.";
                DrawText(prompt,
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isInsertPredictLevel()) {
                DrawText("Insert 10 into this tree. Predict the rotation case.",
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isHeapLevel()) {
                const std::string& levelId = loader.acts[actIdx].levels[levelIdx].id;
                const char* prompt = levelId == "4.1" ? "Build a Max Heap from the array."
                                   : levelId == "4.2" ? "Repair this array with bottom-up heapify."
                                   : levelId == "4.3" ? "Extract the maximum values one at a time."
                                   : "Sort the heap one extraction step at a time.";
                DrawText(prompt, 30, (int)y + 7, 18, DARKGRAY);
            } else if (isOrderStatisticLevel()) {
                DrawText(("Select the " + std::to_string(orderStatisticK) + "th smallest key").c_str(),
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isDegenerationLevel()) {
                DrawText(("Degeneration: find " + std::to_string(searchTarget) + " in the right-leaning chain").c_str(),
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isMirrorLevel()) {
                DrawText("Mirror every room that has a branch by clicking it.",
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isPatrolLevel()) {
                DrawText("Preorder: click Root, Left, Right", 30, (int)y + 7, 18, DARKGRAY);
            } else if (isSearchLevel()) {
                DrawText(("BST search: find " + std::to_string(searchTarget) + " by comparing each room").c_str(),
                         30, (int)y + 7, 18, DARKGRAY);
            } else if (isInsertLevel()) {
                DrawText("Insert:", (int)insLabelX, (int)y + 7, 18, DARKGRAY);
                DrawRectangleRec(iR, RAYWHITE);
                DrawRectangleLinesEx(iR, active == 0 ? 2.0f : 1.0f, active == 0 ? BLUE : GRAY);
                DrawText(insB, (int)iR.x + 5, (int)iR.y + 7, 18, BLACK);
                if (active == 0 && ((int)(GetTime() * 2)) % 2 == 0)
                    DrawText("|", (int)iR.x + 7 + MeasureText(insB, 18), (int)iR.y + 7, 18, BLACK);
                if (drawBtn(iBt, "Insert")) doOp(true);
                DrawText(("Target: " + std::to_string(insertTarget)).c_str(),
                         (int)delLabelX, (int)y + 7, 18, DARKGRAY);
            } else if (isDeleteLevel()) {
                DrawText(("Target " + std::to_string(deleteTarget) + ":").c_str(),
                         (int)delLabelX, (int)y + 7, 18, DARKGRAY);
                DrawRectangleRec(dR, RAYWHITE);
                DrawRectangleLinesEx(dR, active == 1 ? 2.0f : 1.0f, active == 1 ? BLUE : GRAY);
                DrawText(delB, (int)dR.x + 5, (int)dR.y + 7, 18, BLACK);
                if (active == 1 && ((int)(GetTime() * 2)) % 2 == 0)
                    DrawText("|", (int)dR.x + 7 + MeasureText(delB, 18), (int)dR.y + 7, 18, BLACK);
                if (drawBtn(dBt, "Delete")) doOp(false);
            } else {
            DrawText("Insert:", (int)insLabelX, (int)y + 7, 18, DARKGRAY);
            DrawRectangleRec(iR, RAYWHITE);
            DrawRectangleLinesEx(iR, active == 0 ? 2.0f : 1.0f, active == 0 ? BLUE : GRAY);
            DrawText(insB, (int)iR.x + 5, (int)iR.y + 7, 18, BLACK);
            if (active == 0 && ((int)(GetTime() * 2)) % 2 == 0)
                DrawText("|", (int)iR.x + 7 + MeasureText(insB, 18), (int)iR.y + 7, 18, BLACK);
            if (drawBtn(iBt, "Insert")) doOp(true);

            DrawText("Delete:", (int)delLabelX, (int)y + 7, 18, DARKGRAY);
            DrawRectangleRec(dR, RAYWHITE);
            DrawRectangleLinesEx(dR, active == 1 ? 2.0f : 1.0f, active == 1 ? BLUE : GRAY);
            DrawText(delB, (int)dR.x + 5, (int)dR.y + 7, 18, BLACK);
            if (active == 1 && ((int)(GetTime() * 2)) % 2 == 0)
                DrawText("|", (int)dR.x + 7 + MeasureText(delB, 18), (int)dR.y + 7, 18, BLACK);
            if (drawBtn(dBt, "Delete")) doOp(false);
            }

            if (drawBtn(pActBt, "Prev Act")) {
                if (actIdx > 0) { actIdx--; levelIdx = 0; resetToLevel(); undoStack.clear(); stat = "Switched to Act " + std::to_string(loader.acts[actIdx].act); statC = OK_C; relayout(); }
            }
            if (drawBtn(nActBt, "Next Act")) {
                if (actIdx < (int)loader.acts.size() - 1) { actIdx++; levelIdx = 0; resetToLevel(); undoStack.clear(); stat = "Switched to Act " + std::to_string(loader.acts[actIdx].act); statC = OK_C; relayout(); }
            }
            if (drawBtn(pLvlBt, "Prev Lvl")) {
                if (levelIdx > 0) { levelIdx--; resetToLevel(); undoStack.clear(); stat = "Switched to Level " + loader.acts[actIdx].levels[levelIdx].id; statC = OK_C; relayout(); }
            }
            if (drawBtn(nLvlBt, "Next Lvl")) {
                if (levelIdx < (int)loader.acts[actIdx].levels.size() - 1) { levelIdx++; resetToLevel(); undoStack.clear(); stat = "Switched to Level " + loader.acts[actIdx].levels[levelIdx].id; statC = OK_C; relayout(); }
            }

            // Row 2: Undo / Reset / counter (left aligned)
            float bx = 30;
            if (isHeapLevel()) {
                const std::string& levelId = loader.acts[actIdx].levels[levelIdx].id;
                const char* action = levelId == "4.1" ? "Build Heap"
                                   : levelId == "4.2" ? "Heapify"
                                   : levelId == "4.3" ? "Extract Max"
                                   : "Sort Step";
                if (drawBtn({ 30, (float)row2Y, 130, 30 }, action)) doHeapAction();
                if (drawBtn({ 170, (float)row2Y, 70, 30 }, "Reset")) doReset();
                DrawText(heapTaskComplete ? "Complete" : "Use the action to continue",
                         260, row2Y + 7, 16, DARKGRAY);
            } else if (isInsertPredictLevel()) {
                if (drawBtn({ 30, (float)row2Y, 80, 30 }, "Undo [Z]")) doUndo();
                if (drawBtn({ 122, (float)row2Y, 55, 30 }, "LL")) predictInsertion("LL");
                if (drawBtn({ 184, (float)row2Y, 55, 30 }, "RR")) predictInsertion("RR");
                if (drawBtn({ 246, (float)row2Y, 55, 30 }, "LR")) predictInsertion("LR");
                if (drawBtn({ 308, (float)row2Y, 55, 30 }, "RL")) predictInsertion("RL");
                if (drawBtn({ 370, (float)row2Y, 70, 30 }, "Reset")) doReset();
            } else if (isDoubleRotateLevel()) {
                if (drawBtn({ 30, (float)row2Y, 80, 30 }, "Undo [Z]")) doUndo();
                if (drawBtn({ 122, (float)row2Y, 150, 30 },
                            doubleRotationStep == 0
                                ? (doubleRotationCase == "LR" ? "Rotate Left at 20" : "Rotate Right at 70")
                                : (doubleRotationCase == "LR" ? "Rotate Right at 50" : "Rotate Left at 50")))
                    doDoubleRotation();
                if (drawBtn({ 284, (float)row2Y, 70, 30 }, "Reset")) doReset();
                if (drawBtn({ 370, (float)row2Y, 48, 30 }, "LR") && doubleRotationCase != "LR") {
                    doubleRotationCase = "LR"; resetToLevel(); undoStack.clear(); relayout();
                }
                if (drawBtn({ 424, (float)row2Y, 48, 30 }, "RL") && doubleRotationCase != "RL") {
                    doubleRotationCase = "RL"; resetToLevel(); undoStack.clear(); relayout();
                }
                DrawText((std::to_string(doubleRotationStep) + " / 2").c_str(), 486, row2Y + 7, 16, DARKGRAY);
            } else if (isSingleRotateLevel()) {
                if (drawBtn({ 30, (float)row2Y, 80, 30 }, "Undo [Z]")) doUndo();
                if (drawBtn({ 122, (float)row2Y, 150, 30 },
                            singleRotationCase == "LL" ? "Rotate Right" : "Rotate Left")) doSingleRotation();
                if (drawBtn({ 284, (float)row2Y, 70, 30 }, "Reset")) doReset();
                if (drawBtn({ 370, (float)row2Y, 48, 30 }, "LL") && singleRotationCase != "LL") {
                    singleRotationCase = "LL"; resetToLevel(); undoStack.clear(); relayout();
                }
                if (drawBtn({ 424, (float)row2Y, 48, 30 }, "RR") && singleRotationCase != "RR") {
                    singleRotationCase = "RR"; resetToLevel(); undoStack.clear(); relayout();
                }
                DrawText(singleRotationComplete ? "Balanced" : singleRotationCase.c_str(),
                         486, row2Y + 7, 16, DARKGRAY);
            } else if (isHeightDiameterLevel()) {
                DrawText(heightDiameterStep == 0 ? "Height:" : "Diameter:",
                         30, row2Y + 7, 18, DARKGRAY);
                DrawRectangleRec(measureAnswerRect, RAYWHITE);
                DrawRectangleLinesEx(measureAnswerRect, active == 0 ? 2.0f : 1.0f,
                                     active == 0 ? BLUE : GRAY);
                DrawText(insB, (int)measureAnswerRect.x + 5, (int)measureAnswerRect.y + 7, 18, BLACK);
                if (active == 0 && ((int)(GetTime() * 2)) % 2 == 0)
                    DrawText("|", (int)measureAnswerRect.x + 7 + MeasureText(insB, 18),
                             (int)measureAnswerRect.y + 7, 18, BLACK);
                if (drawBtn({ 170, (float)row2Y, 70, 30 }, "Check")) checkHeightDiameterAnswer();
                if (drawBtn({ 250, (float)row2Y, 70, 30 }, "Reset")) doReset();
                DrawText((std::to_string(heightDiameterStep) + " / 2").c_str(), 340, row2Y + 7, 16, DARKGRAY);
            } else if (isReconstructionLevel()) {
                std::string clue = "Inorder: " + valuesToText(inorderClue);
                DrawText(clue.c_str(), 30, row2Y + 7, 18, DARKGRAY);
                DrawText("Root:", 460, row2Y + 7, 18, DARKGRAY);
                DrawRectangleRec(rootAnswerRect, RAYWHITE);
                DrawRectangleLinesEx(rootAnswerRect, active == 0 ? 2.0f : 1.0f,
                                     active == 0 ? BLUE : GRAY);
                DrawText(insB, (int)rootAnswerRect.x + 5, (int)rootAnswerRect.y + 7, 18, BLACK);
                if (active == 0 && ((int)(GetTime() * 2)) % 2 == 0)
                    DrawText("|", (int)rootAnswerRect.x + 7 + MeasureText(insB, 18),
                             (int)rootAnswerRect.y + 7, 18, BLACK);
                if (drawBtn({ 590, (float)row2Y, 70, 30 }, "Check")) checkReconstructionRoot();
                if (drawBtn({ 670, (float)row2Y, 70, 30 }, "Reset")) doReset();
            } else {
                if (drawBtn({ bx, (float)row2Y, 80, 30 }, "Undo [Z]")) doUndo();
                bx += 80 + 12;
                if (drawBtn({ bx, (float)row2Y, 70, 30 }, "Reset")) doReset();
                bx += 70 + 12;
                DrawText((isGuidedLevel()
                              ? (isOrderStatisticLevel()
                                     ? (orderStatisticComplete ? "Found: " + std::to_string(orderStatisticTarget)
                                                               : "Find the " + std::to_string(orderStatisticK) + "th smallest")
                                     : (isBalanceLevel()
                                     ? "Unbalanced rooms: " + std::to_string(unbalancedSelected.size()) + " / " +
                                           std::to_string(unbalancedTargets.size())
                                     : (isRangeLevel()
                                     ? "Range keys: " + std::to_string(rangeVisited.size()) + " / " +
                                           std::to_string(rangeExpected.size())
                                     : (isMirrorLevel()
                                     ? "Branches mirrored: " + std::to_string(mirrorFlipped.size()) + " / " +
                                           std::to_string(mirrorTargets.size())
                                     : (isLcaLevel()
                                            ? "Rooms selected: " + std::to_string(lcaSelected.size()) + " / 2"
                                            : "Steps: " + std::to_string(isPatrolLevel() ? patrolVisited.size() : searchVisited.size()) +
                                                  " / " + std::to_string(isPatrolLevel() ? patrolOrder.size() : searchPath.size()))))))
                              : "Undo steps: " + std::to_string(undoStack.size())).c_str(),
                         (int)bx, row2Y + 7, 16, DARKGRAY);
            }
            if (statC.r == ERR_C.r && statC.g == ERR_C.g &&
                statC.b == ERR_C.b && statC.a == ERR_C.a) {
                if (stat != lastCountedError) {
                    ++mistakesThisLevel;
                    lastCountedError = stat;
                }
            } else {
                lastCountedError.clear();
            }
            if (levelsLoaded && !loader.acts.empty()) {
                std::string currentLevelId = loader.acts[actIdx].levels[levelIdx].id;
                auto currentBest = bestScores.find(currentLevelId);
                int shownScore = currentBest == bestScores.end()
                                     ? std::max(0, 1000 - 10 * alarmSteps - scorePenalties)
                                     : currentBest->second;
                int shownStars = isCurrentLevelComplete()
                    ? starsEarned
                    : (bestStars.count(currentLevelId) ? bestStars[currentLevelId] : 0);
                DrawText(("Alarm: " + std::to_string(alarmSteps) + "/" + std::to_string(alarmCapacity) +
                          "  Score: " + std::to_string(shownScore) + "  Stars: " +
                          std::to_string(shownStars) + "/3").c_str(), WIN_W - 400, row2Y + 7, 16, DARKGRAY);
            }

            // Row 3: instructions, Row 4: status
            std::string instructions = "Instructions: Click box or press [TAB] to switch. [ENTER] executes. [Z] Undos.";
            if (isPatrolLevel()) instructions = "Click rooms in root-left-right order. [Z] undoes. Reset restarts the patrol.";
            else if (isSearchLevel()) instructions = "Click each room along the BST search path. [Z] undoes. Reset restarts the search.";
            else if (isLcaLevel()) instructions = "Choose any two rooms, then find the deepest node shared by both root paths.";
            else if (isMirrorLevel()) instructions = "Each click swaps a room's children. Undo reverses the last swap.";
            else if (isRangeLevel()) instructions = "Click every key in the range from smallest to largest.";
            else if (isDegenerationLevel()) instructions = "Follow each comparison to the end of this unbalanced BST.";
            else if (isBalanceLevel()) instructions = "A room is unbalanced when the subtree height difference is greater than one.";
            else if (isSingleRotateLevel()) instructions = "The heavy path goes left twice, so repair it with one right rotation.";
            else if (isDoubleRotateLevel()) instructions = "The heavy path goes left then right, so apply both rotations in order.";
            else if (isInsertPredictLevel()) instructions = "Choose LL, RR, LR, or RL. Correct predictions trigger an automatic repair.";
            else if (isOrderStatisticLevel()) instructions = "The k-th smallest key is at position k in the sorted order.";
            else if (isHeapLevel()) instructions = "The array and complete-tree view show the same heap. Click the action button to advance.";
            else if (isHeightDiameterLevel()) instructions = "Height is root to deepest leaf in edges; diameter is the longest path in edges.";
            else if (isReconstructionLevel()) instructions = "Use preorder and inorder to identify the root, then check your answer.";
            else if (isInsertLevel()) instructions = "Type the target key and press [ENTER] or click Insert. [Z] undoes.";
            else if (isDeleteLevel()) instructions = "Delete the node with two children to watch the inorder successor take its place.";
            instructions += " [H] Hint. Wheel zooms; drag empty canvas to pan.";
            DrawText(instructions.c_str(), 30, instrY, 16, GRAY);
            DrawText(stat.c_str(), 30, statusY, 18, statC);

            // Canvas
            BeginScissorMode(canvasX, canvasY, canvasW, canvasH);
                if (isHeapLevel()) {
                    std::string arrayText = heapArrayText();
                    DrawText(arrayText.c_str(), canvasX + 15, canvasY + 10, 18, DARKGRAY);
                }
                std::unordered_map<int, Color> highlights;
                if (isGuidedLevel()) {
                    if (isOrderStatisticLevel()) {
                        if (orderStatisticComplete) highlights[orderStatisticTargetId] = BLUE;
                    } else if (isBalanceLevel()) {
                        for (int id : unbalancedSelected) highlights[id] = BLUE;
                    } else if (isRangeLevel()) {
                        for (int id : rangeVisited) highlights[id] = BLUE;
                    } else if (isMirrorLevel()) {
                        for (int id : mirrorFlipped) highlights[id] = BLUE;
                    } else if (isLcaLevel()) {
                        if (!lcaSelected.empty()) highlights[lcaSelected[0]] = BLUE;
                        if (lcaSelected.size() > 1) highlights[lcaSelected[1]] = GOLD;
                        if (lcaComplete) highlights[lcaExpectedId] = GREEN;
                    } else {
                        const std::vector<int>& visited = isPatrolLevel() ? patrolVisited : searchVisited;
                        for (int id : visited) highlights[id] = BLUE;
                    }
                }
                if (isDeleteLevel() && !deleteCompleted) {
                    BSTNode* target = tree.search(deleteTarget);
                    if (target) highlights[target->id] = GOLD;
                }
                if (isSingleRotateLevel() && !singleRotationComplete) {
                    BSTNode* pivot = tree.search(30);
                    if (pivot) highlights[pivot->id] = GOLD;
                }
                if (isDoubleRotateLevel() && doubleRotationStep < 2) {
                    BSTNode* pivot = tree.search(50);
                    if (pivot) highlights[pivot->id] = GOLD;
                }
                if (isInsertPredictLevel() && !insertPredictComplete) {
                    BSTNode* pivot = tree.search(30);
                    if (pivot) highlights[pivot->id] = GOLD;
                }
                if (!isReconstructionLevel() || reconstructionComplete) {
                    renderer.drawTree(tree.getRoot(), layout, highlights, canvas, cameraZoom, cameraPan);
                } else {
                    DrawText("Reconstruct the root from the two traversal clues above.",
                             canvasX + 35, canvasY + 30, 20, DARKGRAY);
                }
            EndScissorMode();
            if (isCurrentLevelComplete() || alarmFailed) {
                int panelW = 390, panelH = 170;
                int panelX = canvasX + (canvasW - panelW) / 2;
                int panelY = canvasY + (canvasH - panelH) / 2;
                DrawRectangle(panelX, panelY, panelW, panelH, Color{20, 35, 55, 245});
                DrawRectangleLines(panelX, panelY, panelW, panelH, GOLD);
                const char* resultTitle = alarmFailed ? "ALARM TRIGGERED" : "LEVEL COMPLETE";
                DrawText(resultTitle, panelX + 25, panelY + 20, 26, alarmFailed ? RED : GREEN);
                if (alarmFailed) {
                    DrawText("Reset the level to try again.", panelX + 25, panelY + 65, 18, RAYWHITE);
                } else {
                    std::string stars = "Stars: " + std::string(starsEarned, '*') +
                                        std::string(3 - starsEarned, '-');
                    DrawText(stars.c_str(), panelX + 25, panelY + 61, 22, GOLD);
                    DrawText(("Alarm steps: " + std::to_string(alarmSteps) +
                              " / " + std::to_string(alarmCapacity)).c_str(),
                             panelX + 25, panelY + 92, 17, RAYWHITE);
                    DrawText(("Score: " + std::to_string(std::max(0, 1000 - 10 * alarmSteps - scorePenalties)) +
                              "    Optimal: " + std::to_string(optimalSteps())).c_str(),
                             panelX + 25, panelY + 119, 17, RAYWHITE);
                }
                DrawText("Use Reset to retry or Next Lvl to continue.", panelX + 25, panelY + 145, 15, LIGHTGRAY);
            }
            if (hoveredNode) {
                int nodeHeight = subtreeHeight(hoveredNode);
                int balance = subtreeHeight(hoveredNode->left) - subtreeHeight(hoveredNode->right);
                std::string tip = "Key " + std::to_string(hoveredNode->value) +
                    " | depth " + std::to_string(depthOfNode(tree.getRoot(), hoveredNode->id)) +
                    " | height " + std::to_string(nodeHeight) +
                    " | balance " + std::to_string(balance);
                int tipW = MeasureText(tip.c_str(), 15) + 14;
                int tipX = std::min((int)m.x + 14, WIN_W - tipW - 8);
                int tipY = std::min((int)m.y + 14, WIN_H - 34);
                DrawRectangle(tipX, tipY, tipW, 26, Color{30, 30, 30, 235});
                DrawText(tip.c_str(), tipX + 7, tipY + 5, 15, RAYWHITE);
            }
            DrawRectangleLines(canvasX, canvasY, canvasW, canvasH, LIGHTGRAY);
        EndDrawing();
    }
    saveActiveState();
    CloseWindow();
    return 0;
}
