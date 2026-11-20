#ifndef RENDERER_H
#define RENDERER_H

#include "raylib.h"
#include "LayoutEngine.h"
#include "Theme.h"
#include "../models/TreeNode.h"
#include <string>

class Renderer {
public:
    void drawTreeSlots(TreeNode* node, LayoutEngine& layout, Rectangle viewport,
                       float zoom = 1.0f, Vector2 pan = {0, 0}) {
        if (!node) return;
        NodePosition pos = layout.positions[node->id];
        pos.x = viewport.x + viewport.width / 2.0f + (pos.x - (viewport.x + viewport.width / 2.0f)) * zoom + pan.x;
        pos.y = viewport.y + viewport.height / 2.0f + (pos.y - (viewport.y + viewport.height / 2.0f)) * zoom + pan.y;
        float radius = (pos.radius > 0 ? pos.radius : 25.0f) * zoom;
        if (node->left) {
            NodePosition child = layout.positions[node->left->id];
            child.x = viewport.x + viewport.width / 2.0f + (child.x - (viewport.x + viewport.width / 2.0f)) * zoom + pan.x;
            child.y = viewport.y + viewport.height / 2.0f + (child.y - (viewport.y + viewport.height / 2.0f)) * zoom + pan.y;
            DrawLineEx({pos.x, pos.y}, {child.x, child.y}, 1.8f, Theme::muted());
            drawTreeSlots(node->left, layout, viewport, zoom, pan);
        }
        if (node->right) {
            NodePosition child = layout.positions[node->right->id];
            child.x = viewport.x + viewport.width / 2.0f + (child.x - (viewport.x + viewport.width / 2.0f)) * zoom + pan.x;
            child.y = viewport.y + viewport.height / 2.0f + (child.y - (viewport.y + viewport.height / 2.0f)) * zoom + pan.y;
            DrawLineEx({pos.x, pos.y}, {child.x, child.y}, 1.8f, Theme::muted());
            drawTreeSlots(node->right, layout, viewport, zoom, pan);
        }
        DrawCircleLines(pos.x, pos.y, radius, Theme::cyan());
    }

    void drawTree(TreeNode* node, LayoutEngine& layout,
                  const std::unordered_map<int, Color>& highlight = {},
                  Rectangle viewport = {0, 0, 0, 0}, float zoom = 1.0f,
                  Vector2 pan = {0, 0}) {
        if (!node) return;

        NodePosition pos = layout.positions[node->id];
        pos.x = viewport.x + viewport.width / 2.0f + (pos.x - (viewport.x + viewport.width / 2.0f)) * zoom + pan.x;
        pos.y = viewport.y + viewport.height / 2.0f + (pos.y - (viewport.y + viewport.height / 2.0f)) * zoom + pan.y;
        auto it = highlight.find(node->id);
        Color color = Theme::card();
        Color outline = Theme::cyan();
        if (it != highlight.end()) {
            // Previously selected nodes are orange; the current event node is blue.
            if (it->second.r == BLUE.r && it->second.g == BLUE.g) { color = Color{255, 183, 3, 80}; outline = Theme::amber(); }
            else if (it->second.r == ORANGE.r && it->second.g == ORANGE.g) { color = Color{0, 240, 255, 70}; outline = Theme::cyan(); }
            else if (it->second.r == GOLD.r && it->second.g == GOLD.g) { color = Color{255, 183, 3, 80}; outline = Theme::amber(); }
            else if (it->second.r == RED.r || it->second.r == MAROON.r) { color = Color{255, 0, 85, 80}; outline = Theme::red(); }
            else if (it->second.r == GREEN.r && it->second.g == GREEN.g) { color = Color{0, 255, 157, 80}; outline = Theme::green(); }
        }

        float r = (pos.radius > 0 ? pos.radius : 25.0f) * zoom;

        // Draw edges to children first (behind the nodes)
        if (node->left) {
            NodePosition leftPos = layout.positions[node->left->id];
            leftPos.x = viewport.x + viewport.width / 2.0f + (leftPos.x - (viewport.x + viewport.width / 2.0f)) * zoom + pan.x;
            leftPos.y = viewport.y + viewport.height / 2.0f + (leftPos.y - (viewport.y + viewport.height / 2.0f)) * zoom + pan.y;
            DrawLineEx({pos.x, pos.y}, {leftPos.x, leftPos.y}, 5.0f, Color{0, 240, 255, 34});
            DrawLineEx({pos.x, pos.y}, {leftPos.x, leftPos.y}, 1.8f, Theme::cyan());
            drawTree(node->left, layout, highlight, viewport, zoom, pan);
        }
        if (node->right) {
            NodePosition rightPos = layout.positions[node->right->id];
            rightPos.x = viewport.x + viewport.width / 2.0f + (rightPos.x - (viewport.x + viewport.width / 2.0f)) * zoom + pan.x;
            rightPos.y = viewport.y + viewport.height / 2.0f + (rightPos.y - (viewport.y + viewport.height / 2.0f)) * zoom + pan.y;
            DrawLineEx({pos.x, pos.y}, {rightPos.x, rightPos.y}, 5.0f, Color{0, 240, 255, 34});
            DrawLineEx({pos.x, pos.y}, {rightPos.x, rightPos.y}, 1.8f, Theme::cyan());
            drawTree(node->right, layout, highlight, viewport, zoom, pan);
        }

        // Draw the node itself
        if (color.r == Theme::card().r && color.g == Theme::card().g && color.b == Theme::card().b)
            DrawCircleLines(pos.x, pos.y, r + 3.0f, Color{0, 240, 255, 70});
        DrawCircleV({pos.x, pos.y}, r, color);
        DrawCircleLines(pos.x, pos.y, r, outline);

        // Draw the value text (scaled font size if radius is small)
        std::string valStr = std::to_string(node->value);
        int fontSize = (int)(r * 0.8f);
        if (fontSize < 10) fontSize = 10;
        int textWidth = MeasureText(valStr.c_str(), fontSize);

        // Requirement 2: Text inside a highlighted (blue) node must be white
        Color textColor = Theme::text();

        DrawText(valStr.c_str(), pos.x - textWidth / 2, pos.y - fontSize / 2, fontSize, textColor);
    }
};

#endif
