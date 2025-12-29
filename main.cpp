#include <raylib.h>

int main() {
    InitWindow(800, 600, "Sailing Sim Test");
    SetTargetFPS(60);
    
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLUE);
        DrawText("Raylib is working!", 250, 250, 30, WHITE);
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}
