#include "raylib.h"
#include "../models/BST.h"
#include "../models/AVLTree.h"
#include "../models/MaxHeap.h"
#include "LayoutEngine.h"
#include "Renderer.h"
#include "../level/LevelLoader.h"
#include "../models/Validator.h"
#include "../models/ScoreSystem.h"
#include "../level/LevelManager.h"
#include "EventLog.h"
#include "AnimationController.h"
#include "SaveManager.h"
#include "UIManager.h"
#include "Theme.h"
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

static void collectPostorderKeys(TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    collectPostorderKeys(n->left, out);
    collectPostorderKeys(n->right, out);
    out.push_back(n->value);
}

static void collectPreorderValues(TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    out.push_back(n->value);
    collectPreorderValues(n->left, out);
    collectPreorderValues(n->right, out);
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
static void collectPostorderIds(TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    collectPostorderIds(n->left, out);
    collectPostorderIds(n->right, out);
    out.push_back(n->id);
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

static void collectAllIds(TreeNode* n, std::vector<int>& out) {
    if (!n) return;
    out.push_back(n->id);
    collectAllIds(n->left, out);
    collectAllIds(n->right, out);
}

static void collectRangeSkippedIds(TreeNode* n, int low, int high, std::vector<int>& out) {
    if (!n) return;
    int key = ((BSTNode*)n)->key;
    if (key < low) {
        collectAllIds(n->left, out);
        collectRangeSkippedIds(n->right, low, high, out);
    } else if (key > high) {
        collectAllIds(n->right, out);
        collectRangeSkippedIds(n->left, low, high, out);
    } else {
        collectRangeSkippedIds(n->left, low, high, out);
        collectRangeSkippedIds(n->right, low, high, out);
    }
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

static TreeNode* findTreeNodeByValue(TreeNode* root, int value) {
    if (!root) return nullptr;
    std::vector<TreeNode*> pending{root};
    for (size_t i = 0; i < pending.size(); ++i) {
        TreeNode* node = pending[i];
        if (node->value == value) return node;
        if (node->left) pending.push_back(node->left);
        if (node->right) pending.push_back(node->right);
    }
    return nullptr;
}

static bool insertCompleteBinary(BinaryTree& tree, int value, int& nextId) {
    TreeNode* added = new TreeNode(nextId++, value);
    if (!tree.root) {
        tree.root = added;
    } else {
        std::vector<TreeNode*> pending{tree.root};
        bool inserted = false;
        for (size_t i = 0; i < pending.size() && !inserted; ++i) {
            TreeNode* node = pending[i];
            if (!node->left) {
                node->left = added;
                added->parent = node;
                inserted = true;
            } else if (!node->right) {
                node->right = added;
                added->parent = node;
                inserted = true;
            } else {
                pending.push_back(node->left);
                pending.push_back(node->right);
            }
        }
    }
    ++tree.nodeCount;
    return true;
}

static bool removeCompleteBinary(BinaryTree& tree, int value) {
    if (!tree.root) return false;
    std::vector<TreeNode*> pending{tree.root};
    TreeNode* target = nullptr;
    TreeNode* last = nullptr;
    for (size_t i = 0; i < pending.size(); ++i) {
        TreeNode* node = pending[i];
        if (!target && node->value == value) target = node;
        last = node;
        if (node->left) pending.push_back(node->left);
        if (node->right) pending.push_back(node->right);
    }
    if (!target || !last) return false;
    target->value = last->value;
    if (!last->parent) tree.root = nullptr;
    else if (last->parent->left == last) last->parent->left = nullptr;
    else last->parent->right = nullptr;
    delete last;
    --tree.nodeCount;
    return true;
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
    // Keep snapshot IDs aligned with the live tree, which is numbered in-order.
    nextId = 1;
    renumberNodes(root, nextId);
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
    LevelManager levelManager;
    LevelLoader& loader = levelManager.levels;
    std::string levelsPath = std::string(GetApplicationDirectory()) + "data/levels.json";
    bool levelsLoaded = levelManager.load(levelsPath);
    if (!levelsLoaded) levelsLoaded = loader.load("data/levels.json");
    int totalLevels = 0;
    for (const Act& act : loader.acts) totalLevels += (int)act.levels.size();

    int actIdx = 0;
    int levelIdx = 0;
    levelManager.actIndex = actIdx;
    levelManager.levelIndex = levelIdx;
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
    BinaryTree reconstructionTemplate;
    LayoutEngine reconstructionLayout;
    BST sandboxTree;
    AVLTree sandboxAVL;
    BinaryTree sandboxBinaryTree;
    BST endlessTree;
    MaxHeap sandboxHeap;
    LayoutEngine sandboxLayout;
    LayoutEngine endlessLayout;
    EventBus eventBus;
    EventLog eventLog;
    eventLog.attach(eventBus);
    AnimationController animation;
    BinaryTree replayTree;
    BinaryTree replayPreviousTree;
    LayoutEngine replayLayout;
    LayoutEngine replayPreviousLayout;
    AppScreen screen = AppScreen::MainMenu;
    bool menuDarkTheme = true;
    bool quitRequested = false;
    int targetFps = 60;
    std::string screenMessage;
    std::string sandboxInput;
    int sandboxOperation = 0;
    int sandboxQuery = 50;
    int sandboxMode = 1;
    int sandboxNextId = 1;
    std::mt19937 sandboxRandom((unsigned int)GetTime() + 2069U);
    std::mt19937 endlessRandom((unsigned int)GetTime() + 20261119U);
    int endlessScore = 0;
    int endlessLives = 3;
    int endlessValue = 50;
    bool endlessInsert = true;
    double endlessCountdown = 0.0;
    double endlessInterval = 2.5;
    bool endlessGameOver = false;
    SaveManager saveManager(std::string(GetApplicationDirectory()) + "data/leaderboard.txt");
    std::vector<int> currentLevelKeys;
    std::vector<int> patrolOrder;
    std::vector<int> patrolVisited;
    std::vector<int> preorderClue;
    std::vector<int> inorderClue;
    bool reconstructionUsePostorder = false;
    bool reconstructionComplete = false;
    std::vector<int> reconstructionSlots;
    std::vector<bool> reconstructionSlotFilled;
    std::vector<int> reconstructionTokens;
    std::vector<bool> reconstructionTokenUsed;
    std::vector<int> reconstructionPlacementOrder;
    int draggedReconstructionToken = -1;
    int heightTarget = 0;
    int diameterTarget = 0;
    int heightDiameterStep = 0;
    std::vector<int> heightNodeOrder;
    std::vector<int> heightVisited;
    int heightSelectedNodeId = -1;
    std::vector<int> lcaSelected;
    int lcaExpectedId = -1;
    bool lcaPathPhase = false;
    std::vector<int> lcaPathExpected;
    std::vector<int> lcaPathVisited;
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
    Sound alarmSound{};
    bool alarmSoundReady = false;
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
    auto prepareEndlessTask = [&]() {
        std::uniform_int_distribution<int> keyDistribution(10, 999);
        std::uniform_int_distribution<int> opDistribution(0, 1);
        endlessInsert = opDistribution(endlessRandom) == 0 || endlessTree.nodeCount < 2;
        endlessValue = keyDistribution(endlessRandom);
        if (endlessInsert) {
            while (endlessTree.search(endlessValue)) endlessValue = keyDistribution(endlessRandom);
        } else {
            std::vector<int> existing = endlessTree.inorder();
            std::uniform_int_distribution<int> existingIndex(0, (int)existing.size() - 1);
            endlessValue = existing[existingIndex(endlessRandom)];
        }
        endlessCountdown = endlessInterval;
    };
    auto startEndless = [&]() {
        clearNodes(endlessTree.root);
        endlessTree.root = nullptr;
        endlessTree.nodeCount = 0;
        for (int value : {50, 30, 70, 20, 40, 60, 80}) endlessTree.insert(value);
        endlessScore = 0;
        endlessLives = 3;
        endlessInterval = 2.5;
        endlessGameOver = false;
        prepareEndlessTask();
        screen = AppScreen::Endless;
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
        if (id == "1.3") return heightDiameterStep >= (int)heightNodeOrder.size() + 1;
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
        if (id == "1.3") return 2 * (int)heightNodeOrder.size() + 1;
        if (id == "1.4") return tree.diameter() + 5;
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
        return isPatrolLevel() || isSearchLevel() || isLcaLevel() || isHeightDiameterLevel() || isMirrorLevel() ||
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
        eventLog.clear();
        animation.stop();
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
        } else if (lvl.id == "4.3") {
            currentLevelKeys.clear();
            for (int value = 100; (int)currentLevelKeys.size() < itemCount; --value)
                currentLevelKeys.push_back(value);
        } else if (lvl.id == "4.4") {
            currentLevelKeys = {99, 90, 88, 80, 85, 82, 86, 70, 75, 77, 79, 71, 80, 83, 84};
        } else if (lvl.id == "4.5") {
            currentLevelKeys = {50, 50, 50, 40, 40, 50, 50, 30, 30, 40,
                                40, 50, 30, 40, 50, 30, 40, 50, 40, 30};
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
        clearNodes(reconstructionTemplate.root);
        reconstructionTemplate.root = nullptr;
        reconstructionTemplate.nodeCount = 0;
        reconstructionSlots.clear();
        reconstructionSlotFilled.clear();
        reconstructionTokens.clear();
        reconstructionTokenUsed.clear();
        reconstructionPlacementOrder.clear();
        draggedReconstructionToken = -1;
        heightDiameterStep = 0;
        heightNodeOrder.clear();
        heightVisited.clear();
        heightSelectedNodeId = -1;
        lcaSelected.clear();
        lcaExpectedId = -1;
        lcaPathPhase = false;
        lcaPathExpected.clear();
        lcaPathVisited.clear();
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
            reconstructionTemplate.root = reconstructionTemplate.buildFromPreIn(preorderClue, inorderClue);
            reconstructionTemplate.nodeCount = (int)preorderClue.size();
            collectPreorderValues(reconstructionTemplate.root, reconstructionSlots);
            reconstructionSlotFilled.assign(reconstructionSlots.size(), false);
            if (reconstructionUsePostorder) collectPostorderKeys(tree.getRoot(), reconstructionTokens);
            else reconstructionTokens = preorderClue;
            reconstructionTokenUsed.assign(reconstructionTokens.size(), false);
        }
        if (isHeightDiameterLevel()) {
            heightTarget = tree.height();
            diameterTarget = tree.diameter();
            collectPostorderIds(tree.getRoot(), heightNodeOrder);
        }
        if (isOrderStatisticLevel() && currentLevelKeys.size() >= (size_t)orderStatisticK) {
            std::vector<int> sortedKeys = currentLevelKeys;
            std::sort(sortedKeys.begin(), sortedKeys.end());
            orderStatisticTarget = sortedKeys[orderStatisticK - 1];
        }
    };

    resetToLevel();

    std::vector<std::string> undoStack;
    struct HeapUndoState { std::vector<int> heap; std::vector<int> extracted; int activeSize; bool complete; };
    std::vector<HeapUndoState> heapUndoStack;
    LayoutEngine layout; Renderer renderer;

    const int WIN_W = 1100, WIN_H = 850;
    InitWindow(WIN_W, WIN_H, "Vault Heist");
    SetTargetFPS(60);
    InitAudioDevice();
    if (IsAudioDeviceReady()) {
        std::string alarmPath = std::string(GetApplicationDirectory()) + "assets/Alarm.mp3";
        if (!FileExists(alarmPath.c_str())) alarmPath = "assets/Alarm.mp3";
        alarmSound = LoadSound(alarmPath.c_str());
        alarmSoundReady = alarmSound.stream.buffer != nullptr;
        if (alarmSoundReady) SetSoundVolume(alarmSound, 0.8f);
    }
    auto stopAlarmSound = [&]() {
        if (alarmSoundReady) StopSound(alarmSound);
    };

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
    Rectangle sandboxCanvas = {20, 255, 1060, 570};
    auto refreshSandboxHeap = [&]() {
        clearNodes(sandboxTree.root);
        int nextId = 1;
        sandboxTree.root = buildHeapView(sandboxHeap.heap, 0, nullptr, nextId);
        sandboxTree.nodeCount = nextId - 1;
    };
    auto sandboxRoot = [&]() -> TreeNode* {
        if (sandboxMode == 0) return sandboxBinaryTree.getRoot();
        if (sandboxMode == 2) return sandboxAVL.getRoot();
        return sandboxTree.getRoot();
    };
    auto resetSandbox = [&]() {
        clearNodes(sandboxTree.root);
        sandboxTree.root = nullptr;
        sandboxTree.nodeCount = 0;
        clearNodes(sandboxAVL.root);
        sandboxAVL.root = nullptr;
        sandboxAVL.nodeCount = 0;
        clearNodes(sandboxBinaryTree.root);
        sandboxBinaryTree.root = nullptr;
        sandboxBinaryTree.nodeCount = 0;
        sandboxNextId = 1;
        if (sandboxMode == 3) {
            sandboxHeap.heap = {12, 7, 10, 3, 5, 8};
            refreshSandboxHeap();
        } else if (sandboxMode == 0) {
            for (int value : {50, 30, 70, 20, 40, 60, 80})
                insertCompleteBinary(sandboxBinaryTree, value, sandboxNextId);
        } else if (sandboxMode == 2) {
            for (int value : {50, 30, 70, 20, 40, 60, 80}) sandboxAVL.insert(value);
        } else {
            for (int value : {50, 30, 70, 20, 40, 60, 80}) sandboxTree.insert(value);
        }
        sandboxQuery = 50;
        sandboxInput = "50";
        screenMessage = "Sandbox reset.";
    };
    auto runSandboxOperation = [&](int operation) {
        if (sandboxInput.empty()) { screenMessage = "Enter a key first."; return; }
        sandboxQuery = std::stoi(sandboxInput);
        if (sandboxMode == 3) {
            if (operation == 0) { sandboxHeap.insert(sandboxQuery); screenMessage = "Inserted into heap."; }
            else if (operation == 1) { sandboxHeap.buildMaxHeap(sandboxHeap.heap); screenMessage = "Max Heap built."; }
            else if (operation == 2 && !sandboxHeap.heap.empty())
                screenMessage = "Extracted maximum: " + std::to_string(sandboxHeap.extractMax());
            else if (operation == 3) { sandboxHeap.heapSort(); screenMessage = "Heap Sort complete."; }
            else screenMessage = "Heap has no maximum to extract.";
            refreshSandboxHeap();
            return;
        }
        if (sandboxMode == 0) {
            if (operation == 0) screenMessage = insertCompleteBinary(sandboxBinaryTree, sandboxQuery, sandboxNextId)
                ? "Key added to the next open room." : "Could not add the key.";
            else if (operation == 1) screenMessage = removeCompleteBinary(sandboxBinaryTree, sandboxQuery)
                ? "Room removed; deepest value moved into its place." : "Key not found.";
            else if (operation == 2) screenMessage = findTreeNodeByValue(sandboxBinaryTree.getRoot(), sandboxQuery)
                ? "Key found in the binary tree." : "Key not found.";
            return;
        }
        if (sandboxMode == 2) {
            if (operation == 0) screenMessage = sandboxAVL.insert(sandboxQuery)
                ? "Key inserted; AVL balance maintained." : "Duplicate key rejected.";
            else if (operation == 1) screenMessage = sandboxAVL.remove(sandboxQuery)
                ? "Key deleted; AVL balance maintained." : "Key not found.";
            else if (operation == 2) screenMessage = sandboxAVL.search(sandboxQuery)
                ? "Key found in the AVL tree." : "Key not found.";
            else if (operation == 3) {
                std::vector<int> values = sandboxAVL.rangeQuery(sandboxQuery - 10, sandboxQuery + 10);
                screenMessage = "Range [key-10, key+10]: ";
                for (int value : values) screenMessage += std::to_string(value) + " ";
            } else if (operation == 4) {
                screenMessage = "5th smallest: " + std::to_string(sandboxAVL.kth(5));
            }
            return;
        }
        if (operation == 0) screenMessage = sandboxTree.insert(sandboxQuery) ? "Key inserted." : "Duplicate key rejected.";
        else if (operation == 1) screenMessage = sandboxTree.remove(sandboxQuery) ? "Key deleted." : "Key not found.";
        else if (operation == 2) screenMessage = sandboxTree.search(sandboxQuery) ? "Key found." : "Key not found.";
        else if (operation == 3) {
            std::vector<int> values = sandboxTree.rangeQuery(sandboxQuery - 10, sandboxQuery + 10);
            screenMessage = "Range [key-10, key+10]: ";
            for (int value : values) screenMessage += std::to_string(value) + " ";
        } else if (operation == 4) {
            BSTNode* ancestor = sandboxTree.lcaBST(sandboxQuery, sandboxQuery + 10);
            screenMessage = ancestor ? "LCA: " + std::to_string(ancestor->key) : "One requested key is missing.";
        } else if (operation == 5) {
            sandboxTree.mirror(); screenMessage = "Tree mirrored.";
        } else if (operation == 6) {
            screenMessage = rotateLeftAt(sandboxTree, sandboxQuery) ? "Left rotation applied." : "No right child at that key.";
        } else {
            screenMessage = rotateRightAt(sandboxTree, sandboxQuery) ? "Right rotation applied." : "No left child at that key.";
        }
    };
    auto randomizeSandbox = [&]() {
        std::vector<int> values;
        std::uniform_int_distribution<int> distribution(10, 99);
        while (values.size() < 12) {
            int value = distribution(sandboxRandom);
            if (std::find(values.begin(), values.end(), value) == values.end()) values.push_back(value);
        }
        resetSandbox();
        clearNodes(sandboxTree.root);
        sandboxTree.root = nullptr;
        sandboxTree.nodeCount = 0;
        if (sandboxMode == 3) {
            sandboxHeap.heap = values;
            refreshSandboxHeap();
        } else if (sandboxMode == 0) {
            clearNodes(sandboxBinaryTree.root);
            sandboxBinaryTree.root = nullptr;
            sandboxBinaryTree.nodeCount = 0;
            sandboxNextId = 1;
            for (int value : values) insertCompleteBinary(sandboxBinaryTree, value, sandboxNextId);
        } else if (sandboxMode == 2) {
            clearNodes(sandboxAVL.root);
            sandboxAVL.root = nullptr;
            sandboxAVL.nodeCount = 0;
            for (int value : values) sandboxAVL.insert(value);
        } else {
            for (int value : values) sandboxTree.insert(value);
        }
        screenMessage = "Generated 12 random keys for the selected structure.";
    };
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
    auto recordEvent = [&](EventType type, int nodeId = -1, int otherNodeId = -1) {
        if (!BinaryTree::eventRecordingEnabled) return;
        Event event{};
        event.type = type;
        event.nodeId = nodeId;
        event.otherNodeId = otherNodeId;
        const char* action = "Updated the tree";
        switch (type) {
            case EventType::VISIT: action = "Visited a room"; break;
            case EventType::COMPARE: action = "Compared a key"; break;
            case EventType::INSERT: action = "Inserted a value"; break;
            case EventType::UNLINK: action = "Removed a link"; break;
            case EventType::ROTATE: action = "Rotated the tree"; break;
            case EventType::MIRROR: action = "Mirrored the tree"; break;
            case EventType::PATH_TO: action = "Traced a route"; break;
            case EventType::HEAP_BUILD: action = "Built the max heap"; break;
            case EventType::HEAP_SWAP: action = "Swapped heap entries"; break;
            case EventType::EXTRACT_MAX: action = "Extracted the maximum"; break;
            case EventType::MARK_SORTED: action = "Placed value in sorted order"; break;
            case EventType::UNDO: action = "Undid the previous action"; break;
            case EventType::ANSWER: action = "Submitted an answer"; break;
            default: break;
        }
        event.detail = action;
        if (nodeId >= 0) {
            TreeNode* subject = findNodeById(tree.getRoot(), nodeId);
            if (subject) event.detail += ": " + std::to_string(subject->value);
        }
        eventBus.publish(event, serializeTree(tree.getRoot()));
        animation.seek((int)eventLog.size() - 1, (int)eventLog.size());
    };

    auto chargeAlarm = [&](int amount = 1) {
        if (alarmFailed || isCurrentLevelComplete()) return;
        alarmSteps += amount;
        alarmCapacity = ScoreSystem::alarmCapacity(optimalSteps());
        if (alarmSteps >= alarmCapacity) {
            alarmSteps = alarmCapacity;
            alarmFailed = true;
            if (alarmSoundReady) PlaySound(alarmSound);
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
        if (isHeapLevel()) {
            if (heapUndoStack.empty()) { stat = "Nothing to undo"; statC = ERR_C; return; }
            HeapUndoState previous = std::move(heapUndoStack.back());
            heapUndoStack.pop_back();
            heapState.heap = std::move(previous.heap);
            extractedValues = std::move(previous.extracted);
            activeHeapSize = previous.activeSize;
            heapTaskComplete = previous.complete;
            refreshHeapView();
            recordEvent(EventType::UNDO);
            stat = "Last heap operation undone."; statC = OK_C;
            return;
        }
        if (isOrderStatisticLevel()) {
            orderStatisticComplete = false;
            recordEvent(EventType::UNDO);
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
                recordEvent(EventType::UNDO);
                stat = "Last room selection undone."; statC = OK_C;
            } else {
                stat = "No room selections to undo."; statC = ERR_C;
            }
            return;
        }
        if (isRangeLevel()) {
            if (!rangeVisited.empty()) {
                rangeVisited.pop_back();
                recordEvent(EventType::UNDO);
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
                recordEvent(EventType::UNDO);
                stat = "Last mirror action undone."; statC = OK_C;
            } else {
                stat = "No mirror actions to undo."; statC = ERR_C;
            }
            return;
        }
        if (isLcaLevel()) {
            bool changed = false;
            if (lcaComplete) {
                lcaComplete = false;
                changed = true;
                stat = "LCA answer cleared."; statC = OK_C;
            } else if (!lcaPathVisited.empty()) {
                lcaPathVisited.pop_back();
                changed = true;
                stat = "Last path room undone."; statC = OK_C;
            } else if (lcaPathPhase) {
                lcaPathPhase = false;
                lcaPathExpected.clear();
                changed = true;
                stat = "Path tracing undone. Select the LCA again."; statC = OK_C;
            } else if (!lcaSelected.empty()) {
                lcaSelected.pop_back();
                changed = true;
                stat = "Last room selection undone."; statC = OK_C;
            } else {
                stat = "No room selections to undo."; statC = ERR_C;
            }
            TreeNode* ancestor = lcaSelected.size() == 2
                                     ? tree.lca(lcaSelected[0], lcaSelected[1])
                                     : nullptr;
            lcaExpectedId = ancestor ? ancestor->id : -1;
            if (changed) recordEvent(EventType::UNDO);
            return;
        }
        if (isHeightDiameterLevel()) {
            if (heightSelectedNodeId >= 0) {
                heightSelectedNodeId = -1;
            } else if (heightDiameterStep > 0) {
                if (heightDiameterStep <= (int)heightNodeOrder.size() && !heightVisited.empty()) {
                    heightVisited.pop_back();
                }
                --heightDiameterStep;
                recordEvent(EventType::UNDO);
            }
            insB[0] = '\0'; iL = 0;
            stat = "Last answer undone."; statC = OK_C;
            return;
        }
        if (isReconstructionLevel()) {
            if (!reconstructionPlacementOrder.empty()) {
                int slot = reconstructionPlacementOrder.back();
                reconstructionPlacementOrder.pop_back();
                reconstructionSlotFilled[(size_t)slot] = false;
                int value = reconstructionSlots[(size_t)slot];
                for (size_t i = 0; i < reconstructionTokens.size(); ++i) {
                    if (reconstructionTokens[i] == value) reconstructionTokenUsed[i] = false;
                }
                reconstructionComplete = false;
                recordEvent(EventType::UNDO);
                stat = "Last placed room undone."; statC = OK_C;
            } else {
                stat = "No placed rooms to undo."; statC = ERR_C;
            }
            return;
        }
        if (isSearchLevel()) {
            if (searchVisited.empty()) {
                stat = "No search steps to undo"; statC = ERR_C;
            } else {
                searchVisited.pop_back();
                recordEvent(EventType::UNDO);
                stat = "Last search choice undone"; statC = OK_C;
            }
            return;
        }
        if (isPatrolLevel()) {
            if (patrolVisited.empty()) {
                stat = "No patrol steps to undo"; statC = ERR_C;
            } else {
                patrolVisited.pop_back();
                recordEvent(EventType::UNDO);
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
        recordEvent(EventType::UNDO);
        if (isInsertLevel()) insertCompleted = false;
        if (isDeleteLevel()) deleteCompleted = false;
        stat = "Undone"; statC = OK_C;
    };

    auto doReset = [&]() {
        stopAlarmSound();
        eventLog.clear();
        animation.stop();
        undoStack.clear();
        heapUndoStack.clear();
        mistakesThisLevel = 0;
        lastCountedError.clear();
        alarmSteps = 0;
        scorePenalties = 0;
        starsEarned = 0;
        starCap = 3;
        alarmFailed = false;
        if (isHeapLevel()) {
            heapUndoStack.clear();
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
                    BSTNode* insertedNode = tree.search(v);
                    recordEvent(EventType::INSERT, insertedNode ? insertedNode->id : -1);
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
                recordEvent(EventType::UNLINK);
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
        BSTNode* rotationPivot = tree.search(singleRotationCase == "LL" ? 30 : 70);
        const bool rotated = singleRotationCase == "LL"
            ? rotateRightAt(tree, 30) : rotateLeftAt(tree, 70);
        if (rotated) {
            recordEvent(EventType::ROTATE, rotationPivot ? rotationPivot->id : -1);
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
        recordEvent(EventType::ROTATE);
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
            recordEvent(EventType::INSERT);
            if (rotateRightAt(tree, 30)) {
                recordEvent(EventType::ROTATE);
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
        const bool willChange = levelId == "4.1" || levelId == "4.2" ||
                                (levelId == "4.3" && !heapState.heap.empty()) ||
                                ((levelId == "4.4" || levelId == "4.5") && activeHeapSize > 1);
        if (willChange) heapUndoStack.push_back({heapState.heap, extractedValues,
                                                 activeHeapSize, heapTaskComplete});
        if (levelId == "4.1" || levelId == "4.2") {
            std::vector<int> input = heapState.heap;
            heapState.buildMaxHeap(input);
            refreshHeapView();
            heapTaskComplete = heapState.isMaxHeap();
            recordEvent(EventType::HEAP_BUILD);
            recordEvent(EventType::HEAPIFY_DONE);
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
                recordEvent(EventType::EXTRACT_MAX);
                heapTaskComplete = heapState.heap.empty();
                stat = heapTaskComplete ? "All maxima extracted." : "Maximum extracted. Continue.";
                statC = OK_C;
            }
        } else if (activeHeapSize > 1) {
            std::swap(heapState.heap[0], heapState.heap[activeHeapSize - 1]);
            --activeHeapSize;
            heapState.siftDown(0, activeHeapSize);
            refreshHeapView();
            recordEvent(EventType::HEAP_SWAP, 0, activeHeapSize);
            recordEvent(EventType::HEAP_SIZE_CHANGE);
            recordEvent(EventType::HEAPIFY_DONE);
            recordEvent(EventType::MARK_SORTED, activeHeapSize);
            heapTaskComplete = activeHeapSize <= 1;
            stat = heapTaskComplete ? "Heap Sort complete. Array is sorted." : "Sort step complete. Continue.";
            statC = OK_C;
        }
    };

    auto heapArrayText = [&]() {
        const std::string& id = loader.acts[actIdx].levels[levelIdx].id;
        std::string result = (id == "4.4" || id == "4.5") ? "Active heap: [" : "Heap array: [";
        for (int i = 0; i < (int)heapState.heap.size(); ++i) {
            if (i && ((id != "4.4" && id != "4.5") || i != activeHeapSize)) result += " ";
            if ((id == "4.4" || id == "4.5") && i == activeHeapSize) {
                result += "]  |  Sorted region: [";
            }
            result += std::to_string(heapState.heap[i]);
        }
        if (id == "4.4" || id == "4.5") {
            if (activeHeapSize >= (int)heapState.heap.size()) result += "]  |  Sorted region: [";
            result += "]";
        }
        if (id == "4.3" && !extractedValues.empty()) {
            result += "]  |  Extracted (max first): [";
            for (size_t i = 0; i < extractedValues.size(); ++i) {
                if (i) result += " ";
                result += std::to_string(extractedValues[i]);
            }
            result += "]";
        } else if (id != "4.4" && id != "4.5") {
            result += "]";
        }
        return result;
    };

    auto checkHeightDiameterAnswer = [&]() {
        if (alarmFailed) return;
        chargeAlarm();
        if (alarmFailed) return;
        if (iL == 0) {
            stat = "Enter a number first."; statC = ERR_C;
            return;
        }
        if (heightDiameterStep > (int)heightNodeOrder.size()) {
            stat = "All room heights and the diameter are correct."; statC = OK_C;
            return;
        }
        int answer = std::stoi(insB);
        recordEvent(EventType::ANSWER);
        bool answeringRoomHeight = heightDiameterStep < (int)heightNodeOrder.size();
        if (answeringRoomHeight && heightSelectedNodeId < 0) {
            stat = "Select the room whose height you entered."; statC = ERR_C;
            return;
        }
        TreeNode* selectedHeightNode = answeringRoomHeight
            ? findNodeById(tree.getRoot(), heightSelectedNodeId) : nullptr;
        int expected = answeringRoomHeight ? subtreeHeight(selectedHeightNode) : diameterTarget;
        if (answer != expected) {
            stat = answeringRoomHeight ? "That room height is not correct. Count edges to its deepest leaf."
                                       : "That diameter is not correct. Find the longest path in edges.";
            statC = ERR_C;
            return;
        }
        if (answeringRoomHeight) {
            heightVisited.push_back(heightSelectedNodeId);
            heightSelectedNodeId = -1;
        }
        ++heightDiameterStep;
        insB[0] = '\0'; iL = 0;
        if (heightDiameterStep == (int)heightNodeOrder.size()) {
            stat = "Every room height is correct. Now enter the tree diameter."; statC = OK_C;
        } else if (heightDiameterStep > (int)heightNodeOrder.size()) {
            stat = "Room heights and diameter correct. Level complete."; statC = OK_C;
        } else {
            stat = "Correct room height. Select the next room and enter its height."; statC = OK_C;
        }
    };

    auto checkLcaPathLength = [&]() {
        if (alarmFailed || !lcaPathPhase || lcaPathVisited.size() != lcaPathExpected.size()) return;
        chargeAlarm();
        if (alarmFailed || iL == 0) {
            if (!alarmFailed) { stat = "Enter the traced path length."; statC = ERR_C; }
            return;
        }
        int answer = std::stoi(insB);
        recordEvent(EventType::ANSWER);
        int expectedLength = (int)lcaPathExpected.size() - 1;
        if (answer == expectedLength) {
            lcaComplete = true;
            stat = "Correct LCA path and length. Level complete."; statC = OK_C;
        } else {
            stat = "That path length is not correct. Count the edges between the rooms."; statC = ERR_C;
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
        Theme::setDark(menuDarkTheme);
        if (screen != AppScreen::Gameplay) {
            Vector2 mouse = GetMousePosition();
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (screen == AppScreen::Result) stopAlarmSound();
                screen = screen == AppScreen::Result ? AppScreen::CampaignMap : AppScreen::MainMenu;
            }
            BeginDrawing(); Theme::drawBackground(WIN_W, WIN_H);
            if (screen == AppScreen::MainMenu) {
                UIManager::title("VAULT HEIST");
                DrawText("Choose a way into the vault", 375, 108, 22, menuDarkTheme ? RAYWHITE : DARKGRAY);
                const char* labels[] = {"Campaign", "Endless Mode", "Sandbox", "Leaderboard", "Settings", "Quit"};
                AppScreen destinations[] = {AppScreen::CampaignMap, AppScreen::Endless, AppScreen::Sandbox,
                                             AppScreen::Leaderboard, AppScreen::Settings, AppScreen::MainMenu};
                for (int i=0;i<6;++i) if (UIManager::button({390,145.0f+i*78.0f,320,58},labels[i],mouse)) {
                    if (i==0) screen=destinations[i];
                    else if (i==1) startEndless();
                    else if (i==2) { sandboxMode=1; resetSandbox(); screen=AppScreen::Sandbox; }
                    else if (i==3 || i==4) screen=destinations[i];
                    else quitRequested=true;
                }
                DrawText(("Campaign progress: "+std::to_string(completedLevels.size())+" / "+std::to_string(totalLevels)).c_str(),400,630,20,menuDarkTheme?RAYWHITE:DARKGRAY);
            } else if (screen == AppScreen::CampaignMap) {
                UIManager::title("CAMPAIGN MAP");
                for(int a=0;a<(int)loader.acts.size();++a){
                    float y=105.0f+a*168.0f;
                    DrawText(("Act "+std::to_string(loader.acts[a].act)+": "+loader.acts[a].title).c_str(),28,(int)y,19,Theme::cyan());
                    for(int l=0;l<(int)loader.acts[a].levels.size();++l){
                        const Level& level=loader.acts[a].levels[l]; bool unlocked=levelManager.unlocked(a,l,completedLevels);
                        int stars=bestStars.count(level.id)?bestStars[level.id]:0;
                        Rectangle node={30.0f+l*210.0f,y+33,190,85};
                        std::string label=level.id+" ["+std::string(stars,'*')+std::string(3-stars,'-')+"]";
                        if(unlocked){
                            if(UIManager::button(node,label,mouse)){actIdx=a;levelIdx=l;levelManager.actIndex=a;levelManager.levelIndex=l;resetToLevel();undoStack.clear();heapUndoStack.clear();relayout();screen=AppScreen::LevelBriefing;}
                            DrawText(level.name.c_str(),(int)node.x+5,(int)node.y+65,13,menuDarkTheme?RAYWHITE:DARKGRAY);
                        }else{DrawRectangleRec(node,Theme::panel());DrawRectangleLinesEx(node,1,Theme::border());DrawText("LOCKED",(int)node.x+12,(int)node.y+28,17,Theme::muted());}
                    }
                }
                if(UIManager::button({28,790,150,42},"Main Menu",mouse))screen=AppScreen::MainMenu;
            } else if (screen == AppScreen::LevelBriefing) {
                const Level& level=loader.acts[actIdx].levels[levelIdx];
                UIManager::title("LEVEL BRIEFING");
                DrawText(("Act "+std::to_string(loader.acts[actIdx].act)+": "+loader.acts[actIdx].title).c_str(),300,165,22,menuDarkTheme?RAYWHITE:DARKGRAY);
                DrawText((level.id+": "+level.name).c_str(),300,215,28,Theme::cyan());
                DrawText(("Challenge type: "+level.type).c_str(),300,265,21,menuDarkTheme?RAYWHITE:DARKGRAY);
                DrawText(("Rooms/elements: "+std::to_string(level.nodes>0?level.nodes:level.elements)).c_str(),300,305,21,menuDarkTheme?RAYWHITE:DARKGRAY);
                DrawText("Complete the objective before the alarm fills.",300,350,21,menuDarkTheme?RAYWHITE:DARKGRAY);
                if(UIManager::button({385,430,160,58},"Back",mouse))screen=AppScreen::CampaignMap;
                if(UIManager::button({565,430,180,58},"Start Level",mouse)){screen=AppScreen::Gameplay;stat="Started Level "+level.id;statC=OK_C;}
            } else if (screen == AppScreen::Result) {
                UIManager::title("LEVEL RESULTS");
                bool failed=alarmFailed;
                DrawText(failed?"Alarm reached capacity":"Vault challenge complete",260,170,28,failed?MAROON:DARKGREEN);
                DrawText(("Alarm steps: "+std::to_string(alarmSteps)+" / "+std::to_string(alarmCapacity)).c_str(),330,225,22,menuDarkTheme?RAYWHITE:DARKGRAY);
                DrawText(("Score: "+std::to_string(ScoreSystem::score(alarmSteps,scorePenalties))+"    Stars: "+std::string(starsEarned,'*')+std::string(3-starsEarned,'-')).c_str(),330,265,22,menuDarkTheme?RAYWHITE:DARKGRAY);
                if (!failed && actIdx == 3 && (levelIdx == 3 || levelIdx == 4)) {
                    std::string sorted = "Sorted array: ";
                    for (int value : heapState.heap) sorted += std::to_string(value) + "  ";
                    DrawText(sorted.c_str(), 230, 315, 18, Theme::cyan());
                    if (levelIdx == 4)
                        DrawText("Achievement: FINAL VAULT UNLOCKED", 330, 345, 20, GOLD);
                }
                if(UIManager::button({170,370,190,56},"Retry",mouse)){stopAlarmSound();resetToLevel();undoStack.clear();heapUndoStack.clear();relayout();stat="Level restarted.";statC=OK_C;screen=AppScreen::Gameplay;}
                bool hasNext=levelIdx+1<(int)loader.acts[actIdx].levels.size()||actIdx+1<(int)loader.acts.size();
                if(hasNext&&!failed&&UIManager::button({455,370,190,56},"Next Level",mouse)){
                    stopAlarmSound();
                    if(levelIdx+1<(int)loader.acts[actIdx].levels.size())++levelIdx;else{++actIdx;levelIdx=0;}
                    levelManager.actIndex=actIdx;levelManager.levelIndex=levelIdx;resetToLevel();undoStack.clear();heapUndoStack.clear();relayout();screen=AppScreen::Gameplay;
                }
                if(UIManager::button({740,370,190,56},"Campaign Map",mouse)){stopAlarmSound();screen=AppScreen::CampaignMap;}
                if(UIManager::button({455,455,190,56},"Main Menu",mouse)){stopAlarmSound();screen=AppScreen::MainMenu;}
            } else if (screen == AppScreen::Leaderboard) {
                UIManager::title("LOCAL ENDLESS LEADERBOARD"); auto scores=saveManager.topScores();
                if(scores.empty())DrawText("No Endless runs recorded yet.",360,150,22,menuDarkTheme?RAYWHITE:DARKGRAY);
                for(int i=0;i<(int)scores.size();++i)DrawText((std::to_string(i+1)+".    "+std::to_string(scores[i])+" operations").c_str(),390,150+i*42,21,menuDarkTheme?RAYWHITE:DARKGRAY);
                if(UIManager::button({28,790,150,42},"Main Menu",mouse))screen=AppScreen::MainMenu;
            } else if (screen == AppScreen::Settings) {
                UIManager::title("SETTINGS");
                if(UIManager::button({330,190,440,62},"Frame limit: "+std::to_string(targetFps)+" FPS",mouse)){targetFps=targetFps==60?30:60;SetTargetFPS(targetFps);}
                if(UIManager::button({330,280,440,62},std::string("Theme: ")+(menuDarkTheme?"Dark":"Light"),mouse))menuDarkTheme=!menuDarkTheme;
                if(UIManager::button({28,790,150,42},"Main Menu",mouse))screen=AppScreen::MainMenu;
            } else if (screen == AppScreen::Endless) {
                UIManager::title("ENDLESS MODE - SURVIVE THE OPERATION STREAM");
                if(!endlessGameOver){
                    endlessCountdown-=GetFrameTime();
                    if(endlessCountdown<=0){--endlessLives;if(endlessLives<=0){endlessGameOver=true;saveManager.addScore(endlessScore);}else prepareEndlessTask();}
                    bool apply=UIManager::button({430,112,230,58},"Apply Operation [Space]",mouse)||IsKeyPressed(KEY_SPACE);
                    if(apply&&!endlessGameOver){if(endlessInsert)endlessTree.insert(endlessValue);else endlessTree.remove(endlessValue);++endlessScore;endlessInterval=std::max(0.65,2.5-endlessScore*0.035);prepareEndlessTask();}
                    DrawText((std::string(endlessInsert?"INSERT ":"DELETE ")+std::to_string(endlessValue)+"    Deadline: "+std::to_string((int)std::ceil(endlessCountdown))+"s").c_str(),330,184,24,menuDarkTheme?RAYWHITE:DARKGRAY);
                    DrawText(("Score: "+std::to_string(endlessScore)+"    Lives: "+std::string(endlessLives,'*')).c_str(),400,218,21,menuDarkTheme?RAYWHITE:DARKGRAY);
                    endlessLayout.calculateLayout(endlessTree.root,sandboxCanvas,30,60,65);BeginScissorMode((int)sandboxCanvas.x,(int)sandboxCanvas.y,(int)sandboxCanvas.width,(int)sandboxCanvas.height);renderer.drawTree(endlessTree.root,endlessLayout,{},sandboxCanvas);EndScissorMode();
                }else{DrawText(("Run ended. Final score: "+std::to_string(endlessScore)).c_str(),340,210,28,MAROON);if(UIManager::button({410,320,260,60},"Leaderboard",mouse))screen=AppScreen::Leaderboard;}
                if(UIManager::button({28,790,150,42},"Main Menu",mouse))screen=AppScreen::MainMenu;
            } else if (screen == AppScreen::Sandbox) {
                UIManager::title("SANDBOX - FREE-FORM STRUCTURE EDITOR");
                if(UIManager::button({20,82,135,42},"Main Menu",mouse))screen=AppScreen::MainMenu;
                const char* modeNames[] = {"Binary Tree", "BST", "AVL Tree", "Max Heap"};
                if(UIManager::button({175,82,150,42},std::string("Type: ")+modeNames[sandboxMode],mouse)){
                    sandboxMode=(sandboxMode+1)%4; resetSandbox();
                }
                if(UIManager::button({335,82,125,42},"Randomize",mouse))randomizeSandbox();
                DrawText("Key:",475,94,19,Theme::text());DrawRectangle(520,82,95,42,Theme::card());DrawRectangleLines(520,82,95,42,Theme::cyan());DrawText(sandboxInput.c_str(),530,94,20,Theme::text());
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&CheckCollisionPointRec(mouse,{520,82,95,42}))sandboxOperation=99;
                if(sandboxOperation==99){if(IsKeyPressed(KEY_BACKSPACE)&&!sandboxInput.empty())sandboxInput.pop_back();int ch=GetCharPressed();while(ch>0){if(ch>='0'&&ch<='9'&&sandboxInput.size()<4)sandboxInput.push_back((char)ch);ch=GetCharPressed();}}
                const char* treeOps[]={"Insert","Delete","Search","Range","LCA","Mirror","Rotate Left","Rotate Right","Reset"};
                const char* binaryOps[]={"Insert","Delete","Search","Reset"};
                const char* avlOps[]={"Insert","Delete","Search","Range","5th Smallest","Reset"};
                const char* heapOps[]={"Insert","Build Heap","Extract Max","Heap Sort","Reset"};
                int ops=sandboxMode==0?4:(sandboxMode==1?9:(sandboxMode==2?6:5));
                for(int i=0;i<ops;++i){
                    int col=i%5,row=i/5;Rectangle b={20.0f+col*212.0f,142.0f+row*50.0f,195,40};
                    const char* label=sandboxMode==0?binaryOps[i]:(sandboxMode==1?treeOps[i]:(sandboxMode==2?avlOps[i]:heapOps[i]));
                    if(UIManager::button(b,label,mouse)){if(i==ops-1)resetSandbox();else runSandboxOperation(i);}
                }
                DrawText(screenMessage.c_str(),25,238,17,menuDarkTheme?RAYWHITE:DARKGRAY);
                if(sandboxMode==3){std::string values="Heap array: ";for(int v:sandboxHeap.heap)values+=std::to_string(v)+" ";DrawText(values.c_str(),25,260,18,Theme::cyan());}
                TreeNode* viewRoot=sandboxRoot();
                sandboxLayout.calculateLayout(viewRoot,sandboxCanvas,25,60,60);BeginScissorMode((int)sandboxCanvas.x,(int)sandboxCanvas.y,(int)sandboxCanvas.width,(int)sandboxCanvas.height);renderer.drawTree(viewRoot,sandboxLayout,{},sandboxCanvas);EndScissorMode();
            }
            EndDrawing();
            if(quitRequested)break;
            continue;
        }
        if(IsKeyPressed(KEY_ESCAPE)){screen=AppScreen::CampaignMap;continue;}
        alarmCapacity = ScoreSystem::alarmCapacity(optimalSteps());
        if(IsKeyPressed(KEY_SPACE))animation.toggle();
        if(IsKeyPressed(KEY_RIGHT))animation.step(1,(int)eventLog.size());
        if(IsKeyPressed(KEY_LEFT))animation.step(-1,(int)eventLog.size());
        animation.update(GetFrameTime(),(int)eventLog.size());
        if (animation.currentIndex() >= 0 && animation.currentIndex() < (int)eventLog.size()) {
            clearNodes(replayTree.getRoot());
            replayTree.root = deserializeTree(eventLog.stateAt((size_t)animation.currentIndex()),
                                              replayTree.nodeCount);
            replayLayout.calculateLayout(replayTree.getRoot(), canvas, 50, 80, 80.0f);
            if (animation.currentIndex() > 0) {
                clearNodes(replayPreviousTree.getRoot());
                replayPreviousTree.root = deserializeTree(
                    eventLog.stateAt((size_t)animation.currentIndex() - 1), replayPreviousTree.nodeCount);
                replayPreviousLayout.calculateLayout(replayPreviousTree.getRoot(), canvas, 50, 80, 80.0f);
                float progress = animation.transitionProgress();
                for (auto& position : replayLayout.positions) {
                    auto previous = replayPreviousLayout.positions.find(position.first);
                    if (previous == replayPreviousLayout.positions.end()) continue;
                    position.second.x = previous->second.x + (position.second.x - previous->second.x) * progress;
                    position.second.y = previous->second.y + (position.second.y - previous->second.y) * progress;
                }
            }
        }
        Vector2 m = GetMousePosition();
        Rectangle timelineRect={canvas.x+canvas.width-290,canvas.y+8,260,14};
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&eventLog.size()>0&&CheckCollisionPointRec(m,timelineRect)){
            float t=(m.x-timelineRect.x)/timelineRect.width;
            animation.seek((int)(t*(eventLog.size()-1)),(int)eventLog.size());
        }
        auto toScreen = [&](const NodePosition& p) {
            return Vector2{canvas.x + canvas.width / 2.0f + (p.x - (canvas.x + canvas.width / 2.0f)) * cameraZoom + cameraPan.x,
                           canvas.y + canvas.height / 2.0f + (p.y - (canvas.y + canvas.height / 2.0f)) * cameraZoom + cameraPan.y};
        };
        if (isReconstructionLevel() && reconstructionTemplate.getRoot())
            reconstructionLayout.calculateLayout(reconstructionTemplate.getRoot(), canvas, 50, 80, 80.0f);
        auto reconstructionTokenRect = [&](int index) {
            const float tokenWidth = 64.0f, tokenHeight = 30.0f, gap = 8.0f;
            int columns = std::max(1, (int)((canvas.width - 20.0f) / (tokenWidth + gap)));
            int rows = ((int)reconstructionTokens.size() + columns - 1) / columns;
            int row = index / columns, column = index % columns;
            float top = canvas.y + canvas.height - rows * (tokenHeight + gap) - 8.0f;
            return Rectangle{canvas.x + 10.0f + column * (tokenWidth + gap),
                             top + row * (tokenHeight + gap), tokenWidth, tokenHeight};
        };
        bool inCanvas = CheckCollisionPointRec(m, canvas);
        float wheelMove = GetMouseWheelMove();
        if (inCanvas && wheelMove != 0.0f)
            cameraZoom = std::max(0.5f, std::min(2.5f, cameraZoom + wheelMove * 0.1f));
        TreeNode* hoveredNode = nullptr;
        if (inCanvas && (!isReconstructionLevel() || reconstructionComplete)) {
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
        if (isReconstructionLevel() && !reconstructionComplete) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inCanvas) {
                for (int i = 0; i < (int)reconstructionTokens.size(); ++i) {
                    if (!reconstructionTokenUsed[(size_t)i] &&
                        CheckCollisionPointRec(m, reconstructionTokenRect(i))) {
                        draggedReconstructionToken = i;
                        break;
                    }
                }
            }
            if (draggedReconstructionToken >= 0 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                chargeAlarm();
                int targetSlot = -1;
                float bestDistance = 42.0f * cameraZoom;
                for (int i = 0; i < (int)reconstructionSlots.size(); ++i) {
                    if (reconstructionSlotFilled[(size_t)i]) continue;
                    const NodePosition& slot = reconstructionLayout.positions[reconstructionSlots[(size_t)i]];
                    Vector2 center = toScreen(slot);
                    float distance = std::sqrt((m.x-center.x)*(m.x-center.x) + (m.y-center.y)*(m.y-center.y));
                    if (distance < bestDistance) { bestDistance = distance; targetSlot = i; }
                }
                int tokenValue = reconstructionTokens[(size_t)draggedReconstructionToken];
                if (!alarmFailed && targetSlot >= 0 && tokenValue == reconstructionSlots[(size_t)targetSlot]) {
                    reconstructionSlotFilled[(size_t)targetSlot] = true;
                    reconstructionTokenUsed[(size_t)draggedReconstructionToken] = true;
                    reconstructionPlacementOrder.push_back(targetSlot);
                    recordEvent(EventType::INSERT, reconstructionSlots[(size_t)targetSlot]);
                    if (reconstructionPlacementOrder.size() == reconstructionSlots.size()) {
                        reconstructionComplete = true;
                        recordEvent(EventType::ANSWER);
                        stat = "Blueprint reconstructed from the traversal pair."; statC = OK_C;
                    } else {
                        stat = "Correct room. Place the remaining traversal tokens."; statC = OK_C;
                    }
                } else if (!alarmFailed) {
                    stat = "That token does not belong in this room. Try another slot."; statC = ERR_C;
                }
                draggedReconstructionToken = -1;
            }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && inCanvas && !hoveredNode && draggedReconstructionToken < 0) panning = true;
        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) panning = false;
        if (panning && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            cameraPan.x += m.x - previousMouse.x;
            cameraPan.y += m.y - previousMouse.y;
        }
        previousMouse = m;
        auto drawBtn = [&](Rectangle r, const std::string& t) {
            bool h = CheckCollisionPointRec(m, r);
            bool p = h && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            Theme::drawChamferedPanel(r, p ? Theme::green() : (h ? Theme::border() : Theme::card()),
                                      h ? Theme::cyan() : Theme::border());
            DrawText(t.c_str(), (int)r.x + (int)(r.width - MeasureText(t.c_str(), 16))/2,
                     (int)r.y + 7, 16, p ? Theme::background() : Theme::text());
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
        Rectangle measureAnswerRect = { 100, (float)row2Y, 60, 30 };
        Rectangle lcaAnswerRect = { 520, (float)row2Y, 60, 30 };

        // ---- Input handling ----
        if (!alarmFailed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isGuidedLevel() && !CheckCollisionPointRec(m,timelineRect) &&
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
                    } else if (isHeightDiameterLevel()) {
                        if (heightDiameterStep >= (int)heightNodeOrder.size()) {
                            stat = "Room heights are complete. Enter the diameter now."; statC = OK_C;
                        } else if (std::find(heightVisited.begin(), heightVisited.end(), entry.first) != heightVisited.end()) {
                            stat = "This room's height is already recorded."; statC = OK_C;
                        } else if (entry.first != heightNodeOrder[heightVisited.size()]) {
                            stat = "Work bottom-up: select the next room in postorder."; statC = ERR_C;
                        } else {
                            heightSelectedNodeId = entry.first;
                            stat = "Selected room. Enter its height in edges, then check."; statC = OK_C;
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
                        } else if (lcaPathPhase) {
                            if (lcaPathVisited.size() < lcaPathExpected.size() &&
                                entry.first == lcaPathExpected[lcaPathVisited.size()]) {
                                lcaPathVisited.push_back(entry.first);
                                stat = lcaPathVisited.size() == lcaPathExpected.size()
                                    ? "Path traced. Enter its length in edges."
                                    : "Correct room on the path. Continue tracing.";
                                statC = OK_C;
                            } else if (lcaPathVisited.size() == lcaPathExpected.size()) {
                                stat = "Path already traced. Enter its length in edges."; statC = OK_C;
                            } else {
                                stat = "That room is not next on the LCA path."; statC = ERR_C;
                            }
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
                            std::vector<int> pathA = tree.pathTo(lcaSelected[0]);
                            std::vector<int> pathB = tree.pathTo(lcaSelected[1]);
                            size_t common = 0;
                            while (common < pathA.size() && common < pathB.size() &&
                                   pathA[common] == pathB[common]) ++common;
                            if (common == 0) {
                                stat = "Could not form the route. Reset and select rooms again."; statC = ERR_C;
                            } else {
                                size_t lcaIndex = common - 1;
                                for (int i = (int)pathA.size() - 1; i >= (int)lcaIndex; --i)
                                    lcaPathExpected.push_back(pathA[(size_t)i]);
                                for (size_t i = lcaIndex + 1; i < pathB.size(); ++i)
                                    lcaPathExpected.push_back(pathB[i]);
                                lcaPathPhase = true;
                                stat = "Correct LCA. Trace the path from the first room through the LCA to the second.";
                                statC = OK_C;
                            }
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
                    EventType clickEvent = isMirrorLevel() ? EventType::MIRROR
                        : (isSearchLevel() || isDegenerationLevel()) ? EventType::COMPARE
                        : isBalanceLevel() ? EventType::BALANCE_ALERT
                        : isLcaLevel() ? EventType::PATH_TO : EventType::VISIT;
                    recordEvent(clickEvent, entry.first);
                    break;
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isGuidedLevel() && !isHeapLevel()) {
            if (CheckCollisionPointRec(m, iR)) active = 0;
            else if (CheckCollisionPointRec(m, dR)) active = 1;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isReconstructionLevel() &&
            CheckCollisionPointRec(m, {30, (float)row2Y, 710, 30})) active = 0;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isHeightDiameterLevel() &&
            CheckCollisionPointRec(m, measureAnswerRect)) active = 0;
        bool acceptsGuidedAnswer = isHeightDiameterLevel() ||
            (isLcaLevel() && lcaPathPhase && lcaPathVisited.size() == lcaPathExpected.size());
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isLcaLevel() && acceptsGuidedAnswer &&
            CheckCollisionPointRec(m, lcaAnswerRect)) active = 0;
        if (IsKeyPressed(KEY_TAB) && (!isGuidedLevel() || acceptsGuidedAnswer) && !isHeapLevel()) active = 1 - active;

        int k = ((isGuidedLevel() && !acceptsGuidedAnswer) || isHeapLevel()) ? 0 : GetCharPressed();
        while (k > 0) {
            if (active == 0 && iL < 5 && k >= '0' && k <= '9') { insB[iL++] = (char)k; insB[iL] = '\0'; }
            else if (active == 1 && dL < 5 && k >= '0' && k <= '9') { delB[dL++] = (char)k; delB[dL] = '\0'; }
            k = GetCharPressed();
        }
        if ((!isGuidedLevel() || acceptsGuidedAnswer) && !isHeapLevel() && IsKeyPressed(KEY_BACKSPACE)) {
            if (active == 0 && iL > 0) insB[--iL] = '\0';
            else if (active == 1 && dL > 0) delB[--dL] = '\0';
        }
        if (IsKeyPressed(KEY_ENTER) && !alarmFailed) {
            if (isHeightDiameterLevel()) checkHeightDiameterAnswer();
            else if (isLcaLevel()) checkLcaPathLength();
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
        bool validateAfterInput = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
                                  IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z);
        if (validateAfterInput) {
            ValidationResult validation = Validator::validateTreeLinks(tree.getRoot(), tree.nodeCount);
            if (!validation.valid) {
                stat = "Invariant error: " + validation.message;
                statC = ERR_C;
            }
            if (isHeapLevel()) {
                const std::string& id = loader.acts[actIdx].levels[levelIdx].id;
                bool heapShouldBeValid = id != "4.1" && id != "4.2";
                if (heapShouldBeValid) {
                    int checkedSize = (id == "4.4" || id == "4.5")
                                          ? activeHeapSize : (int)heapState.heap.size();
                    ValidationResult heapValidation = Validator::validateHeap(heapState, checkedSize);
                    if (!heapValidation.valid) {
                        stat = "Invariant error: " + heapValidation.message;
                        statC = ERR_C;
                    }
                }
            } else if (!isMirrorLevel() && !isReconstructionLevel()) {
                ValidationResult bstValidation = Validator::validateBST(tree);
                if (!bstValidation.valid) {
                    stat = "Invariant error: " + bstValidation.message;
                    statC = ERR_C;
                }
            }
        }
        if (isCurrentLevelComplete()) {
            const std::string& completedId = loader.acts[actIdx].levels[levelIdx].id;
            int score = ScoreSystem::score(alarmSteps, scorePenalties);
            starsEarned = ScoreSystem::stars(alarmSteps, optimalSteps(), scorePenalties, starCap);
            bool newCompletion = completedLevels.insert(completedId).second;
            auto best = bestScores.find(completedId);
            bool scoreImproved = newCompletion || best == bestScores.end() || score > best->second;
            bool starsImproved = starsEarned > bestStars[completedId];
            if (scoreImproved || starsImproved) {
                if (scoreImproved) bestScores[completedId] = score;
                if (starsImproved) bestStars[completedId] = starsEarned;
                saveCampaignProgress();
            }
            screen = AppScreen::Result;
        }
        if(alarmFailed)screen=AppScreen::Result;
        if (GetTime() - lastStateSave >= 1.0) {
            saveActiveState();
            lastStateSave = GetTime();
        }

        BeginDrawing();
            Theme::drawBackground(WIN_W, WIN_H);

            // Title bar
            DrawRectangle(0, 0, WIN_W, titleH, Theme::panel());
            DrawRectangle(0, titleH - 2, WIN_W, 2, Theme::cyan());
            std::string titleText = "VAULT HEIST";
            if (levelsLoaded && !loader.acts.empty()) {
                titleText += " - Act " + std::to_string(loader.acts[actIdx].act) + ": " + loader.acts[actIdx].title;
                titleText += " | Level " + loader.acts[actIdx].levels[levelIdx].id + ": " + loader.acts[actIdx].levels[levelIdx].name;
            }
            DrawText(titleText.c_str(), 20, 15, 20, Theme::text());
            std::string campaignText = "Complete: " + std::to_string(completedLevels.size()) +
                                       " / " + std::to_string(totalLevels);
            DrawText(campaignText.c_str(), WIN_W - 160, 17, 16, Theme::text());

            // Toolbar panel
            DrawRectangle(0, titleH, WIN_W, panelBot - titleH, Theme::panel());

            // Row 1: Patrol instructions or Insert / Delete + Level Nav
            if (isReconstructionLevel()) {
                std::vector<int> traversal = preorderClue;
                if (reconstructionUsePostorder) {
                    traversal.clear();
                    collectPostorderKeys(tree.getRoot(), traversal);
                }
                std::string clue = std::string(reconstructionUsePostorder ? "Postorder: " : "Preorder: ") + valuesToText(traversal);
                DrawText(clue.c_str(), 30, (int)y + 7, 18, Theme::muted());
            } else if (isHeightDiameterLevel()) {
                DrawText("Postorder room heights:",
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isLcaLevel()) {
                DrawText("Select two rooms, then find their LCA.",
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isRangeLevel()) {
                DrawText(("Range Raid: click keys from " + std::to_string(rangeLow) + " to " +
                          std::to_string(rangeHigh) + " in order").c_str(),
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isBalanceLevel()) {
                DrawText("Click rooms whose balance factor is outside -1, 0, 1.",
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isSingleRotateLevel()) {
                const char* prompt = singleRotationCase == "LL"
                    ? "LL imbalance at room 30. Apply a right rotation."
                    : "RR imbalance at room 70. Apply a left rotation.";
                DrawText(prompt,
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isDoubleRotateLevel()) {
                const char* prompt = doubleRotationCase == "LR"
                    ? "LR case: rotate left at 20, then right at 50."
                    : "RL case: rotate right at 70, then left at 50.";
                DrawText(prompt,
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isInsertPredictLevel()) {
                DrawText("Insert 10 into this tree. Predict the rotation case.",
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isHeapLevel()) {
                const std::string& levelId = loader.acts[actIdx].levels[levelIdx].id;
                const char* prompt = levelId == "4.1" ? "Build a Max Heap from the array."
                                   : levelId == "4.2" ? "Repair this array with bottom-up heapify."
                                   : levelId == "4.3" ? "Extract the maximum values one at a time."
                                   : "Sort the heap one extraction step at a time.";
                DrawText(prompt, 30, (int)y + 7, 18, Theme::muted());
            } else if (isOrderStatisticLevel()) {
                DrawText(("Select the " + std::to_string(orderStatisticK) + "th smallest key").c_str(),
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isDegenerationLevel()) {
                DrawText(("Degeneration: find " + std::to_string(searchTarget) + " in the right-leaning chain").c_str(),
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isMirrorLevel()) {
                DrawText("Mirror every room that has a branch by clicking it.",
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isPatrolLevel()) {
                DrawText("Preorder: click Root, Left, Right", 30, (int)y + 7, 18, Theme::muted());
            } else if (isSearchLevel()) {
                DrawText(("BST search: find " + std::to_string(searchTarget) + " by comparing each room").c_str(),
                         30, (int)y + 7, 18, Theme::muted());
            } else if (isInsertLevel()) {
                DrawText("Insert:", (int)insLabelX, (int)y + 7, 18, Theme::muted());
                DrawRectangleRec(iR, Theme::card());
                DrawRectangleLinesEx(iR, active == 0 ? 2.0f : 1.0f, active == 0 ? Theme::cyan() : Theme::border());
                DrawText(insB, (int)iR.x + 5, (int)iR.y + 7, 18, Theme::text());
                if (active == 0 && ((int)(GetTime() * 2)) % 2 == 0)
                    DrawText("|", (int)iR.x + 7 + MeasureText(insB, 18), (int)iR.y + 7, 18, Theme::text());
                if (drawBtn(iBt, "Insert")) doOp(true);
                DrawText(("Target: " + std::to_string(insertTarget)).c_str(),
                         (int)delLabelX, (int)y + 7, 18, Theme::muted());
            } else if (isDeleteLevel()) {
                DrawText(("Target " + std::to_string(deleteTarget) + ":").c_str(),
                         (int)delLabelX, (int)y + 7, 18, Theme::muted());
                DrawRectangleRec(dR, Theme::card());
                DrawRectangleLinesEx(dR, active == 1 ? 2.0f : 1.0f, active == 1 ? Theme::cyan() : Theme::border());
                DrawText(delB, (int)dR.x + 5, (int)dR.y + 7, 18, Theme::text());
                if (active == 1 && ((int)(GetTime() * 2)) % 2 == 0)
                    DrawText("|", (int)dR.x + 7 + MeasureText(delB, 18), (int)dR.y + 7, 18, Theme::text());
                if (drawBtn(dBt, "Delete")) doOp(false);
            } else {
            DrawText("Insert:", (int)insLabelX, (int)y + 7, 18, Theme::muted());
            DrawRectangleRec(iR, Theme::card());
            DrawRectangleLinesEx(iR, active == 0 ? 2.0f : 1.0f, active == 0 ? Theme::cyan() : Theme::border());
            DrawText(insB, (int)iR.x + 5, (int)iR.y + 7, 18, Theme::text());
            if (active == 0 && ((int)(GetTime() * 2)) % 2 == 0)
                DrawText("|", (int)iR.x + 7 + MeasureText(insB, 18), (int)iR.y + 7, 18, Theme::text());
            if (drawBtn(iBt, "Insert")) doOp(true);

            DrawText("Delete:", (int)delLabelX, (int)y + 7, 18, Theme::muted());
            DrawRectangleRec(dR, Theme::card());
            DrawRectangleLinesEx(dR, active == 1 ? 2.0f : 1.0f, active == 1 ? Theme::cyan() : Theme::border());
            DrawText(delB, (int)dR.x + 5, (int)dR.y + 7, 18, Theme::text());
            if (active == 1 && ((int)(GetTime() * 2)) % 2 == 0)
                DrawText("|", (int)dR.x + 7 + MeasureText(delB, 18), (int)dR.y + 7, 18, Theme::text());
            if (drawBtn(dBt, "Delete")) doOp(false);
            }

            if (drawBtn(pActBt, "Prev Act")) {
                if (actIdx > 0) { actIdx--; levelIdx = 0; resetToLevel(); undoStack.clear(); heapUndoStack.clear(); stat = "Switched to Act " + std::to_string(loader.acts[actIdx].act); statC = OK_C; relayout(); }
            }
            if (drawBtn(nActBt, "Next Act")) {
                if (actIdx < (int)loader.acts.size() - 1) { actIdx++; levelIdx = 0; resetToLevel(); undoStack.clear(); heapUndoStack.clear(); stat = "Switched to Act " + std::to_string(loader.acts[actIdx].act); statC = OK_C; relayout(); }
            }
            if (drawBtn(pLvlBt, "Prev Lvl")) {
                if (levelIdx > 0) { levelIdx--; resetToLevel(); undoStack.clear(); heapUndoStack.clear(); stat = "Switched to Level " + loader.acts[actIdx].levels[levelIdx].id; statC = OK_C; relayout(); }
            }
            if (drawBtn(nLvlBt, "Next Lvl")) {
                if (levelIdx < (int)loader.acts[actIdx].levels.size() - 1) { levelIdx++; resetToLevel(); undoStack.clear(); heapUndoStack.clear(); stat = "Switched to Level " + loader.acts[actIdx].levels[levelIdx].id; statC = OK_C; relayout(); }
            }
            if(drawBtn({920,(float)row1Y,74,30},"Replay")){animation.seek(0,(int)eventLog.size());animation.play();}
            if(drawBtn({1000,(float)row1Y,80,30},animation.isPlaying()?"Pause":"Play"))animation.toggle();

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
                if (drawBtn({ 250, (float)row2Y, 75, 30 }, "Undo [Z]")) doUndo();
                DrawText(heapTaskComplete ? "Complete" : "Use the action to continue",
                         340, row2Y + 7, 16, Theme::muted());
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
                DrawText((std::to_string(doubleRotationStep) + " / 2").c_str(), 486, row2Y + 7, 16, Theme::muted());
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
                         486, row2Y + 7, 16, Theme::muted());
            } else if (isHeightDiameterLevel()) {
                bool askingRoomHeight = heightDiameterStep < (int)heightNodeOrder.size();
                std::string fieldLabel = askingRoomHeight
                    ? (heightSelectedNodeId < 0 ? "Select room:" : "Height:")
                    : "Diameter:";
                DrawText(fieldLabel.c_str(),
                         30, row2Y + 7, 18, Theme::muted());
                DrawRectangleRec(measureAnswerRect, Theme::card());
                DrawRectangleLinesEx(measureAnswerRect, active == 0 ? 2.0f : 1.0f,
                                     active == 0 ? BLUE : GRAY);
                DrawText(insB, (int)measureAnswerRect.x + 5, (int)measureAnswerRect.y + 7, 18, Theme::text());
                if (active == 0 && ((int)(GetTime() * 2)) % 2 == 0)
                    DrawText("|", (int)measureAnswerRect.x + 7 + MeasureText(insB, 18),
                             (int)measureAnswerRect.y + 7, 18, Theme::text());
                if (drawBtn({ 170, (float)row2Y, 70, 30 }, "Check")) checkHeightDiameterAnswer();
                if (drawBtn({ 250, (float)row2Y, 70, 30 }, "Reset")) doReset();
                std::string heightProgress = askingRoomHeight
                    ? std::to_string(heightVisited.size()) + " / " + std::to_string(heightNodeOrder.size()) + " rooms"
                    : (heightDiameterStep > (int)heightNodeOrder.size() ? "Complete" : "Enter diameter");
                DrawText(heightProgress.c_str(), 340, row2Y + 7, 16, Theme::muted());
            } else if (isReconstructionLevel()) {
                std::string clue = "Inorder: " + valuesToText(inorderClue);
                DrawText(clue.c_str(), 30, row2Y + 7, 18, Theme::muted());
                if (drawBtn({ 330, (float)row2Y, 120, 30 }, reconstructionUsePostorder ? "Use Preorder" : "Use Postorder")) {
                    reconstructionUsePostorder = !reconstructionUsePostorder;
                    reconstructionComplete = false;
                    std::fill(reconstructionSlotFilled.begin(), reconstructionSlotFilled.end(), false);
                    std::fill(reconstructionTokenUsed.begin(), reconstructionTokenUsed.end(), false);
                    reconstructionPlacementOrder.clear();
                    reconstructionTokens.clear();
                    if (reconstructionUsePostorder) collectPostorderKeys(tree.getRoot(), reconstructionTokens);
                    else reconstructionTokens = preorderClue;
                    reconstructionTokenUsed.assign(reconstructionTokens.size(), false);
                    draggedReconstructionToken = -1;
                    stat = reconstructionUsePostorder ? "Postorder + inorder reconstruction selected." : "Preorder + inorder reconstruction selected.";
                    statC = OK_C;
                }
                if (drawBtn({ 460, (float)row2Y, 80, 30 }, "Undo")) doUndo();
                if (drawBtn({ 550, (float)row2Y, 80, 30 }, "Reset")) doReset();
            } else if (isLcaLevel()) {
                DrawText(lcaPathPhase && lcaPathVisited.size() == lcaPathExpected.size()
                             ? "Path length:"
                             : (lcaPathPhase ? "Trace path:" : "Rooms / LCA:"),
                         30, row2Y + 7, 18, Theme::muted());
                if (lcaPathPhase && lcaPathVisited.size() == lcaPathExpected.size()) {
                    DrawRectangleRec(lcaAnswerRect, Theme::card());
                    DrawRectangleLinesEx(lcaAnswerRect, active == 0 ? 2.0f : 1.0f,
                                         active == 0 ? BLUE : GRAY);
                    DrawText(insB, (int)lcaAnswerRect.x + 5, (int)lcaAnswerRect.y + 7, 18, Theme::text());
                    if (active == 0 && ((int)(GetTime() * 2)) % 2 == 0)
                        DrawText("|", (int)lcaAnswerRect.x + 7 + MeasureText(insB, 18),
                                 (int)lcaAnswerRect.y + 7, 18, Theme::text());
                    if (drawBtn({ 590, (float)row2Y, 70, 30 }, "Check")) checkLcaPathLength();
                } else {
                    std::string progress = lcaPathPhase
                        ? std::to_string(lcaPathVisited.size()) + " / " + std::to_string(lcaPathExpected.size()) + " rooms"
                        : std::to_string(lcaSelected.size()) + " / 2 selected";
                    DrawText(progress.c_str(), 340, row2Y + 7, 16, Theme::muted());
                }
                if (drawBtn({ 670, (float)row2Y, 70, 30 }, "Undo [Z]")) doUndo();
                if (drawBtn({ 750, (float)row2Y, 70, 30 }, "Reset")) doReset();
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
                         (int)bx, row2Y + 7, 16, Theme::muted());
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
                          std::to_string(shownStars) + "/3").c_str(), WIN_W - 400, row2Y + 7, 16, Theme::muted());
            }

            // Row 3: instructions, Row 4: status
            std::string instructions = "Instructions: Click box or press [TAB] to switch. [ENTER] executes. [Z] Undos.";
            if (isPatrolLevel()) instructions = "Click rooms in root-left-right order. [Z] undoes. Reset restarts the patrol.";
            else if (isSearchLevel()) instructions = "Click each room along the BST search path. [Z] undoes. Reset restarts the search.";
            else if (isLcaLevel()) instructions = "Choose two rooms, identify their LCA, trace the room path between them, then enter its edge length.";
            else if (isMirrorLevel()) instructions = "Each click swaps a room's children. Undo reverses the last swap.";
            else if (isRangeLevel()) instructions = "Click every key in the range from smallest to largest.";
            else if (isDegenerationLevel()) instructions = "Follow each comparison to the end of this unbalanced BST.";
            else if (isBalanceLevel()) instructions = "A room is unbalanced when the subtree height difference is greater than one.";
            else if (isSingleRotateLevel()) instructions = "The heavy path goes left twice, so repair it with one right rotation.";
            else if (isDoubleRotateLevel()) instructions = "The heavy path goes left then right, so apply both rotations in order.";
            else if (isInsertPredictLevel()) instructions = "Choose LL, RR, LR, or RL. Correct predictions trigger an automatic repair.";
            else if (isOrderStatisticLevel()) instructions = "The k-th smallest key is at position k in the sorted order.";
            else if (isHeapLevel()) instructions = "The array and complete-tree view show the same heap. Click the action button to advance.";
            else if (isHeightDiameterLevel()) instructions = "For each room, enter the height of its subtree in edges, then enter the tree diameter.";
            else if (isReconstructionLevel()) instructions = "Drag each traversal value into its matching empty room. Use both traversal pairs.";
            else if (isInsertLevel()) instructions = "Type the target key and press [ENTER] or click Insert. [Z] undoes.";
            else if (isDeleteLevel()) instructions = "Delete the node with two children to watch the inorder successor take its place.";
            instructions += " [H] Hint. Wheel zooms; drag empty canvas to pan.";
            DrawText(instructions.c_str(), 30, instrY, 16, GRAY);
            Color statusColor = statC.r == ERR_C.r && statC.g == ERR_C.g && statC.b == ERR_C.b
                ? Theme::red() : statC.r == OK_C.r && statC.g == OK_C.g && statC.b == OK_C.b
                ? Theme::green() : Theme::muted();
            DrawText(stat.c_str(), 30, statusY, 18, statusColor);

            // Canvas
            BeginScissorMode(canvasX, canvasY, canvasW, canvasH);
                DrawRectangleRec(timelineRect, Theme::border());
                DrawRectangleLinesEx(timelineRect,1,Theme::muted());
                if(eventLog.size()>1&&animation.currentIndex()>=0){
                    float progress=(float)animation.currentIndex()/(float)(eventLog.size()-1);
                    DrawRectangleRec({timelineRect.x,timelineRect.y,timelineRect.width*progress,timelineRect.height},SKYBLUE);
                    DrawCircleV({timelineRect.x+timelineRect.width*progress,timelineRect.y+timelineRect.height/2},5,ORANGE);
                }
                std::string eventLabel=eventLog.empty()?"Event log: no actions yet":"Event "+std::to_string(animation.currentIndex()+1)+" / "+std::to_string(eventLog.size())+"   [Space] play/pause [Left/Right] step";
                DrawText(eventLabel.c_str(), canvasX + 10, (int)timelineRect.y + 18, 13, Theme::text());
                if (animation.currentIndex() >= 0 && animation.currentIndex() < (int)eventLog.size()) {
                    const std::string& detail = eventLog.at((size_t)animation.currentIndex()).detail;
                    DrawText(detail.c_str(), canvasX + 10, (int)timelineRect.y + 34, 13, Theme::cyan());
                }
                if (isHeapLevel()) {
                    std::string arrayText = heapArrayText();
                    DrawText(arrayText.c_str(), canvasX + 15, canvasY + 10, 18, Theme::muted());
                }
                std::unordered_map<int, Color> highlights;
                if (isGuidedLevel()) {
                    if (isOrderStatisticLevel()) {
                        if (orderStatisticComplete) highlights[orderStatisticTargetId] = BLUE;
                    } else if (isHeightDiameterLevel()) {
                        for (int id : heightVisited) highlights[id] = BLUE;
                        if (heightSelectedNodeId >= 0) highlights[heightSelectedNodeId] = GOLD;
                    } else if (isBalanceLevel()) {
                        for (int id : unbalancedSelected) highlights[id] = BLUE;
                    } else if (isRangeLevel()) {
                        for (int id : rangeVisited) highlights[id] = BLUE;
                    } else if (isMirrorLevel()) {
                        for (int id : mirrorFlipped) highlights[id] = BLUE;
                    } else if (isLcaLevel()) {
                        if (!lcaSelected.empty()) highlights[lcaSelected[0]] = BLUE;
                        if (lcaSelected.size() > 1) highlights[lcaSelected[1]] = GOLD;
                        for (int id : lcaPathVisited) highlights[id] = BLUE;
                        if (lcaPathPhase || lcaComplete) highlights[lcaExpectedId] = GREEN;
                    } else {
                        const std::vector<int>& visited = isPatrolLevel() ? patrolVisited : searchVisited;
                        for (int id : visited) highlights[id] = BLUE;
                    }
                }
                if (isRangeLevel()) {
                    std::vector<int> skipped;
                    collectRangeSkippedIds(tree.getRoot(), rangeLow, rangeHigh, skipped);
                    for (int id : skipped) highlights[id] = Color{215, 215, 215, 255};
                }
                if (isBalanceLevel()) {
                    for (int id : unbalancedTargets) highlights[id] = RED;
                    for (int id : unbalancedSelected) highlights[id] = BLUE;
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
                if(animation.currentIndex()>=0&&animation.currentIndex()<(int)eventLog.size()){
                    int eventNode=eventLog.at(animation.currentIndex()).nodeId;
                    if(eventNode>=0)highlights[eventNode]=ORANGE;
                }
                if (!isReconstructionLevel() || reconstructionComplete) {
                    TreeNode* visibleRoot = animation.currentIndex() >= 0 &&
                        animation.currentIndex() < (int)eventLog.size()
                        ? replayTree.getRoot() : tree.getRoot();
                    LayoutEngine& visibleLayout = visibleRoot == replayTree.getRoot()
                        ? replayLayout : layout;
                    renderer.drawTree(visibleRoot, visibleLayout, highlights, canvas, cameraZoom, cameraPan);
                    if (isBalanceLevel()) {
                        for (int id : unbalancedTargets) {
                            auto posIt = layout.positions.find(id);
                            if (posIt == layout.positions.end()) continue;
                            Vector2 badge = toScreen(posIt->second);
                            DrawText("!", (int)badge.x - 5, (int)badge.y - 10, 20, MAROON);
                        }
                    }
                } else {
                    renderer.drawTreeSlots(reconstructionTemplate.getRoot(), reconstructionLayout,
                                           canvas, cameraZoom, cameraPan);
                    for (size_t i = 0; i < reconstructionSlots.size(); ++i) {
                        if (!reconstructionSlotFilled[i]) continue;
                        NodePosition slot = reconstructionLayout.positions[reconstructionSlots[i]];
                        Vector2 center = toScreen(slot);
                        std::string value = std::to_string(reconstructionSlots[i]);
                        DrawText(value.c_str(), (int)center.x - MeasureText(value.c_str(), 20) / 2,
                                 (int)center.y - 10, 20, Theme::text());
                    }
                    for (size_t i = 0; i < reconstructionTokens.size(); ++i) {
                        if (reconstructionTokenUsed[i] || (int)i == draggedReconstructionToken) continue;
                        Rectangle chip = reconstructionTokenRect((int)i);
                        Theme::drawChamferedPanel(chip, Theme::card(), Theme::cyan());
                        std::string value = std::to_string(reconstructionTokens[i]);
                        DrawText(value.c_str(), (int)(chip.x + (chip.width - MeasureText(value.c_str(), 18)) / 2),
                                 (int)chip.y + 6, 18, Theme::text());
                    }
                    if (draggedReconstructionToken >= 0) {
                        std::string value = std::to_string(reconstructionTokens[(size_t)draggedReconstructionToken]);
                        DrawRectangle((int)m.x - 34, (int)m.y - 15, 68, 30, Color{0, 240, 255, 100});
                        DrawText(value.c_str(), (int)m.x - MeasureText(value.c_str(), 18) / 2,
                                 (int)m.y - 9, 18, Theme::text());
                    }
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
                    DrawText("Reset the level to try again.", panelX + 25, panelY + 65, 18, Theme::text());
                } else {
                    std::string stars = "Stars: " + std::string(starsEarned, '*') +
                                        std::string(3 - starsEarned, '-');
                    DrawText(stars.c_str(), panelX + 25, panelY + 61, 22, GOLD);
                    DrawText(("Alarm steps: " + std::to_string(alarmSteps) +
                              " / " + std::to_string(alarmCapacity)).c_str(),
                             panelX + 25, panelY + 92, 17, Theme::text());
                    DrawText(("Score: " + std::to_string(std::max(0, 1000 - 10 * alarmSteps - scorePenalties)) +
                              "    Optimal: " + std::to_string(optimalSteps())).c_str(),
                             panelX + 25, panelY + 119, 17, Theme::text());
                }
                DrawText("Use Reset to retry or Next Lvl to continue.", panelX + 25, panelY + 145, 15, Theme::border());
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
                Theme::drawChamferedPanel({(float)tipX, (float)tipY, (float)tipW, 26},
                                          Theme::card(), Theme::cyan(), 5.0f);
                DrawText(tip.c_str(), tipX + 7, tipY + 5, 15, Theme::text());
            }
            DrawRectangleLines(canvasX, canvasY, canvasW, canvasH, Theme::border());
        EndDrawing();
    }
    saveActiveState();
    stopAlarmSound();
    if (alarmSoundReady) UnloadSound(alarmSound);
    if (IsAudioDeviceReady()) CloseAudioDevice();
    CloseWindow();
    return 0;
}

