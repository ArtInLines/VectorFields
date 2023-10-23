#ifndef _HELPERS_H_
#define _HELPERS_H_

#define  AIL_ALL_IMPL
#include <math.h>
#include "raylib.h"
#include "ail.h"

typedef struct {
	float h; // Hue        [0, 1]
	float s; // Saturation [0, 1]
	float l; // Ligthness  [0, 1]
} HSLA;

u32 xorshift();
float xorshiftf(float min, float max);
Vector2 addVector2(Vector2 a, Vector2 b);
Vector2 subVector2(Vector2 a, Vector2 b);
float lenVector2(Vector2 v);
i32 powi(i32 a, i32 b);

#endif // _HELPERS_H_