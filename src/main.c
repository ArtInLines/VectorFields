#include "raylib.h"
#include "rlgl.h"
#define AIL_ALL_IMPL
#define AIL_GUI_IMPL
#include "ail.h"
#include "ail_gui.h"
#include "helpers.h"
#include "ir.h"

// @Note: Define SCREEN_SAVER to start app in fullscreen and close it immediately with Escape
// @Note: Define START_FULLSCREEN to start app in fullscreen

#if defined(SCREEN_SAVER) && !defined(START_FULLSCREEN)
#   define START_FULLSCREEN
#endif

#define FONT_SIZE 30
#define INIT_WIDTH 1200
#define INIT_HEIGHT 800
#define FPS 60
#define N 10000

typedef struct {
    float x;
    float y;
    u8 lifetime;
} Particle;

////////////////////
// Global Variables (someone better call the clean code police)
////////////////////
static Rectangle nonFullscreenWin = {0, 0, INIT_WIDTH, INIT_HEIGHT};
static i32   fieldWidth   = INIT_WIDTH;
static i32   fieldHeight  = INIT_HEIGHT;
static bool  showField    = true;
static float hideHUDAfter = 5.0f; // in seconds
static float hideHUDSecs;
static float hueOffset;
static float zoomFactor   = 10.0f;
static Particle *field;
static IR root;
static IR updatedRoot;
static AIL_Gui_Input_Box inputBox;
static char *defaultFunc = "(vec2 (sin (+ x y)) (cos (* x y)))";


bool isKeyPressedPopped(KeyboardKey key)
{
    if (IsKeyPressed(key)) {
        GetKeyPressed();
        GetCharPressed();
        return true;
    }
    return false;
}

void toggleFullscreen(void)
{
    if (IsWindowState(FLAG_FULLSCREEN_MODE)) {
        ClearWindowState(FLAG_FULLSCREEN_MODE | FLAG_BORDERLESS_WINDOWED_MODE);
        SetWindowPosition(nonFullscreenWin.x, nonFullscreenWin.y);
        fieldWidth  = nonFullscreenWin.width;
        fieldHeight = nonFullscreenWin.height;
    } else {
        Vector2 winPos = GetWindowPosition();
        nonFullscreenWin.x      = winPos.x;
        nonFullscreenWin.y      = winPos.y;
        nonFullscreenWin.width  = fieldWidth;
        nonFullscreenWin.height = fieldHeight;
        SetWindowState(FLAG_FULLSCREEN_MODE | FLAG_BORDERLESS_WINDOWED_MODE);
        fieldWidth  = GetScreenWidth();
        fieldHeight = GetScreenHeight();
        SetWindowSize(fieldWidth, fieldHeight);
    }
}

Particle randParticle(void)
{
    return (Particle) {
        .x = xorshiftf(0, fieldWidth),
        .y = xorshiftf(0, fieldHeight),
        .lifetime = xorshift() % (5*FPS),
    };
}

void drawVectorField(void)
{
    DrawRectangle(0, 0, fieldWidth, fieldHeight, (Color){0, 0, 0, 10});

    for (u32 i = 0; i < N; i++) {
        if (!field[i].lifetime) field[i] = randParticle();
        // Normalize field value
        Vector2 in = {
            .x = 2*zoomFactor*field[i].x/fieldWidth  - zoomFactor,
            .y = 2*zoomFactor*field[i].y/fieldHeight - zoomFactor,
        };
        IR_Eval_Res res = evalUserFunc(root, in);
        if (!res.succ) break;
        Vector2 v = res.val.v;
        // To prevent very unpleasant visualizations, where the lines span the whole screen height/width
        v.x = AIL_CLAMP(v.x, -2, 2);
        v.y = AIL_CLAMP(v.y, -2, 2);
        float len = lenVector2((Vector2){v.x/2.0f, v.y/2.0f});
        float h   = hueOffset + AIL_LERP(AIL_CLAMP(len, 0, 1), 0.0f, 60.0f);
        if (h > 360.0f) h -= 360.0f;
        float s   = AIL_LERP(AIL_CLAMP(len, 0, 1), 0.5f, 1.0f);
        float l   = 1.0;
        DrawLine(field[i].x, field[i].y, field[i].x + v.x, field[i].y + v.y, ColorFromHSV(h, s, l));
        field[i].x += v.x/2.0f;
        field[i].y += v.y/2.0f;
        field[i].lifetime--;
    }
    hueOffset += 0.1f;
    if (AIL_UNLIKELY(hueOffset > 360.0f)) hueOffset = 0.0f;

    float wheelVelocity = GetMouseWheelMove();
    if (wheelVelocity == 0.0f) wheelVelocity = lenVector2(GetGesturePinchVector());
    if (wheelVelocity) {
        zoomFactor -= 0.3f * wheelVelocity;
        zoomFactor = AIL_CLAMP(zoomFactor, 0.01f, 1000.0f);
    }

    if (IsKeyPressed(KEY_TAB)) {
        root = randFunction();
        checkUserFunc(&root);
        ail_da_free(&inputBox.label.text);
        inputBox.label.text = irToStr(root);
        inputBox.cur = 0;
    }

    inputBox.label.bounds.width  = fieldWidth;
    inputBox.label.bounds.height = fieldHeight;
    Rectangle b = inputBox.label.bounds;
    ail_gui_resizeLabel(&inputBox.label, ail_gui_getState(b.x, b.y, b.width, b.height));

    if (ail_gui_isPointInRec(GetMouseX(),
                         GetMouseY(),
                         inputBox.label.bounds.x,
                         inputBox.label.bounds.y,
                         inputBox.label.bounds.width,
                         inputBox.label.bounds.height
                        )
        || (IsAnyKeyPressed() && !IsKeyPressed(KEY_TAB))) {
        hideHUDSecs = 0;
        inputBox.selected = true;
    } else {
        hideHUDSecs += GetFrameTime();
    }

    if (hideHUDSecs < hideHUDAfter) {
        AIL_Gui_Update_Res res = ail_gui_drawInputBox(&inputBox);
        if (res.escape || res.tab) inputBox.selected = false;
        // @TODO: Show error messages to user
        if (res.updated) {
            updatedRoot = (IR) {0};
            Parse_Err err = parseUserFunc(inputBox.label.text.data, inputBox.label.text.len - 1, &updatedRoot);
            if (err.msg) {
                printf("Error in parsing at index %d: '%s'\n", err.idx, err.msg);
            } else if (checkUserFunc(&updatedRoot)) {
                root = updatedRoot;
            } else {
                printf("Error in type checking\n");
            }
        }
    } else {
        inputBox.selected = false;
    }
}

