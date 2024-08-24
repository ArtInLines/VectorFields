#ifndef _IR_H_
#define _IR_H_

#define  AIL_ALL_IMPL
#include "ail.h"
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "helpers.h"

#define E 2.7182818284590452354
#define MAX_RAND_DEPTH 6

// @Note: Keep instStrs in ir.c up to date with this enum
typedef enum __attribute__((__packed__)) {
	IR_INST_ROOT = 0, // only for root node
	IR_META_INST_FIRST_CHILDLESS,
	IR_INST_X,
	IR_INST_Y,
	IR_INST_XN,
	IR_INST_YN,
	IR_INST_LITERAL,
	IR_META_INST_LAST_CHILDLESS,
	IR_META_INST_FIRST_UNARY,
	IR_INST_CONV, // For converting types
	IR_INST_ABS,
	IR_INST_SQRT,
	IR_INST_LOG,
	IR_META_INST_FIRST_TRIG,
	IR_INST_SIN,
	IR_INST_COS,
	IR_INST_TAN,
	IR_META_INST_LAST_TRIG,
	IR_META_INST_LAST_UNARY,
	IR_META_INST_FIRST_BINARY,
	IR_INST_VEC2,
	IR_INST_MAX,
	IR_INST_MIN,
	IR_META_INST_LAST_BINARY,
	IR_META_INST_FIRST_TERTIARY,
	IR_INST_CLAMP,
	IR_INST_LERP,
	IR_META_INST_LAST_TERTIARY,
	IR_META_INST_FIRST_LASSOC,
	IR_INST_ADD,
	IR_INST_SUB,
	IR_INST_MUL,
	IR_INST_DIV,
	IR_INST_MOD,
	IR_META_INST_LAST_LASSOC,
	IR_META_INST_FIRST_RASSOC,
	IR_INST_POW,
	IR_META_INST_LAST_RASSOC,
	IR_META_INST_LEN,
} IR_Inst;

// @Note: Keep typeStrs in ir.c up to date with this enum
typedef enum __attribute__((__packed__)) {
	IR_TYPE_INT = 0,
	IR_TYPE_FLOAT,
	IR_TYPE_VEC2,
	IR_TYPE_ANY,
	IR_TYPE_LEN,
} IR_Type;

typedef union {
	i32     i;
	float   f;
	Vector2 v;
} IR_Val;

typedef struct {
	bool succ;
	IR_Val val;
} IR_Eval_Res;

typedef struct IR IR;
AIL_DA_INIT(IR);
struct IR {
	IR_Inst inst;
	IR_Type type;
	IR_Val val;
	AIL_DA(IR) children;  // array of sub IR trees
};

typedef struct {
	char *msg;
	i32 idx;
} Parse_Err;

#define MAX_NAMED_TOK_SZ 8 // 8 -> nice alignment for IR_NAMED_TOK_MAP
typedef struct {
	const char s[MAX_NAMED_TOK_SZ];
	IR   ir;
} IR_NAMED_TOK_MAP; // Map for tokens that are simple strings

#define NAMED_TOK_MAP { \
	(IR_NAMED_TOK_MAP){.s = "xn",    .ir = (IR){.inst = IR_INST_XN,      .type = IR_TYPE_FLOAT, .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "yn",    .ir = (IR){.inst = IR_INST_YN,      .type = IR_TYPE_FLOAT, .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "x",     .ir = (IR){.inst = IR_INST_X,       .type = IR_TYPE_FLOAT, .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "y",     .ir = (IR){.inst = IR_INST_Y,       .type = IR_TYPE_FLOAT, .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "e",     .ir = (IR){.inst = IR_INST_LITERAL, .type = IR_TYPE_FLOAT, .val = {.f = E},  .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "pi",    .ir = (IR){.inst = IR_INST_LITERAL, .type = IR_TYPE_FLOAT, .val = {.f = PI}, .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "**",    .ir = (IR){.inst = IR_INST_POW,     .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "*",     .ir = (IR){.inst = IR_INST_MUL,     .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "/",     .ir = (IR){.inst = IR_INST_DIV,     .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "+",     .ir = (IR){.inst = IR_INST_ADD,     .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "-",     .ir = (IR){.inst = IR_INST_SUB,     .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "%",     .ir = (IR){.inst = IR_INST_MOD,     .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "sqrt",  .ir = (IR){.inst = IR_INST_SQRT,    .type = IR_TYPE_FLOAT, .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "log",   .ir = (IR){.inst = IR_INST_LOG,     .type = IR_TYPE_FLOAT, .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "sin",   .ir = (IR){.inst = IR_INST_SIN,     .type = IR_TYPE_FLOAT, .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "cos",   .ir = (IR){.inst = IR_INST_COS,     .type = IR_TYPE_FLOAT, .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "tan",   .ir = (IR){.inst = IR_INST_TAN,     .type = IR_TYPE_FLOAT, .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "abs",   .ir = (IR){.inst = IR_INST_ABS,     .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "lerp",  .ir = (IR){.inst = IR_INST_LERP,    .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "clamp", .ir = (IR){.inst = IR_INST_CLAMP,   .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "max",   .ir = (IR){.inst = IR_INST_MAX,     .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "min",   .ir = (IR){.inst = IR_INST_MIN,     .type = IR_TYPE_ANY,   .val = {0},       .children = ail_da_new_empty(IR)}}, \
	(IR_NAMED_TOK_MAP){.s = "vec2",  .ir = (IR){.inst = IR_INST_VEC2,    .type = IR_TYPE_VEC2,  .val = {0},       .children = ail_da_new_empty(IR)}}, \
}
// @Cleanup: This is rly messy tbh
#define RAND_PREFERED_NAMED_TOK_MAP_MIN 0
#define RAND_PREFERED_NAMED_TOK_MAP_LEN 17

#define RAND_LITERALS { \
    IR_INST_X,       \
	IR_INST_Y,       \
	IR_INST_LITERAL, \
	IR_INST_LITERAL, \
	IR_INST_XN,      \
	IR_INST_YN,      \
}

void printIR(IR node);
bool isAlpha(char c);
bool isNum(char c);
bool isOp(char c);
bool strEq(const char *a, const char *b);
Parse_Err parseExpr(char *text, i32 len, i32 *idx, IR *node, i32 depth);
Parse_Err parseUserFunc(char *text, i32 textlen, IR *root);
void insertConv(IR *node, i32 idx);
i32 getExpectedChildAmount(IR_Inst inst);
bool checkUserFunc(IR *root);
IR_Eval_Res evalUserFunc(IR node, Vector2 in);
IR randFunction(void);
AIL_DA(char) irToStr(IR node);

#endif // _IR_H_
