#include "ir.h"

// void printIRHelper(IR node, i32 indent)
// {
// 	for (i32 i = 0; i < indent; i++) printf("  ");
// 	printf("inst: %s, type: %s, val: ", instStrs[node.inst], typeStrs[node.type]);
// 	AIL_STATIC_ASSERT(IR_TYPE_LEN == 4);
// 	switch (node.type) {
// 		case IR_TYPE_INT:   printf("%d", node.val.i); break;
// 		case IR_TYPE_FLOAT: printf("%f", node.val.f); break;
// 		case IR_TYPE_VEC2:  printf("(%f, %f)", node.val.v.x, node.val.v.y); break;
// 		case IR_TYPE_ANY:   printf("-"); break;
// 		default:            AIL_UNREACHABLE();
// 	}
// 	printf("\n");
// 	for (u32 i = 0; i < node.children.len; i++)
// 		printIRHelper(((IR *)node.children.data)[i], indent + 1);
// }

// void printIR(IR node)
// {
// 	printIRHelper(node, 0);
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
	return !*a && *(a-1) == *(b-1);
}

static u8 token_buffer_data[sizeof(IR_Token) * 4];
static AIL_RingBuffer token_buffer = {
	.data = token_buffer_data,
	.buffer_size = AIL_ARRLEN(token_buffer_data),
};

AIL_SV tokenize(AIL_SV sv)
{
	sv = ail_sv_ltrim(sv);
	IR_Token tok;
	tok.val.sv = ail_sv_from_parts(sv.str, 1);
	if (!sv.len) {
		tok.type = IR_TOK_NONE;
	} else {
		// Asserts exist, bc we assume that only '+', '-', '*', '/' and '**' exist as operators
		AIL_STATIC_ASSERT(IR_META_INST_LAST_LASSOC - IR_META_INST_FIRST_LASSOC - 1 == 5);
		AIL_STATIC_ASSERT(IR_META_INST_LAST_RASSOC - IR_META_INST_FIRST_RASSOC - 1 == 1);
		switch (sv.str[0]) {
			case '(': tok.type = IR_TOK_OPEN_PARAN;   break;
			case ')': tok.type = IR_TOK_CLOSED_PARAN; break;
			case ',': tok.type = IR_TOK_COMMA; break;
			case '+': tok.type = IR_TOK_ADD;   break;
			case '-': tok.type = IR_TOK_SUB;   break;
			case '/': tok.type = IR_TOK_DIV;   break;
			case '%': tok.type = IR_TOK_MOD;   break;
			case '*': {
				if (sv.len < 2 || sv.str[1] != '*') tok.type = IR_TOK_MUL;
				else tok.type = IR_TOK_POW;
			} break;
			default: {
				if (isAlpha(sv.str[0])) {
					u32 n = 1;
					while (isAlpha(n < sv.len && isAlpha(sv.str[n]))) n++;
					tok.type = IR_TOK_ID;
					tok.val.sv.len = n;
					sv = ail_sv_offset(sv, n);
				} else if (isNum(sv.str[0])) {
					u32 n;
					u64 x = ail_sv_parse_unsigned(sv, &n);
					AIL_ASSERT(n != 0);
					if (sv.len > n && sv.str[n] == '.') {
						tok.type     = IR_TOK_REAL;
						tok.val.real = ail_sv_parse_float(sv, &n);
					} else {
						tok.type    = IR_TOK_NAT;
						tok.val.nat = x;
					}
					sv = ail_sv_offset(sv, n);
				} else {
					AIL_UNREACHABLE();
					tok.type = IR_TOK_NONE;
				}
			}
		}
	}
	ail_ring_writen(&token_buffer, sizeof(IR_Token), &tok);
	return sv;
}

IR_Token popToken(AIL_SV *text)
{
	if (!ail_ring_len(token_buffer)) *text = tokenize(*text);
	IR_Token tok;
	ail_ring_readn(&token_buffer, sizeof(IR_Token), &tok);
	return tok;
}

IR_Token peekToken(AIL_SV *text)
{
	if (!ail_ring_len(token_buffer)) *text = tokenize(*text);
	IR_Token tok;
	ail_ring_peekn(token_buffer, sizeof(IR_Token), &tok);
	return tok;

}

