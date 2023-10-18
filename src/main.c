#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "raylib.h"
#define STB_DS_IMPLEMENTATION
#define GUI_IMPLEMENTATION
#include "util.h"
#include "gui.h"

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define i8  int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

#define E 2.7182818284590452354
#define FPS 60
#define CLAMP(x, min, max) (x) > (max) ? (max) : (x) < (min) ? (min) : (x)

typedef struct {
	float h; // Hue        [0, 1]
	float s; // Saturation [0, 1]
	float l; // Ligthness  [0, 1]
	u8    a; // Alpha - same as in RGBA
} HSLA;

typedef struct {
	float x;
	float y;
	u8 lifetime;
} Particle;

static i32 winWidth  = 1200;
static i32 winHeight =  800;
static u64 n;
static Particle *field;
static Gui_Input_Box inputBox;
static char *defaultFunc = "(vec2 (sin (+ x y)) (cos (* x y)))";

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

Particle randParticle(void)
{
	return (Particle) {
		.x = xorshiftf(0, winWidth),
		.y = xorshiftf(0, winHeight),
		.lifetime = xorshift() % (5*FPS),
	};
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

float lerp(float min, float max, float t)
{
	return min + t*(max - min);
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

typedef enum __attribute__((__packed__)) {
	IR_INST_ROOT, // only for root node
	IR_INST_CONV, // For converting types
	IR_INST_VEC2,
	IR_META_INST_FIRST_CHILDLESS,
	IR_INST_X,
	IR_INST_Y,
	IR_INST_LITERAL,
	IR_META_INST_LAST_CHILDLESS,
	IR_META_INST_FIRST_TRIG,
	IR_INST_SIN,
	IR_INST_COS,
	IR_INST_TAN,
	IR_META_INST_LAST_TRIG,
	IR_META_INST_FIRST_ARITH,
	IR_INST_ADD,
	IR_INST_SUB,
	IR_INST_MUL,
	IR_INST_DIV,
	IR_INST_POW,
	IR_META_INST_LAST_ARITH,
} IR_Inst;

char *instStrs[] = {"IR_INST_ROOT", "IR_INST_CONV", "IR_INST_VEC2", "IR_META_INST_FIRST_CHILDLESS", "IR_INST_X", "IR_INST_Y", "IR_INST_LITERAL", "IR_META_INST_LAST_CHILDLESS", "IR_META_INST_FIRST_TRIG", "IR_INST_SIN", "IR_INST_COS", "IR_INST_TAN", "IR_META_INST_LAST_TRIG", "IR_META_INST_FIRST_ARITH", "IR_INST_ADD", "IR_INST_SUB", "IR_INST_MUL", "IR_INST_DIV", "IR_INST_POW", "IR_META_INST_LAST_ARITH"};

typedef enum __attribute__((__packed__)) {
	IR_TYPE_INT,
	IR_TYPE_FLOAT,
	IR_TYPE_VEC2,
	IR_TYPE_ANY,
} IR_Type;

char *typeStrs[] = {"IR_TYPE_INT", "IR_TYPE_FLOAT", "IR_TYPE_VEC2", "IR_TYPE_ANY"};

typedef union {
	i32     i;
	float   f;
	Vector2 v;
} IR_Val;

typedef struct {
	bool succ;
	IR_Val val;
} IR_Eval_Res;

typedef struct IR {
	IR_Inst inst;
	IR_Type type;
	IR_Val val;
	struct IR *children;  // array of sub IR trees
} IR;

void printIRHelper(IR node, i32 indent)
{
	for (i32 i = 0; i < indent; i++) printf("  ");
	printf("inst: %s, type: %s, val: ", instStrs[node.inst], typeStrs[node.type]);
	switch (node.type) {
		case IR_TYPE_INT:   printf("%d", node.val.i); break;
		case IR_TYPE_FLOAT: printf("%f", node.val.f); break;
		case IR_TYPE_VEC2:  printf("(%f, %f)", node.val.v.x, node.val.v.y); break;
		case IR_TYPE_ANY:   printf("-"); break;
	}
	printf("\n");
	for (i32 i = 0; i < arrlen(node.children); i++)
		printIRHelper(node.children[i], indent + 1);
}

void printIR(IR node)
{
	printIRHelper(node, 0);
}

// void freeIR(IR *ir)
// {
// 	for (i32 i = 0; i < arrlen(ir->children); i++) freeIR(&(ir->children[i]));
// 	free(ir);
// }

bool isAlpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

bool isNum(char c)
{
	return c >= '0' && c <= '9';
}

// Unlike strcmp only `a` needs to be null-terminated
bool strEq(const char *a, const char *b)
{
	while (*a && *a++ == *b++) {}
	return !*a;
}

typedef struct {
	char *msg;
	i32 idx;
} Parse_Err;

// Return value is NULL or an error message
// The parsed IR is written to node
Parse_Err parseExpr(char *text, i32 len, i32 *idx, IR *node, i32 depth)
{
	bool first = true;
	for (char c; *idx < len; *idx += 1) {
		c = text[*idx];
		if (c == ' ' || c == '\t' || c == '\n')
			continue;
		else if (c == '(') {
			*idx += 1;
			if (first) {
				first = false;
				Parse_Err err = parseExpr(text, len, idx, node, 1);
				if (err.msg) return err;
			}
			else {
				IR child = {0};
				Parse_Err err = parseExpr(text, len, idx, &child, 1);
				if (err.msg) return err;
				arrput(node->children, child);
			}
		}
		else if (c == ')') {
			depth--;
			if (depth == 0) break; // Should always be the case
			if (depth < 0)  return (Parse_Err){ .msg = "Too many closing parantheses", .idx = *idx };
		}
		else if (isAlpha(c)) {
			i32 j = 1;
			while ((c = text[*idx + j]) && (isAlpha(c) || isNum(c))) j++;

			IR ir = {0};
			switch (j) {
				case 1:
					if (text[*idx] == 'x')
						ir = (IR) { .inst = IR_INST_X, .type = IR_TYPE_FLOAT, .val = {0}, .children = NULL};
					else if (text[*idx] == 'y')
						ir = (IR) { .inst = IR_INST_Y, .type = IR_TYPE_FLOAT, .val = {0}, .children = NULL};
					else if (text[*idx] == 'e')
						ir = (IR) { .inst = IR_INST_LITERAL, .type = IR_TYPE_FLOAT, .val = { .f = E }, .children = NULL};
					else
						return (Parse_Err){ .msg = "Unknown identifier", .idx = *idx };
					break;
				case 2:
					if (strEq("pi", &text[*idx]))
						ir = (IR) { .inst = IR_INST_LITERAL, .type = IR_TYPE_FLOAT, .val = { .f = PI }, .children = NULL};
					else
						return (Parse_Err){ .msg = "Unknown identifier", .idx = *idx };
					break;
				case 3:
					if (strEq("sin", &text[*idx]))
						ir = (IR) { .inst = IR_INST_SIN, .type = IR_TYPE_FLOAT, .val = {0}, .children = NULL};
					else if (strEq("cos", &text[*idx]))
						ir = (IR) { .inst = IR_INST_COS, .type = IR_TYPE_FLOAT, .val = {0}, .children = NULL};
					else if (strEq("tan", &text[*idx]))
						ir = (IR) { .inst = IR_INST_TAN, .type = IR_TYPE_FLOAT, .val = {0}, .children = NULL};
					else
						return (Parse_Err){ .msg = "Unknown identifier", .idx = *idx };
					break;
				case 4:
					if (strEq("vec4", &text[*idx]))
						ir = (IR) { .inst = IR_INST_VEC2, .type = IR_TYPE_VEC2, .val = {0}, .children = NULL};
					else
						return (Parse_Err){ .msg = "Unknown identifier", .idx = *idx };
					break;
				default:
					return (Parse_Err){ .msg = "Unknown identifier", .idx = *idx };
			}
			if (first) {
				first = false;
				*node = ir;
			}
			else arrput(node->children, ir);

			*idx += j - 1; // -1 because +1 is added at the next iteration again
		}
		else if (isNum(c) || c == '.') {
			IR_Val val = {0};
			bool  isFloat  = false;
			float decDiv   = 0.1f;
			do {
				if (c == '_') {} // Do nothing
				else if (c == '.') {
					if (isFloat) return (Parse_Err){ .msg = "Several dots in a number are not allowed", .idx = *idx };
					isFloat = true;
					val.f = (float) val.i;
				} else if (isFloat) {
					val.f  += decDiv*(c - '0');
					decDiv /= 10.0f;
				} else {
					val.i *= 10;
					val.i += c - '0';
				}
				*idx += 1;
				c = text[*idx];
			} while (isNum(c) || c == '.' || c == '_');
			*idx -= 1; // bc it gets incremented in the next iteration again

			IR ir   = {0};
			ir.inst = IR_INST_LITERAL;
			ir.type = isFloat ? IR_TYPE_FLOAT : IR_TYPE_INT;
			ir.val  = val;
			if (first) {
				first = false;
				*node = ir;
			}
			else arrput(node->children, ir);
		} else {
			IR ir = {0};
			switch (c) {
				case '+':
					ir = (IR) { .inst = IR_INST_ADD, .type = IR_TYPE_ANY, .val = {0}, .children = NULL };
					break;
				case '-':
					ir = (IR) { .inst = IR_INST_SUB, .type = IR_TYPE_ANY, .val = {0}, .children = NULL };
					break;
				case '*': {
					IR_Inst inst = IR_INST_MUL;
					if (text[*idx + 1] == '*') {
						inst = IR_INST_POW;
						*idx += 1;
					}
					ir = (IR) { .inst = inst, .type = IR_TYPE_ANY, .val = {0}, .children = NULL };
					break;
				}
				case '/':
					ir = (IR) { .inst = IR_INST_DIV, .type = IR_TYPE_ANY, .val = {0}, .children = NULL };
					break;
				default:
					return (Parse_Err){ .msg = "Unknown Symbol encountered", .idx = *idx };
			}
			if (first) {
				first = false;
				*node = ir;
			}
			else arrput(node->children, ir);
		}
	}
	if (depth > 1) return (Parse_Err){ .msg = "Unbalanced Parantheses", .idx = *idx };

	return (Parse_Err){0};
}

// Result is written to root
// Output is error message or NULL on success
Parse_Err parseUserFunc(char *text, i32 textlen, IR *root)
{
	root->inst = IR_INST_ROOT;
	root->type = IR_TYPE_VEC2;
	root->val  = (IR_Val) {0};
	root->children = NULL;

	i32 idx = 0;
	while (idx < textlen) {
		IR *node = root;
		Parse_Err err = parseExpr(text, textlen, &idx, node, 0);
		if (err.msg) return err;
	}
	return (Parse_Err){0};
}

void insertConv(IR *node, i32 idx)
{
	IR newNode = {0};
	memcpy(&newNode, &node->children[idx], sizeof(IR));
	node->children[idx].children = NULL;
	arrput(node->children[idx].children, newNode);
	node->children[idx].inst = IR_INST_CONV;
	node->children[idx].type = IR_TYPE_FLOAT;
	node->children[idx].val  = (IR_Val) {0};
}

bool isChildlessInst(IR_Inst inst)
{
	return inst > IR_META_INST_FIRST_CHILDLESS && inst < IR_META_INST_LAST_CHILDLESS;
}

// @TODO: Provide error messages
// @TODO: Type-Checking is not comprehensive yet
bool checkUserFunc(IR *root)
{
	IR_Inst i = root->inst;
	i32 len = arrlen(root->children);
	if (!len) return isChildlessInst(i);
	else if (isChildlessInst(i)) return false;

	for (i32 i = 0; i < len; i++) {
		if (!checkUserFunc(&root->children[i])) return false;
	}

	if (i > IR_META_INST_FIRST_TRIG && i < IR_META_INST_LAST_TRIG) {
		if (len > 1) return false;
		if (root->children[0].type == IR_TYPE_INT) insertConv(root, 0);
		root->type = root->children[0].type;
	}
	else if (i > IR_META_INST_FIRST_ARITH && i < IR_META_INST_LAST_ARITH) {
		root->type = root->children[0].type;
		for (i32 i = 1; i < len; i++) {
			IR_Type t = root->children[i].type;
			if (t == root->type) continue;
			switch (t) {
				case IR_TYPE_INT:
					if (root->type == IR_TYPE_VEC2) return false;
					break;
				case IR_TYPE_FLOAT:
					if (root->type == IR_TYPE_VEC2) return false;
					root->type = t;
					break;
				default:
					return false;
			}
		}

		for (i32 i = 0; i < len; i++) {
			IR_Type t = root->children[i].type;
			switch (t) {
				case IR_TYPE_INT:
					if (root->type == IR_TYPE_FLOAT) insertConv(root, i);
					else if (root->type != t) return false;
					break;
				case IR_TYPE_FLOAT:
					if (root->type != t && root->type != IR_TYPE_INT) return false;
					break;
				case IR_TYPE_VEC2:
					if (root->type != t) return false;
					break;
				default:
					UTIL_UNREACHABLE();
			}
		}
	}

	if (root->inst == IR_INST_ROOT) return root->children[len-1].type == IR_TYPE_VEC2;
	else return true;
}

IR_Eval_Res evalUserFunc(IR node, Vector2 in)
{
	switch (node.inst) {
		case IR_INST_ROOT: {
			IR_Eval_Res res;
			for (i32 i = 0; i < arrlen(node.children); i++) {
				res = evalUserFunc(node.children[i], in);
				if (!res.succ) return res;
			}
			return res;
		}
		case IR_INST_CONV: {
			if (node.type != IR_TYPE_FLOAT) UTIL_TODO();
			if (arrlen(node.children) > 1) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(node.children[0], in);
			if (!res.succ) return res;
			if (node.children[0].type != IR_TYPE_FLOAT) res.val.f = (float) res.val.i;
			return (IR_Eval_Res){ .val = res.val, .succ = true };
		}
		case IR_INST_VEC2: {
			if (arrlen(node.children) != 2) return (IR_Eval_Res){0};
			if (node.children[0].type != IR_TYPE_FLOAT || node.children[1].type != IR_TYPE_FLOAT) return (IR_Eval_Res){0};
			IR_Eval_Res x = evalUserFunc(node.children[0], in);
			IR_Eval_Res y = evalUserFunc(node.children[1], in);
			if (!x.succ || !y.succ) return (IR_Eval_Res){0};
			IR_Val val = { .v = (Vector2){ .x = x.val.f, .y = y.val.f, } };
			return (IR_Eval_Res){ .val = val, .succ = true };
		}
		case IR_INST_X: {
			return (IR_Eval_Res){ .val = (IR_Val){.f = in.x}, .succ = true };
		}
		case IR_INST_Y: {
			return (IR_Eval_Res){ .val = (IR_Val){.f = in.y}, .succ = true };
		}
		case IR_INST_LITERAL: {
			return (IR_Eval_Res){ .val = node.val, .succ = true };
		}
		case IR_INST_SIN: {
			if (arrlen(node.children) != 1) return (IR_Eval_Res){0};
			if (node.children[0].type != IR_TYPE_FLOAT) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(node.children[0], in);
			if (!res.succ) return res;
			return (IR_Eval_Res) { .val = (IR_Val){.f = sinf(res.val.f)}, .succ = true };
		}
		case IR_INST_COS: {
			if (arrlen(node.children) != 1) return (IR_Eval_Res){0};
			if (node.children[0].type != IR_TYPE_FLOAT) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(node.children[0], in);
			if (!res.succ) return res;
			return (IR_Eval_Res) { .val = (IR_Val){.f = cosf(res.val.f)}, .succ = true };
		}
		case IR_INST_TAN: {
			if (arrlen(node.children) != 1) return (IR_Eval_Res){0};
			if (node.children[0].type != IR_TYPE_FLOAT) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(node.children[0], in);
			if (!res.succ) return res;
			return (IR_Eval_Res) { .val = (IR_Val){.f = tanf(res.val.f)}, .succ = true };
		}
		case IR_INST_ADD: {
			if (!arrlen(node.children)) return (IR_Eval_Res){0};
			IR_Val out = {0};
			for (i32 i = 0; i < arrlen(node.children); i++) {
				IR_Eval_Res res = evalUserFunc(node.children[i], in);
				if (!res.succ) return res;
				switch (node.type) {
					case IR_TYPE_INT:   out.i += res.val.i; break;
					case IR_TYPE_FLOAT: out.f += res.val.f; break;
					case IR_TYPE_VEC2:  out.v = addVector2(out.v, res.val.v); break;
					default:
						return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_SUB: {
			if (!arrlen(node.children)) return (IR_Eval_Res){0};
			i32 i = 0;
			IR_Val out;
			if (arrlen(node.children) == 1) {
				switch (node.type) {
					case IR_TYPE_INT:   out = (IR_Val){ .i = 0 };            break;
					case IR_TYPE_FLOAT: out = (IR_Val){ .f = 0.0f };         break;
					case IR_TYPE_VEC2:  out = (IR_Val){ .v = (Vector2){0} }; break;
					default: return (IR_Eval_Res){0};
				}
			} else {
				IR_Eval_Res res = evalUserFunc(node.children[0], in);
				if (!res.succ) return res;
				out = res.val;
				i   = 1;
			}
			for (; i < arrlen(node.children); i++) {
				IR_Eval_Res res = evalUserFunc(node.children[i], in);
				if (!res.succ) return res;
				switch (node.type) {
					case IR_TYPE_INT:   out.i -= res.val.i;                    break;
					case IR_TYPE_FLOAT: out.f -= res.val.f;                    break;
					case IR_TYPE_VEC2:  out.v  = subVector2(out.v, res.val.v); break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_MUL: {
			if (!arrlen(node.children)) return (IR_Eval_Res){0};
			IR_Val out;
			switch (node.type) {
				case IR_TYPE_INT:   out = (IR_Val){ .i = 1 };                     break;
				case IR_TYPE_FLOAT: out = (IR_Val){ .f = 1.0f };                  break;
				case IR_TYPE_VEC2:  out = (IR_Val){ .v = (Vector2){1.0f, 1.0f} }; break;
				default: return (IR_Eval_Res){0};
			}
			for (i32 i = 0; i < arrlen(node.children); i++) {
				IR_Eval_Res res = evalUserFunc(node.children[i], in);
				if (!res.succ) return res;
				switch (node.type) {
					case IR_TYPE_INT:   out.i *= res.val.i; break;
					case IR_TYPE_FLOAT: out.f *= res.val.f; break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_DIV: {
			if (!arrlen(node.children)) return (IR_Eval_Res){0};
			i32 i = 0;
			IR_Val out;
			if (arrlen(node.children) == 1) {
				switch (node.type) {
					case IR_TYPE_INT:   out = (IR_Val){ .i = 1 };    break;
					case IR_TYPE_FLOAT: out = (IR_Val){ .f = 1.0f }; break;
					default:  return (IR_Eval_Res){0};
				}
			} else {
				IR_Eval_Res res = evalUserFunc(node.children[0], in);
				if (!res.succ) return res;
				out = res.val;
				i   = 1;
			}
			for (; i < arrlen(node.children); i++) {
				IR_Eval_Res res = evalUserFunc(node.children[i], in);
				if (!res.succ) return res;
				switch (node.type) {
					case IR_TYPE_INT:   out.i /= res.val.i; break;
					case IR_TYPE_FLOAT: out.f /= res.val.f; break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_POW: {
			if (arrlen(node.children) < 2) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(node.children[0], in);
			if (!res.succ) return res;
			IR_Val out = res.val;
			for (i32 i = 1; i < arrlen(node.children); i++) {
				res = evalUserFunc(node.children[i], in);
				if (!res.succ) return res;
				switch (node.type) {
					case IR_TYPE_INT:   out.i = powi(out.i, res.val.i); break;
					case IR_TYPE_FLOAT: out.f = powf(out.f, res.val.f); break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		default:
			return (IR_Eval_Res){0};
	}
}

void onResize(void) {
	n = winWidth * winHeight / 100;
	field = malloc(n * sizeof(Particle));
	memset(field, 0, n*sizeof(Particle));
}

int main(void)
{
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
	parseUserFunc(inputBox.label.text, arrlen(inputBox.label.text) - 1, &root);
	checkUserFunc(&root);

	SetGesturesEnabled(GESTURE_PINCH_IN | GESTURE_PINCH_OUT);
	float zoomFactor = 10.0f;

	onResize();
	while (!WindowShouldClose()) {
		BeginDrawing();
		if (IsWindowResized()) {
			winWidth  = GetScreenWidth();
			winHeight = GetScreenHeight();
			free(field);
			onResize();
			ClearBackground(BLACK);
		}
		DrawRectangle(0, 0, winWidth, winHeight, (Color){0, 0, 0, 10});

		for (u32 i = 0; i < n; i++) {
			if (!field[i].lifetime) field[i] = randParticle();
			// Normalize field value
			Vector2 in = {
				.x = 2*zoomFactor*field[i].x/winWidth  - zoomFactor,
				.y = 2*zoomFactor*field[i].y/winHeight - zoomFactor,
			};
			IR_Eval_Res res = evalUserFunc(root, in);
			if (!res.succ) break;
			Vector2 v = res.val.v;
			float len = lenVector2(v);
			HSLA hsl = {
				lerp(0.0f, 120.0f/360.0f, CLAMP(1 - len, 0, 1)),
				lerp(0.5f, 1.0f, CLAMP(len, 0, 1)),
				0.5f,
				255
			};
			DrawLine(field[i].x, field[i].y, field[i].x + v.x, field[i].y + v.y, hslToRGB(hsl));
			field[i].x += v.x/2.0f;
			field[i].y += v.y/2.0f;
			field[i].lifetime--;
		}

		float wheelVelocity = GetMouseWheelMove();
		if (wheelVelocity == 0.0f) wheelVelocity = lenVector2(GetGesturePinchVector());
		if (wheelVelocity) {
			zoomFactor -= 0.3f * wheelVelocity;
			zoomFactor = CLAMP(zoomFactor, 0.01f, 1000.0f);
		}

		inputBox.label.bounds.width = winWidth;
		inputBox.label.bounds.width = winHeight;
		bool updated = gui_drawInputBox(&inputBox).updated;

		// @TODO: Show error messages to user
		if (updated) {
			updatedRoot = (IR) {0};
			Parse_Err err = parseUserFunc(inputBox.label.text, arrlen(inputBox.label.text) - 1, &updatedRoot);
			if (err.msg) {
				printf("Error in parsing at index %d: '%s'\n", err.idx, err.msg);
			} else if (checkUserFunc(&updatedRoot)) {
				root = updatedRoot;
			} else {
				printf("Error in type checking\n");
			}
		}

		EndDrawing();
	}

	free(field);
	return 0;
}