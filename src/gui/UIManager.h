#ifndef VAULT_HEIST_UI_MANAGER_H
#define VAULT_HEIST_UI_MANAGER_H

#include "raylib.h"
#include <string>

enum class AppScreen { MainMenu, CampaignMap, LevelBriefing, Gameplay, Endless, Sandbox, Leaderboard, Settings, Result };

class UIManager {
public:
    static bool button(Rectangle rect, const std::string& label, Vector2 mouse) {
        bool hovered = CheckCollisionPointRec(mouse, rect);
        bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        DrawRectangleRec(rect, clicked ? LIGHTGRAY : hovered ? Color{225, 235, 250, 255} : RAYWHITE);
        DrawRectangleLinesEx(rect, 1.0f, DARKBLUE);
        DrawText(label.c_str(), (int)(rect.x + (rect.width - MeasureText(label.c_str(), 20)) / 2),
                 (int)(rect.y + (rect.height - 20) / 2), 20, DARKBLUE);
        return clicked;
    }
    static void title(const char* text) {
        DrawRectangle(0, 0, GetScreenWidth(), 70, Color{0, 60, 130, 255});
        DrawText(text, 28, 20, 28, RAYWHITE);
    }
};

#endif
