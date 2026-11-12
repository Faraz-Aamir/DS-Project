#ifndef LAYOUT_ENGINE_H
#define LAYOUT_ENGINE_H

#include "../models/TreeNode.h"
#include "raylib.h"
#include <unordered_map>
#include <algorithm>

struct NodePosition {
    float x;
    float y;
    float radius;
};

class LayoutEngine {
public:
    std::unordered_map<int, NodePosition> positions;
    float computedRadius = 25.0f;

    void calculateLayout(TreeNode* root, const Rectangle& canvas,
                          float startY, float levelSpacing,
                          float maxGap = 80.0f) {
        positions.clear();
        computedRadius = 25.0f;
        if (!root) return;

        int nodeCount = countNodes(root);
        int maxDepth = 0;
        getMaxDepth(root, 0, maxDepth);
        if (nodeCount == 0) return;

        // 1. Assign inorder slot indices
        int xIndex = 0;
        assignInorderSlots(root, xIndex, 0, levelSpacing);

        // 2. Determine horizontal spacing (cap gap)
        float slotWidth = canvas.width / (nodeCount + 1);
        float spacing = std::min(slotWidth, maxGap);

        // 3. Compute raw x positions
        float minWidth = 1e9f, maxWidth = -1e9f;
        for (auto& pair : positions) {
            float x = canvas.x + (pair.second.x + 0.5f) * spacing;
            pair.second.x = x;
            minWidth = std::min(minWidth, x);
            maxWidth = std::max(maxWidth, x);
        }

        // 4. Center horizontally
        float treeWidth = maxWidth - minWidth;
        float canvasCenter = canvas.x + canvas.width / 2.0f;
        float treeCenter = (minWidth + maxWidth) / 2.0f;
        float offset = canvasCenter - treeCenter;

        // 5. Overflow handling: scale radius and spacing if tree exceeds canvas bounds
        float totalTreeHeight = (maxDepth + 1) * levelSpacing;
        float availableHeight = canvas.height - startY - 20;

        float scale = 1.0f;
        if (treeWidth > canvas.width - 40) {
            scale = std::min(scale, (canvas.width - 40) / treeWidth);
        }
        if (totalTreeHeight > availableHeight) {
            scale = std::min(scale, availableHeight / totalTreeHeight);
        }

        computedRadius = std::max(12.0f, 25.0f * scale);

        for (auto& pair : positions) {
            pair.second.x = canvas.x + (pair.second.x - minWidth) * scale + offset * (1.0f - scale); // smooth shift
            // actually let's keep treeCenter alignment:
            pair.second.x = canvasCenter + (pair.second.x - treeCenter) * scale;
            pair.second.y = canvas.y + startY + pair.second.y * scale;
            pair.second.radius = computedRadius;
        }
    }

private:
    int countNodes(TreeNode* node) {
        if (!node) return 0;
        return 1 + countNodes(node->left) + countNodes(node->right);
    }

    void getMaxDepth(TreeNode* node, int depth, int& maxD) {
        if (!node) return;
        maxD = std::max(maxD, depth);
        getMaxDepth(node->left, depth + 1, maxD);
        getMaxDepth(node->right, depth + 1, maxD);
    }

    void assignInorderSlots(TreeNode* node, int& xIndex, int depth,
                             float levelSpacing) {
        if (!node) return;
        assignInorderSlots(node->left, xIndex, depth + 1, levelSpacing);
        positions[node->id] = { (float)xIndex, depth * levelSpacing, 25.0f };
        xIndex++;
        assignInorderSlots(node->right, xIndex, depth + 1, levelSpacing);
    }
};

#endif