// Return value is NULL or an error message
// The parsed IR is written to node
Parse_Err parseExpr(AIL_SV *text, IR *node, i32 depth)
{
	Parse_Err res = {0};
	IR_Token cur_tok = popToken(text);
	switch (cur_tok.type) {
		case IR_TOK_NONE: {
			if (depth) res.msg = "Unclosed Parantheses";
			return res;
		} break;
		case IR_TOK_ID: {

		} break;
		case IR_TOK_NAT: {

		} break;
		case IR_TOK_REAL: {

		} break;
		case IR_TOK_POW: {

		} break;
		case IR_TOK_OPEN_PARAN: {

		} break;
		case IR_TOK_CLOSED_PARAN: {

		} break;
		case IR_TOK_COMMA: {

		} break;
		case IR_TOK_ADD: {

		} break;
		case IR_TOK_SUB: {

		} break;
		case IR_TOK_MUL: {

		} break;
		case IR_TOK_DIV: {

		} break;
		case IR_TOK_MOD: {

		} break;
	}

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
				ail_da_push(&node->children, child);
			}
		}
		else if (c == ')') {
			depth--;
			if (depth == 0) break; // Should always be the case
			if (depth < 0)  return (Parse_Err){ .msg = "Too many closing parantheses", .idx = *idx };
		}
		else if (isAlpha(c) || isOp(c)) {
			i32 j = 1;
			while ((c = text[*idx + j]) && (isAlpha(c) || isOp(c) || isNum(c))) j++;

			IR ir = {0};
			bool foundIR = false;
			#define X(instr, str, inst_type) if (!foundIR && strEq(str, &text[*idx])) { foundIR = true; ir = (IR){ .inst = instr, .type = inst_type, .val = (IR_Val){0}, .children = ail_da_new_with_cap(IR, 16)}; }
				INSTRUCTIONS
			#undef X
			if (!foundIR) {
				return (Parse_Err){ .msg = "Unknown identifier", .idx = *idx };
			}
			if (first) {
				first = false;
				*node = ir;
			}
			else ail_da_push(&node->children, ir);

			*idx += j - 1; // -1 because +1 is added at the next iteration again
		}
		else if (isNum(c) || c == '.') {
			IR_Val val = {0};
			float decDiv = 0.1f;
			bool isFloat = false;
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
			ir.type = IR_TYPE_REAL;
			ir.val  = val;
			if (first) {
				first = false;
				*node = ir;
			}
			else ail_da_push(&node->children, ir);
		} else {
			return (Parse_Err){ .msg = "Unexpected character", .idx = *idx };
		}
	}
	if (depth > 1) return (Parse_Err){ .msg = "Unbalanced Parantheses", .idx = *idx };

	return (Parse_Err){0};
}

// Result is written to root
// Output is error message or NULL on success
Parse_Err parseUserFunc(AIL_SV *text, IR *root)
{
	root->inst = IR_INST_ROOT;
	root->type = IR_TYPE_VEC2;
	root->val  = (IR_Val) {0};
	root->children = ail_da_new_empty(IR);

	while (text->len) {
		IR *node = root;
		Parse_Err err = parseExpr(text, node, 0);
		if (err.msg) return err;
	}
	return (Parse_Err){0};
}

i32 getExpectedChildAmount(IR_Inst inst)
{
	AIL_STATIC_ASSERT(IR_META_INST_LEN == 38);
	AIL_STATIC_ASSERT(IR_META_INST_LAST_CHILDLESS < IR_META_INST_LAST_TRIG);
	AIL_STATIC_ASSERT(IR_META_INST_LAST_TRIG < IR_META_INST_LAST_UNARY);
	AIL_STATIC_ASSERT(IR_META_INST_LAST_UNARY < IR_META_INST_LAST_BINARY);
	AIL_STATIC_ASSERT(IR_META_INST_LAST_BINARY < IR_META_INST_LAST_TERTIARY);
	AIL_STATIC_ASSERT(IR_META_INST_LAST_TERTIARY < IR_META_INST_LAST_LASSOC);
	AIL_STATIC_ASSERT(IR_META_INST_LAST_LASSOC < IR_META_INST_LAST_RASSOC);

	if      (inst < IR_META_INST_FIRST_CHILDLESS) return -1;
	else if (inst < IR_META_INST_LAST_CHILDLESS)  return 0;
	else if (inst < IR_META_INST_LAST_UNARY)      return 1;
	else if (inst < IR_META_INST_LAST_BINARY)     return 2;
	else if (inst < IR_META_INST_LAST_TERTIARY)   return 3;
	else 									      return -1;
}

