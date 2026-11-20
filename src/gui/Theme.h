#ifndef VAULT_HEIST_THEME_H
#define VAULT_HEIST_THEME_H

#include "raylib.h"

class Theme {
public:
    static inline bool dark = true;
    static void setDark(bool enabled) { dark = enabled; }

    static Color background() { return dark ? Color{7, 9, 14, 255} : Color{218, 231, 240, 255}; }
    static Color panel() { return dark ? Color{11, 16, 25, 255} : Color{237, 244, 248, 255}; }
    static Color card() { return dark ? Color{16, 23, 38, 255} : Color{216, 231, 240, 255}; }
    static Color border() { return dark ? Color{27, 42, 64, 255} : Color{84, 118, 139, 255}; }
    static Color cyan() { return dark ? Color{0, 240, 255, 255} : Color{0, 117, 145, 255}; }
    static Color green() { return dark ? Color{0, 255, 157, 255} : Color{0, 132, 91, 255}; }
    static Color amber() { return dark ? Color{255, 183, 3, 255} : Color{169, 105, 0, 255}; }
    static Color red() { return dark ? Color{255, 0, 85, 255} : Color{174, 24, 67, 255}; }
    static Color text() { return dark ? Color{209, 226, 243, 255} : Color{16, 37, 57, 255}; }
    static Color muted() { return dark ? Color{125, 145, 167, 255} : Color{78, 104, 122, 255}; }

    static void drawChamferedPanel(Rectangle rect, Color fill, Color outline, float cut = 7.0f) {
        Vector2 points[8] = {
            {rect.x + cut, rect.y}, {rect.x + rect.width - cut, rect.y},
            {rect.x + rect.width, rect.y + cut}, {rect.x + rect.width, rect.y + rect.height - cut},
            {rect.x + rect.width - cut, rect.y + rect.height}, {rect.x + cut, rect.y + rect.height},
            {rect.x, rect.y + rect.height - cut}, {rect.x, rect.y + cut}
        };
        DrawTriangleFan(points, 8, fill);
        for (int i = 0; i < 8; ++i) DrawLineEx(points[i], points[(i + 1) % 8], 1.3f, outline);
    }

    static void drawBackground(int width, int height) {
        ClearBackground(background());
        Color grid = dark ? Color{0, 240, 255, 12} : Color{0, 117, 145, 24};
        for (int x = 0; x < width; x += 36) DrawLine(x, 0, x, height, grid);
        for (int y = 0; y < height; y += 36) DrawLine(0, y, width, y, grid);
        if (dark) DrawCircleGradient({width / 2.0f, 80.0f}, 340.0f,
                                     Color{0, 240, 255, 16}, Color{7, 9, 14, 0});
    }
};

#endif
