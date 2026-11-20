#ifndef VAULT_HEIST_UI_MANAGER_H
#define VAULT_HEIST_UI_MANAGER_H

#include "raylib.h"
#include "Theme.h"
#include <string>

enum class AppScreen { MainMenu, CampaignMap, LevelBriefing, Gameplay, Endless, Sandbox, Leaderboard, Settings, Result };

class UIManager {
public:
    static bool button(Rectangle rect, const std::string& label, Vector2 mouse) {
        bool hovered = CheckCollisionPointRec(mouse, rect);
        bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        Color fill = clicked ? Theme::cyan() : hovered ? Theme::border() : Theme::card();
        Theme::drawChamferedPanel(rect, fill, hovered ? Theme::cyan() : Theme::border());
        DrawText(label.c_str(), (int)(rect.x + (rect.width - MeasureText(label.c_str(), 20)) / 2),
                 (int)(rect.y + (rect.height - 20) / 2), 20,
                 clicked ? Theme::background() : Theme::text());
        return clicked;
    }
    static void title(const char* text) {
        DrawRectangle(0, 0, GetScreenWidth(), 70, Theme::panel());
        DrawRectangle(0, 68, GetScreenWidth(), 2, Theme::cyan());
        DrawText(text, 28, 20, 28, Theme::text());
    }
};

#endif