int main(void)
{
    field = malloc(N * sizeof(Particle));

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(fieldWidth, fieldHeight, "Vector Fields");
    SetGesturesEnabled(GESTURE_PINCH_IN | GESTURE_PINCH_OUT);
    SetTargetFPS(FPS);
    SetExitKey(KEY_F4);
#ifdef START_FULLSCREEN
    toggleFullscreen(); // @Note: Starts the application in fullscreen, particularly nice when used as a screen-saver
#endif
    Font font = LoadFontEx("./assets/Roboto-Regular.ttf", FONT_SIZE, NULL, 94);
    AIL_Gui_Style style = {
        .color        = BLACK,
        .bg           = LIGHTGRAY,
        .border_color = DARKGRAY,
        .border_width = 3,
        .font         = font,
        .font_size    = FONT_SIZE,
        .cSpacing     = 0,
        .lSpacing     = 2,
        .pad          = 10,
        .hAlign       = AIL_GUI_ALIGN_LT,
        .vAlign       = AIL_GUI_ALIGN_LT,
    };
    AIL_Gui_Label label = ail_gui_newLabel((Rectangle){0}, defaultFunc, style, style);
    inputBox = ail_gui_newInputBox("", true, true, true, label);

    parseUserFunc(inputBox.label.text.data, inputBox.label.text.len - 1, &root);
    checkUserFunc(&root);

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            fieldWidth  = GetScreenWidth();
            fieldHeight = GetScreenHeight();
            BeginDrawing();
            ClearBackground(BLACK);
            EndDrawing();
        }
        BeginDrawing();

#ifdef SCREEN_SAVER
        if (IsKeyPressed(KEY_ESCAPE)) break;
#endif

        if (showField) {
            if (!inputBox.selected) {
                if (isKeyPressedPopped(KEY_F)) toggleFullscreen();
                else if (isKeyPressedPopped(KEY_P)) {
                    const char pathPrefix[] = "./screenshot-";
                    char path[sizeof(pathPrefix) + 7] = {0};
                    memcpy(path, pathPrefix, sizeof(pathPrefix));
                    i32 i = 1;
                    do {
                        path[sizeof(pathPrefix) - 1] = i > 100 ? (i-=100) + '0' : '0';
                        path[sizeof(pathPrefix) + 0] = i > 10  ? (i-=10)  + '0' : '0';
                        path[sizeof(pathPrefix) + 1] = i + '0';
                        path[sizeof(pathPrefix) + 2] = '.';
                        path[sizeof(pathPrefix) + 3] = 'p';
                        path[sizeof(pathPrefix) + 4] = 'n';
                        path[sizeof(pathPrefix) + 5] = 'g';
                        path[sizeof(pathPrefix) + 6] = 0;
                        i++;
                    } while (FileExists(path));
                    printf("%s\n", path);
                    u8 *pixels = rlReadScreenPixels(fieldWidth, fieldHeight);
                    Image img  = { pixels, fieldWidth, fieldHeight, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
                    ExportImage(img, path);
                }
                else if (isKeyPressedPopped(KEY_ESCAPE)) {
                    if (IsWindowState(FLAG_FULLSCREEN_MODE)) toggleFullscreen();
                    else break;
                }
            } else if (isKeyPressedPopped(KEY_ESCAPE)) {
                inputBox.selected = false;
            }
            drawVectorField();
        }

        EndDrawing();
    }

    CloseWindow();
    free(field);
    return 0;
}
