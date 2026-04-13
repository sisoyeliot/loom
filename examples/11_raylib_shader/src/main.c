#include "raylib.h"

const char *GLSL_FRAG = 
"#version 330\n"
"in vec2 fragTexCoord;\n"
"in vec4 fragColor;\n"
"out vec4 finalColor;\n"
"uniform float time;\n"
"uniform vec2 resolution;\n"
"void main() {\n"
"    vec2 uv = gl_FragCoord.xy / resolution.xy;\n"
"    uv = uv * 2.0 - 1.0;\n"
"    uv.x *= resolution.x / resolution.y;\n"
"    float d = length(uv);\n"
"    vec3 col = vec3(0.5 + 0.5 * cos(time + uv.xyx + vec3(0, 2, 4)));\n"
"    d = sin(d * 8.0 - time * 2.0) / 8.0;\n"
"    d = abs(d);\n"
"    d = 0.02 / d;\n"
"    col *= d;\n"
"    finalColor = vec4(col, 1.0);\n"
"}\n";

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Loom + Raylib - Shader Core");

    Shader shader = LoadShaderFromMemory(0, GLSL_FRAG);
    int timeLoc = GetShaderLocation(shader, "time");
    int resLoc = GetShaderLocation(shader, "resolution");

    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(shader, resLoc, resolution, SHADER_UNIFORM_VEC2);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float time = (float)GetTime();
        SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
            ClearBackground(BLACK);
            
            BeginShaderMode(shader);
                DrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
            EndShaderMode();
            
            DrawFPS(10, 10);
            DrawText("Raylib Shader compiled by Loom Build System", 10, 40, 20, RAYWHITE);
        EndDrawing();
    }

    UnloadShader(shader);
    CloseWindow();

    return 0;
}
