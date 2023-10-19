#ifndef _HELPERS_H_
#define _HELPERS_H_

#include <math.h>
#include "raylib.h"
#include "util.h"

typedef struct {
	float h; // Hue        [0, 1]
	float s; // Saturation [0, 1]
	float l; // Ligthness  [0, 1]
	u8    a; // Alpha - same as in RGBA
} HSLA;

u32 xorshift();
float xorshiftf(float min, float max);
Color hslToRGB(HSLA col);
Vector2 addVector2(Vector2 a, Vector2 b);
Vector2 subVector2(Vector2 a, Vector2 b);
float lenVector2(Vector2 v);
i32 powi(i32 a, i32 b);

#endif // _HELPERS_H_