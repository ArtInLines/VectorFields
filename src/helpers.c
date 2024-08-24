#include "helpers.h"

u32 xorshift(void)
{
	static u32 RNGState = 69;
	RNGState ^= RNGState << 13;
	RNGState ^= RNGState >> 17;
	RNGState ^= RNGState << 5;
	return RNGState;
}

float xorshiftf(float min, float max)
{
	return min + (max - min)*((float)(xorshift() % UINT32_MAX)/(float)UINT32_MAX);
}

Vector2 addVector2(Vector2 a, Vector2 b)
{
	return (Vector2) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}

Vector2 subVector2(Vector2 a, Vector2 b)
{
	return (Vector2) {
		.x = a.x - b.x,
		.y = a.y - b.y,
	};
}

// @Study: Is this really the right semantic for modVector?
Vector2 modVector2(Vector2 a, Vector2 b)
{
    return (Vector2) {
        .x = fmodf(a.x, b.x),
        .y = fmodf(a.y, b.y),
    };
}

float lenVector2(Vector2 v)
{
	return sqrtf(v.x*v.x + v.y*v.y);
}

i32 powi(i32 a, i32 b)
{
	if (b < 0) return 0; // floor the number, bc return type is int
	i32 res = 1;
	for (i32 i = 0; i < b; i++)
		res *= a;
	return res;
}