// @TODO: Provide error messages
bool checkUserFunc(IR *root)
{
	AIL_STATIC_ASSERT(IR_META_INST_LEN == 38);

	IR_Inst inst    = root->inst;
	i32 expectedLen = getExpectedChildAmount(inst);
	i32 len         = root->children.len;

	if (expectedLen >= 0 && len != expectedLen) return false;
	if (expectedLen <  0 && len == 0)           return false;
	for (i32 inst = 0; inst < len; inst++) if (!checkUserFunc(&((IR *)root->children.data)[inst])) return false;
	if (inst == IR_INST_ROOT) return ((IR *)root->children.data)[len-1].type == IR_TYPE_VEC2;

	AIL_STATIC_ASSERT(IR_TYPE_LEN == 4);
	if (len >= 1) {
		for (i32 i = 0; i < len; i++) {
			IR_Type t = ((IR *)root->children.data)[i].type;
			if (t == IR_TYPE_VEC2) return false;
		}
	}
	return true;
}

#define GET_NODE_VALUE(node, t) (((node).type == IR_TYPE_REAL) ? (t)(node).val.f : (t)(node).val.i)

IR_Eval_Res valToDesiredType(IR node, IR_Type desired_type) {
	IR_Eval_Res res;
	res.succ = true;
	switch (desired_type) {
		case IR_TYPE_VEC2:
			if (node.type != IR_TYPE_VEC2) {
				float x = GET_NODE_VALUE(node, float);
				res.val.v = (Vector2){ .x = x, .y = x };
			}
			break;
		case IR_TYPE_REAL: res.val.f = GET_NODE_VALUE(node, float); break;
		case IR_TYPE_POS:
		case IR_TYPE_NAT:  res.val.i = GET_NODE_VALUE(node, int); break;
		case IR_TYPE_LEN:  res.succ = false;
	}
	return res;
}

IR_Eval_Res evalInt(int x, IR_Type desired_type)
{
	IR_Val val;
	switch (desired_type) {
		case IR_TYPE_REAL: val.f = (float)x; break;
		case IR_TYPE_VEC2: val.v = (Vector2){ .x = (float)x, .y = (float)x }; break;
		default: val.i = x;
	}
	return (IR_Eval_Res){ .val = val, .succ = true };
}

IR_Eval_Res evalFloat(float x, IR_Type desired_type)
{
	IR_Val val;
	switch (desired_type) {
		case IR_TYPE_REAL: val.f = x; break;
		case IR_TYPE_VEC2: val.v = (Vector2){ .x = (float)x, .y = (float)x }; break;
		default: val.i = (int)x;
	}
	return (IR_Eval_Res){ .val = val, .succ = true };
}

#define EVAL_REAL_UNARY_FUNC(func) {                                             \
		if (node.children.len != 1) return (IR_Eval_Res){0};                     \
		IR_Eval_Res res = evalUserFunc(node.children.data[0], in, IR_TYPE_REAL); \
		if (!res.succ) return res;                                               \
		else return evalFloat(func(res.val.f), desired_type);                    \
	}

