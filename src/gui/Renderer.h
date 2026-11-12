#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"
#include "LayoutEngine.h"
#include "../models/TreeNode.h"
#include <string>

class Renderer {
public:
    void drawTree(TreeNode* node, LayoutEngine& layout,
                  const std::unordered_map<int, Color>& highlight = {},
                  Rectangle viewport = {0, 0, 0, 0}, float zoom = 1.0f,
                  Vector2 pan = {0, 0}) {
        if (!node) return;

        NodePosition pos = layout.positions[node->id];
        pos.x = viewport.x + viewport.width / 2.0f + (pos.x - (viewport.x + viewport.width / 2.0f)) * zoom + pan.x;
        pos.y = viewport.y + viewport.height / 2.0f + (pos.y - (viewport.y + viewport.height / 2.0f)) * zoom + pan.y;
        Color color = RAYWHITE;

        auto it = highlight.find(node->id);
        if (it != highlight.end()) color = it->second;

        float r = (pos.radius > 0 ? pos.radius : 25.0f) * zoom;

        // Draw edges to children first (behind the nodes)
        if (node->left) {
            NodePosition leftPos = layout.positions[node->left->id];
            leftPos.x = viewport.x + viewport.width / 2.0f + (leftPos.x - (viewport.x + viewport.width / 2.0f)) * zoom + pan.x;
            leftPos.y = viewport.y + viewport.height / 2.0f + (leftPos.y - (viewport.y + viewport.height / 2.0f)) * zoom + pan.y;
            DrawLineEx({pos.x, pos.y}, {leftPos.x, leftPos.y}, 2.0f, DARKGRAY);
            drawTree(node->left, layout, highlight, viewport, zoom, pan);
        }
        if (node->right) {
            NodePosition rightPos = layout.positions[node->right->id];
            rightPos.x = viewport.x + viewport.width / 2.0f + (rightPos.x - (viewport.x + viewport.width / 2.0f)) * zoom + pan.x;
            rightPos.y = viewport.y + viewport.height / 2.0f + (rightPos.y - (viewport.y + viewport.height / 2.0f)) * zoom + pan.y;
            DrawLineEx({pos.x, pos.y}, {rightPos.x, rightPos.y}, 2.0f, DARKGRAY);
            drawTree(node->right, layout, highlight, viewport, zoom, pan);
        }

        // Draw the node itself
        DrawCircleV({pos.x, pos.y}, r, color);
        DrawCircleLines(pos.x, pos.y, r, BLACK);

        // Draw the value text (scaled font size if radius is small)
        std::string valStr = std::to_string(node->value);
        int fontSize = (int)(r * 0.8f);
        if (fontSize < 10) fontSize = 10;
        int textWidth = MeasureText(valStr.c_str(), fontSize);

        // Requirement 2: Text inside a highlighted (blue) node must be white
        Color textColor = DARKBLUE;
        if (color.r == BLUE.r && color.g == BLUE.g && color.b == BLUE.b) {
            textColor = RAYWHITE;
        }

        DrawText(valStr.c_str(), pos.x - textWidth / 2, pos.y - fontSize / 2, fontSize, textColor);
    }
};

#endif
