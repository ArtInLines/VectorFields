#ifndef _IR_H_
#define _IR_H_

#define  AIL_ALL_IMPL
#define  AIL_RING_IMPL
#define  AIL_SV_IMPL
#include "ail.h"
#include "ail_ring.h"
#include "ail_sv.h"
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "helpers.h"

#define E 2.7182818284590452354
#define MAX_RAND_DEPTH 6

typedef struct {
    float x;
    float y;
    int   t;
} Input;

typedef enum {
	IR_TYPE_POS = 0, // Natural numbers without 0
	IR_TYPE_NAT,     // Natural numbers including 0
	IR_TYPE_REAL,    // Real / floating-point numbers
	IR_TYPE_VEC2,    // 2D vector of reals
	IR_TYPE_LEN,     // Metadata containing amount of available types
} IR_Type;

typedef enum {
	IR_TOK_NONE, // used to indicate that all tokens have been exhausted
	IR_TOK_ID,   // Identifier
	IR_TOK_NAT,  // Natural number literal
	IR_TOK_REAL, // Real / floating-point number literal
	IR_TOK_POW,
	IR_TOK_OPEN_PARAN   = '(',
	IR_TOK_CLOSED_PARAN = ')',
	IR_TOK_COMMA = ',',
	IR_TOK_ADD   = '+',
	IR_TOK_SUB   = '-',
	IR_TOK_MUL   = '*',
	IR_TOK_DIV   = '/',
	IR_TOK_MOD   = '%',
} IR_Token_Type;

typedef struct {
	IR_Token_Type type;
	union {
		AIL_SV sv;
		u64 nat;
		f64 real;
	} val;
} IR_Token;

typedef enum {
	IR_INST_ROOT = 0, // only for root node
	IR_META_INST_FIRST_CHILDLESS,
	IR_INST_T,
	IR_INST_X,
	IR_INST_Y,
	IR_INST_XN,
	IR_INST_YN,
	IR_INST_LITERAL,
	IR_META_INST_LAST_CHILDLESS,
	IR_META_INST_FIRST_UNARY,
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

#define INSTRUCTIONS                          \
	X(IR_INST_T,       "t",     IR_TYPE_POS)  \
	X(IR_INST_X,       "x",     IR_TYPE_REAL) \
	X(IR_INST_Y,       "y",     IR_TYPE_REAL) \
	X(IR_INST_XN,      "xn",    IR_TYPE_NAT)  \
	X(IR_INST_YN,      "yn",    IR_TYPE_NAT)  \
	X(IR_INST_LITERAL, "e",     IR_TYPE_POS)  \
	X(IR_INST_LITERAL, "pi",    IR_TYPE_POS)  \
	X(IR_INST_ABS,     "abs",   IR_TYPE_NAT)  \
	X(IR_INST_SQRT,    "sqrt",  IR_TYPE_NAT)  \
	X(IR_INST_LOG,     "log",   IR_TYPE_REAL) \
	X(IR_INST_SIN,     "sin",   IR_TYPE_REAL) \
	X(IR_INST_COS,     "cos",   IR_TYPE_REAL) \
	X(IR_INST_TAN,     "tan",   IR_TYPE_REAL) \
	X(IR_INST_VEC2,    "vec2",  IR_TYPE_VEC2) \
	X(IR_INST_MAX,     "max",   IR_TYPE_REAL) \
	X(IR_INST_MIN,     "min",   IR_TYPE_REAL) \
	X(IR_INST_CLAMP,   "clamp", IR_TYPE_REAL) \
	X(IR_INST_LERP,    "lerp",  IR_TYPE_REAL) \
	X(IR_INST_ADD,     "+",     IR_TYPE_REAL) \
	X(IR_INST_SUB,     "-",     IR_TYPE_REAL) \
	X(IR_INST_MUL,     "*",     IR_TYPE_REAL) \
	X(IR_INST_DIV,     "/",     IR_TYPE_REAL) \
	X(IR_INST_MOD,     "%",     IR_TYPE_REAL) \
	X(IR_INST_POW,     "**",    IR_TYPE_REAL)

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
} Parse_Err;

#define MAX_NAMED_TOK_SZ 8 // 8 -> nice alignment for IR_NAMED_TOK_MAP
typedef struct {
	const char s[MAX_NAMED_TOK_SZ];
	IR   ir;
} IR_NAMED_TOK_MAP; // Map for tokens that are simple strings

// void printIR(IR node);
bool isAlpha(char c);
bool isNum(char c);
bool isOp(char c);
bool strEq(const char *a, const char *b);
Parse_Err parseExpr(AIL_SV *text, IR *node, i32 depth);
Parse_Err parseUserFunc(AIL_SV *text, IR *root);
i32 getExpectedChildAmount(IR_Inst inst);
bool checkUserFunc(IR *root);
IR_Eval_Res evalUserFunc(IR node, Input in, IR_Type desired_type);
IR randFunction();
AIL_DA(char) irToStr(IR node);

#endif // _IR_H_