IR_Eval_Res evalUserFunc(IR node, Input in, IR_Type desired_type)
{
	AIL_STATIC_ASSERT(IR_META_INST_LEN == 38);
	switch (node.inst) {
		case IR_INST_ROOT: {
			IR_Eval_Res res;
			for (u32 i = 0; i < node.children.len; i++) {
				res = evalUserFunc(node.children.data[i], in, desired_type);
				if (!res.succ) return res;
			}
			return res;
		}
		case IR_INST_VEC2: {
			if (node.children.len != 2) return (IR_Eval_Res){0};
			IR_Eval_Res x = evalUserFunc(node.children.data[0], in, IR_TYPE_REAL);
			IR_Eval_Res y = evalUserFunc(node.children.data[1], in, IR_TYPE_REAL);
			if (!x.succ || !y.succ) return (IR_Eval_Res){0};
			IR_Val val = { .v = (Vector2){ .x = x.val.f, .y = y.val.f, } };
			return (IR_Eval_Res){ .val = val, .succ = true };
		}
		case IR_INST_T: return evalInt(in.t, desired_type);
		case IR_INST_X: return evalFloat(in.x, desired_type);
		case IR_INST_Y: return evalFloat(in.y, desired_type);
		case IR_INST_XN: return evalFloat(fabsf(in.x), desired_type);
		case IR_INST_YN: return evalFloat(fabsf(in.y), desired_type);
		case IR_INST_LITERAL: {
			if (node.type == IR_TYPE_REAL) return evalFloat(node.val.f, desired_type);
			else return evalInt(node.val.i, desired_type);
		}
		case IR_INST_ABS: {
			IR_Type type = node.children.data[0].type;
			IR_Eval_Res res = evalUserFunc(node.children.data[0], in, node.children.data[0].type);
			if (!res.succ) return res;
			switch (type) {
				case IR_TYPE_VEC2: res.val.v = (Vector2){.x = fabsf(res.val.v.x), .y = fabsf(res.val.v.y)}; break;
				case IR_TYPE_REAL: res.val.f = fabsf(res.val.f); break;
				default:           res.val.i = abs(res.val.i);   break;
			}
			return res;
		}
		case IR_INST_SQRT: EVAL_REAL_UNARY_FUNC(sqrtf);
		case IR_INST_LOG:  EVAL_REAL_UNARY_FUNC(logf);
		case IR_INST_SIN:  EVAL_REAL_UNARY_FUNC(sinf);
		case IR_INST_COS:  EVAL_REAL_UNARY_FUNC(cosf);
		case IR_INST_TAN:  EVAL_REAL_UNARY_FUNC(tanf);
		case IR_INST_MAX: {
			if (node.children.len != 2) return (IR_Eval_Res){0};
			IR_Eval_Res a = evalUserFunc(node.children.data[0], in, desired_type);
			IR_Eval_Res b = evalUserFunc(node.children.data[1], in, desired_type);
			if (!a.succ || !b.succ) return (IR_Eval_Res){ .val = {0}, .succ = false };
			IR_Val v = {0};
			switch (desired_type) {
				case IR_TYPE_NAT:
				case IR_TYPE_POS:  v.i = AIL_MAX(a.val.i, b.val.i); break;
				case IR_TYPE_REAL: v.f = AIL_MAX(a.val.f, b.val.f); break;
				case IR_TYPE_VEC2: AIL_TODO();
				case IR_TYPE_LEN:  AIL_UNREACHABLE();
			}
			return (IR_Eval_Res) { .val = v, .succ = true };
		}
		case IR_INST_MIN: {
			if (node.children.len != 2) return (IR_Eval_Res){0};
			IR_Eval_Res a = evalUserFunc(node.children.data[0], in, desired_type);
			IR_Eval_Res b = evalUserFunc(node.children.data[1], in, desired_type);
			if (!a.succ || !b.succ) return (IR_Eval_Res){ .val = {0}, .succ = false };
			IR_Val v = {0};
			switch (desired_type) {
				case IR_TYPE_NAT:
				case IR_TYPE_POS:  v.i = AIL_MIN(a.val.i, b.val.i); break;
				case IR_TYPE_REAL: v.f = AIL_MIN(a.val.f, b.val.f); break;
				case IR_TYPE_VEC2: AIL_TODO();
				case IR_TYPE_LEN:  AIL_UNREACHABLE();
			}
			return (IR_Eval_Res) { .val = v, .succ = true };
		}
		case IR_INST_CLAMP: {
			if (node.children.len != 3) return (IR_Eval_Res){0};
			IR_Eval_Res a = evalUserFunc(node.children.data[0], in, desired_type);
			IR_Eval_Res b = evalUserFunc(node.children.data[1], in, desired_type);
			IR_Eval_Res c = evalUserFunc(node.children.data[2], in, desired_type);
			if (!a.succ || !b.succ) return (IR_Eval_Res){ .val = {0}, .succ = false };
			IR_Val v = {0};
			switch (desired_type) {
				case IR_TYPE_NAT:
				case IR_TYPE_POS:  v.i = AIL_CLAMP(a.val.i, b.val.i, c.val.i); break;
				case IR_TYPE_REAL: v.f = AIL_CLAMP(a.val.f, b.val.f, c.val.f); break;
				case IR_TYPE_VEC2: AIL_TODO();
				case IR_TYPE_LEN:  AIL_UNREACHABLE();
			}
			return (IR_Eval_Res) { .val = v, .succ = true };
		}
		case IR_INST_LERP: {
			if (node.children.len != 3) return (IR_Eval_Res){0};
			IR_Eval_Res a = evalUserFunc(node.children.data[0], in, desired_type);
			IR_Eval_Res b = evalUserFunc(node.children.data[1], in, desired_type);
			IR_Eval_Res c = evalUserFunc(node.children.data[2], in, desired_type);
			if (!a.succ || !b.succ) return (IR_Eval_Res){ .val = {0}, .succ = false };
			IR_Val v = {0};
			switch (desired_type) {
				case IR_TYPE_NAT:
				case IR_TYPE_POS:  v.i = AIL_LERP(a.val.i, b.val.i, c.val.i); break;
				case IR_TYPE_REAL: v.f = AIL_LERP(a.val.f, b.val.f, c.val.f); break;
				case IR_TYPE_VEC2: AIL_TODO();
				case IR_TYPE_LEN:  AIL_UNREACHABLE();
			}
			return (IR_Eval_Res) { .val = v, .succ = true };
		}
		case IR_INST_ADD: {
			if (!node.children.len) return (IR_Eval_Res){0};
			IR_Val out = {0};
			for (u32 i = 0; i < node.children.len; i++) {
				IR_Eval_Res res = evalUserFunc(node.children.data[i], in, desired_type);
				if (!res.succ) return res;
				switch (desired_type) {
					case IR_TYPE_POS:
					case IR_TYPE_NAT:  out.i += res.val.i; break;
					case IR_TYPE_REAL: out.f += res.val.f; break;
					case IR_TYPE_VEC2: out.v = addVector2(out.v, res.val.v); break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_SUB: {
			if (!node.children.len) return (IR_Eval_Res){0};
			u32 i = 0;
			IR_Val out;
			if (node.children.len == 1) {
				switch (desired_type) {
					case IR_TYPE_POS:
					case IR_TYPE_NAT:  out = (IR_Val){ .i = 0 };            break;
					case IR_TYPE_REAL: out = (IR_Val){ .f = 0.0f };         break;
					case IR_TYPE_VEC2: out = (IR_Val){ .v = (Vector2){0} }; break;
					default: return (IR_Eval_Res){0};
				}
			} else {
				IR_Eval_Res res = evalUserFunc(node.children.data[0], in, desired_type);
				if (!res.succ) return res;
				out = res.val;
				i   = 1;
			}
			for (; i < node.children.len; i++) {
				IR_Eval_Res res = evalUserFunc(node.children.data[i], in, desired_type);
				if (!res.succ) return res;
				switch (desired_type) {
					case IR_TYPE_POS:
					case IR_TYPE_NAT:  out.i -= res.val.i;                    break;
					case IR_TYPE_REAL: out.f -= res.val.f;                    break;
					case IR_TYPE_VEC2: out.v  = subVector2(out.v, res.val.v); break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_MOD: {
			if (!node.children.len) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(node.children.data[0], in, desired_type);
			if (node.children.len == 1 || !res.succ) return res;
			IR_Val out = res.val;
			for (u32 i = 1; i < node.children.len; i++) {
				res = evalUserFunc(node.children.data[i], in, desired_type);
				if (!res.succ) return res;
				switch (desired_type) {
					case IR_TYPE_POS:
					case IR_TYPE_NAT:  out.i %= res.val.i;                    break;
					case IR_TYPE_REAL: out.f  = fmodf(out.f, res.val.f);      break;
					case IR_TYPE_VEC2: out.v  = modVector2(out.v, res.val.v); break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_MUL: {
			if (!node.children.len) return (IR_Eval_Res){0};
			IR_Val out;
			switch (desired_type) {
				case IR_TYPE_POS:
				case IR_TYPE_NAT:  out = (IR_Val){ .i = 1 };                     break;
				case IR_TYPE_REAL: out = (IR_Val){ .f = 1.0f };                  break;
				case IR_TYPE_VEC2: out = (IR_Val){ .v = (Vector2){1.0f, 1.0f} }; break;
				default: return (IR_Eval_Res){0};
			}
			for (u32 i = 0; i < node.children.len; i++) {
				IR_Eval_Res res = evalUserFunc(node.children.data[i], in, desired_type);
				if (!res.succ) return res;
				switch (desired_type) {
					case IR_TYPE_POS:
					case IR_TYPE_NAT:  out.i *= res.val.i; break;
					case IR_TYPE_REAL: out.f *= res.val.f; break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_DIV: {
			if (!node.children.len) return (IR_Eval_Res){0};
			u32 i = 0;
			IR_Val out;
			if (node.children.len == 1) {
				switch (desired_type) {
					case IR_TYPE_POS:
					case IR_TYPE_NAT:  out = (IR_Val){ .i = 1 };    break;
					case IR_TYPE_REAL: out = (IR_Val){ .f = 1.0f }; break;
					default:  return (IR_Eval_Res){0};
				}
			} else {
				IR_Eval_Res res = evalUserFunc(node.children.data[0], in, desired_type);
				if (!res.succ) return res;
				out = res.val;
				i   = 1;
			}
			for (; i < node.children.len; i++) {
				IR_Eval_Res res = evalUserFunc(node.children.data[i], in, desired_type);
				if (!res.succ) return res;
				switch (desired_type) {
					case IR_TYPE_POS:
					case IR_TYPE_NAT:
					   if (res.val.i == 0) out.i = 0;
					   else out.i /= res.val.i;
					   break;
					case IR_TYPE_REAL:
					   if (res.val.f == 0) out.f = 0;
					   else out.f /= res.val.f;
					   break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_POW: {
			if (node.children.len < 2) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(node.children.data[0], in, desired_type);
			if (!res.succ) return res;
			IR_Val out = res.val;
			for (u32 i = 1; i < node.children.len; i++) {
				res = evalUserFunc(node.children.data[i], in, desired_type);
				if (!res.succ) return res;
				switch (desired_type) {
					case IR_TYPE_POS:
					case IR_TYPE_NAT:  out.i = powi(out.i, res.val.i); break;
					case IR_TYPE_REAL: out.f = powf(out.f, res.val.f); break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		default:
			AIL_UNREACHABLE();
	}
}

// void addRandChildren(IR *node, i32 depth)
// {
// 	IR_NAMED_TOK_MAP namedTokMap[] = NAMED_TOK_MAP;
// 	IR_Inst randLiterals[] = RAND_LITERALS;
// 	i32 amount = getExpectedChildAmount(node->inst);
// 	if (amount < 0) amount = 2 + (xorshift() % 3);
// 	for (i32 i = 0; i < amount; i++) {
// 		IR child;
// 		if (depth >= MAX_RAND_DEPTH) {
// 			IR_Inst inst = randLiterals[xorshift() % (sizeof(randLiterals)/sizeof(randLiterals[0]))];
// 			child = (IR){ .inst = inst, .type = IR_TYPE_FLOAT, .val = {0}, .children = ail_da_new_empty(IR) };
// 		} else {
// 			do {
// 			    bool getPrefered = (xorshift() % 2) == 0;
// 			    u32 idx;
// 			    if (getPrefered) idx = RAND_PREFERED_NAMED_TOK_MAP_MIN + (xorshift() % RAND_PREFERED_NAMED_TOK_MAP_LEN);
// 				else             idx = xorshift() % sizeof(namedTokMap)/sizeof(namedTokMap[0]);
// 				child = namedTokMap[idx].ir;
// 			} while (AIL_UNLIKELY(child.type != IR_TYPE_ANY && child.type != IR_TYPE_FLOAT));
// 			addRandChildren(&child, depth + 1);
// 		}
// 		ail_da_push(&node->children, child);
// 	}
// }

IR randFunction()
{
	IR root = { .inst = IR_INST_ROOT, .type = IR_TYPE_VEC2, .val = {0}, .children = ail_da_new_with_cap(IR, 1) };
	IR vec2 = { .inst = IR_INST_VEC2, .type = IR_TYPE_VEC2, .val = {0}, .children = ail_da_new_with_cap(IR, 2) };
	// addRandChildren(&vec2, 1);
	ail_da_push(&root.children, vec2);
	return root;
}

void irToStrHelper(IR node, AIL_DA(char) *sb)
{
	const char *name = NULL;
	#define X(instr, str, t) if (instr == node.inst) { name = str; }
		INSTRUCTIONS
	#undef X
	AIL_ASSERT(name != NULL);

	if (node.children.len > 0) ail_da_push(sb, '(');
	ail_da_pushn(sb, name, strlen(name) + 1);
	sb->data[sb->len - 1] = ' ';
	if (node.children.len > 0) {
		for (u32 i = 0; i < node.children.len; i++) {
			irToStrHelper(((IR *)node.children.data)[i], sb);
		}
		ail_da_push(sb, ')');
		ail_da_push(sb, ' ');
	}
}

AIL_DA(char) irToStr(IR node)
{
	AIL_DA(char) sb = ail_da_new(char);
	if (node.inst == IR_INST_ROOT) node = node.children.data[node.children.len - 1];
	irToStrHelper(node, &sb);
	ail_da_push(&sb, 0);
	return sb;
}