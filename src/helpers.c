#include "helpers.h"

u32 xorshift()
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

Color hslToRGB(HSLA col)
{
	//  Algorithm adapted from Wikipedia: https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB
	float r1, g1, b1;
	float chroma = (1.0f - fabsf(2.0f*col.l - 1.0f)) * col.s;
	float hPrime = col.h*360.0f / 60.0f;
	float x      = chroma * (1 - fabsf(fmodf(hPrime, 2) - 1));

	if (hPrime < 1.0f) {
		r1 = chroma;
		g1 = x;
		b1 = 0.0f;
	} else if (hPrime < 2.0f) {
		r1 = x;
		g1 = chroma;
		b1 = 0.0f;
	} else if (hPrime < 3.0f) {
		r1 = 0.0f;
		g1 = chroma;
		b1 = x;
	} else if (hPrime < 4.0f) {
		r1 = 0.0f;
		g1 = x;
		b1 = chroma;
	} else if (hPrime < 5.0f) {
		r1 = x;
		g1 = 0.0f;
		b1 = chroma;
	} else {
		r1 = chroma;
		g1 = 0.0f;
		b1 = x;
	}

	float m = col.l - chroma/2.0f;
	u8 r = roundf(255.0f*(r1 + m));
	u8 g = roundf(255.0f*(g1 + m));
	u8 b = roundf(255.0f*(b1 + m));
	return (Color) { r, g, b, col.a };
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