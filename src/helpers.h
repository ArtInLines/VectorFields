#ifndef _HELPERS_H_
#define _HELPERS_H_

#define  AIL_ALL_IMPL
#include <math.h>
#include "raylib.h"
#include "ail.h"

u32 xorshift();
float xorshiftf(float min, float max);
Vector2 addVector2(Vector2 a, Vector2 b);
Vector2 subVector2(Vector2 a, Vector2 b);
Vector2 modVector2(Vector2 a, Vector2 b);
float lenVector2(Vector2 v);
i32 powi(i32 a, i32 b);

#endif // _HELPERS_H_