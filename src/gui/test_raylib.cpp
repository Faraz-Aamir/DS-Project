#include "raylib.h"

int main() {
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Vault Heist - Raylib Test");

    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        // TODO: Add update logic

        // Draw
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Raylib is working!", 190, 200, 20, LIGHTGRAY);
            DrawText("Vault Heist - Foundation Ready", 190, 250, 20, DARKBLUE);
            DrawCircle(screenWidth/2, screenHeight/2 + 50, 40, MAROON);
        EndDrawing();
    }

    // De-Initialization
    CloseWindow();

    return 0;
}
