#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define i8  int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define FPS 60

typedef struct {
	float x;
	float y;
	u8 lifetime;
} Particle;

static i32 winWidth  = 1200;
static i32 winHeight =  800;

static u32 RNGState = 69;

u32 xorshift()
{
	RNGState ^= RNGState << 13;
	RNGState ^= RNGState >> 17;
	RNGState ^= RNGState << 5;
	return RNGState;
}

float xorshiftf(float min, float max)
{
	return min + (max - min)*((float)(xorshift() % UINT32_MAX)/(float)UINT32_MAX);
}

Particle randParticle(void)
{
	return (Particle) {
		.x = xorshiftf(0, winWidth),
		.y = xorshiftf(0, winHeight),
		.lifetime = xorshift() % (3*FPS),
	};
}

Vector2 func(i32 x, i32 y)
{
	float a = 2.0f*(float)x/(float)winWidth  - 1.0f;
	float b = 2.0f*(float)y/(float)winHeight - 1.0f;
	return (Vector2) {
		sinf(a + b),
		tanf(a * b),
	};
}

#define GET_SIZE() winWidth * winHeight / 100

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(winWidth, winHeight, "Vector Fields");
    SetTargetFPS(FPS);

	u64 n = GET_SIZE();
	Particle *field = malloc(n * sizeof(Particle));
	memset(field, 0, n*sizeof(Particle));

	while (!WindowShouldClose()) {
		BeginDrawing();
		if (IsWindowResized()) {
			winWidth  = GetScreenWidth();
			winHeight = GetScreenHeight();

			u64 n = GET_SIZE();
			free(field);
			field = malloc(n * sizeof(Particle));
			memset(field, 0, n*sizeof(Particle));
			ClearBackground(BLACK);
		}
		DrawRectangle(0, 0, winWidth, winHeight, (Color){0, 0, 0, 10});

		for (u32 i = 0; i < n; i++) {
			if (!field[i].lifetime) field[i] = randParticle();
			Vector2 v = func(field[i].x, field[i].y);
			DrawLine(field[i].x, field[i].y, field[i].x + v.x, field[i].y + v.y, WHITE);
			field[i].x += v.x/2.0f;
			field[i].y += v.y/2.0f;
			field[i].lifetime--;
		}

		DrawFPS(10, 10);

		EndDrawing();
	}

	free(field);
	return 0;
}