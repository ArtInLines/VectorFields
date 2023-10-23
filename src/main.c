#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "raylib.h"
#define AIL_ALL_IMPL
#define AIL_GUI_IMPL
#include "ail.h"
#include "ail_gui.h"
#include "helpers.h"
#include "ir.h"

#define FPS 60
#define N 10000

typedef struct {
	float x;
	float y;
	u8 lifetime;
} Particle;

static i32 winWidth  = 1200;
static i32 winHeight =  800;
static Gui_Input_Box inputBox;
static char *defaultFunc = "(vec2 (sin (+ x y)) (cos (* x y)))";

Particle randParticle(void)
{
	return (Particle) {
		.x = xorshiftf(0, winWidth),
		.y = xorshiftf(0, winHeight),
		.lifetime = xorshift() % (5*FPS),
	};
}

int main(void)
{
    Particle *field = malloc(N * sizeof(Particle));

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(winWidth, winHeight, "Vector Fields");
    SetTargetFPS(FPS);
	Font font = LoadFontEx("./assets/Roboto-Regular.ttf", 40, NULL, 94);
	Gui_El_Style style = {
		.color        = BLACK,
		.bg           = LIGHTGRAY,
		.border_color = DARKGRAY,
		.border_width = 3,
		.font         = font,
		.font_size    = 40,
		.cSpacing     = 0,
		.lSpacing     = 5,
		.pad          = 10,
		.hAlign       = TEXT_ALIGN_LT,
		.vAlign       = TEXT_ALIGN_LT,
	};
	Gui_Label label = gui_newLabel((Rectangle){0}, defaultFunc, style, style);
	inputBox = gui_newInputBox("", true, true, true, label);

	IR root = {0};
	IR updatedRoot;
	parseUserFunc(inputBox.label.text.data, inputBox.label.text.len - 1, &root);
	checkUserFunc(&root);

	SetGesturesEnabled(GESTURE_PINCH_IN | GESTURE_PINCH_OUT);
	float zoomFactor = 10.0f;

	float hideHUDAfter = 5.0f; // in seconds
	float hideHUDSecs  = 0.0f;

	while (!WindowShouldClose()) {
		BeginDrawing();
		if (IsWindowResized()) {
			winWidth  = GetScreenWidth();
			winHeight = GetScreenHeight();
			ClearBackground(BLACK);
		}
		DrawRectangle(0, 0, winWidth, winHeight, (Color){0, 0, 0, 10});

		for (u32 i = 0; i < N; i++) {
			if (!field[i].lifetime) field[i] = randParticle();
			// Normalize field value
			Vector2 in = {
				.x = 2*zoomFactor*field[i].x/winWidth  - zoomFactor,
				.y = 2*zoomFactor*field[i].y/winHeight - zoomFactor,
			};
			IR_Eval_Res res = evalUserFunc(root, in);
			if (!res.succ) break;
			Vector2 v = res.val.v;
			// To prevent very unpleasant visualizations, where the lines span the whole screen height/width
            v.x = AIL_CLAMP(v.x, -2, 2);
            v.y = AIL_CLAMP(v.y, -2, 2);
			float len = lenVector2((Vector2){v.x/2.0f, v.y/2.0f});
			HSLA hsl = {
				AIL_LERP(AIL_CLAMP(len, 0, 1), 30.0f, 200.0f),
				AIL_LERP(AIL_CLAMP(len, 0, 1),   0.5f, 1.0f),
				1.0f,
			};
			DrawLine(field[i].x, field[i].y, field[i].x + v.x, field[i].y + v.y, ColorFromHSV(hsl.h, hsl.s, hsl.l));
			field[i].x += v.x/2.0f;
			field[i].y += v.y/2.0f;
			field[i].lifetime--;
		}

		float wheelVelocity = GetMouseWheelMove();
		if (wheelVelocity == 0.0f) wheelVelocity = lenVector2(GetGesturePinchVector());
		if (wheelVelocity) {
			zoomFactor -= 0.3f * wheelVelocity;
			zoomFactor = AIL_CLAMP(zoomFactor, 0.01f, 1000.0f);
		}

        if (IsKeyPressed(KEY_TAB)) {
            root = randFunction();
            checkUserFunc(&root);
            ail_da_free(inputBox.label.text);
            inputBox.label.text = irToStr(root);
        }

		inputBox.label.bounds.width = winWidth;
		inputBox.label.bounds.width = winHeight;

		if (gui_isPointInRec(GetMouseX(),
							 GetMouseY(),
							 inputBox.label.bounds.x,
							 inputBox.label.bounds.y,
							 inputBox.label.bounds.width,
							 inputBox.label.bounds.height
							)
			|| (IsAnyKeyPressed() && !IsKeyPressed(KEY_TAB))) {
			hideHUDSecs = 0;
		} else {
			hideHUDSecs += GetFrameTime();
		}

		if (hideHUDSecs < hideHUDAfter) {
			bool updated = gui_drawInputBox(&inputBox).updated;
			// @TODO: Show error messages to user
			if (updated) {
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
		}

		// DrawFPS(10, 10);

		EndDrawing();
	}

	free(field);
	return 0;
}