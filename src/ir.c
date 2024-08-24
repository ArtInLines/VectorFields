#include "ir.h"

// @Note: Keep updated with IR_Inst
const char *instStrs[] = {"IR_INST_ROOT", "IR_META_INST_FIRST_CHILDLESS", "IR_INST_X", "IR_INST_Y", "IR_INST_LITERAL", "IR_META_INST_LAST_CHILDLESS", "IR_META_INST_FIRST_UNARY", "IR_INST_CONV", "IR_INST_ABS", "IR_INST_SQRT", "IR_INST_LOG", "IR_META_INST_FIRST_TRIG", "IR_INST_SIN", "IR_INST_COS", "IR_INST_TAN", "IR_META_INST_LAST_TRIG", "IR_META_INST_LAST_UNARY", "IR_META_INST_FIRST_BINARY", "IR_INST_VEC2", "IR_INST_MAX", "IR_INST_MIN", "IR_META_INST_LAST_BINARY", "IR_META_INST_FIRST_TERTIARY", "IR_INST_CLAMP", "IR_INST_LERP", "IR_META_INST_LAST_TERTIARY", "IR_META_INST_FIRST_LASSOC", "IR_INST_ADD", "IR_INST_SUB", "IR_INST_MOD", "IR_INST_MUL", "IR_INST_DIV", "IR_META_INST_LAST_LASSOC", "IR_META_INST_FIRST_RASSOC", "IR_INST_POW", "IR_META_INST_LAST_RASSOC", "IR_META_INST_LEN"};

// @Note: Keep updated with IR_Type
const char *typeStrs[] = {"IR_TYPE_INT", "IR_TYPE_FLOAT", "IR_TYPE_VEC2", "IR_TYPE_ANY", "IR_TYPE_LEN"};

void printIRHelper(IR node, i32 indent)
{
	for (i32 i = 0; i < indent; i++) printf("  ");
	printf("inst: %s, type: %s, val: ", instStrs[node.inst], typeStrs[node.type]);
	AIL_STATIC_ASSERT(IR_TYPE_LEN == 4);
	switch (node.type) {
		case IR_TYPE_INT:   printf("%d", node.val.i); break;
		case IR_TYPE_FLOAT: printf("%f", node.val.f); break;
		case IR_TYPE_VEC2:  printf("(%f, %f)", node.val.v.x, node.val.v.y); break;
		case IR_TYPE_ANY:   printf("-"); break;
		default:            AIL_UNREACHABLE();
	}
	printf("\n");
	for (u32 i = 0; i < node.children.len; i++)
		printIRHelper(((IR *)node.children.data)[i], indent + 1);
}

void printIR(IR node)
{
	printIRHelper(node, 0);
}

bool isAlpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

bool isNum(char c)
{
	return c >= '0' && c <= '9';
}

bool isOp(char c)
{
	// Asserts exist, bc we assume that only '+', '-', '*', '/' and '**' exist as operators
	AIL_STATIC_ASSERT(IR_META_INST_LAST_LASSOC - IR_META_INST_FIRST_LASSOC - 1 == 5);
	AIL_STATIC_ASSERT(IR_META_INST_LAST_RASSOC - IR_META_INST_FIRST_RASSOC - 1 == 1);
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '%';
}

// Unlike strcmp only `a` needs to be null-terminated
bool strEq(const char *a, const char *b)
{
	while (*a && *a++ == *b++) {}
	return !*a && *(a-1) == *(b-1);
}

// Return value is NULL or an error message
// The parsed IR is written to node
Parse_Err parseExpr(char *text, i32 len, i32 *idx, IR *node, i32 depth)
{
	const IR_NAMED_TOK_MAP namedTokMap[] = NAMED_TOK_MAP;

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
			for (size_t k = 0; !foundIR && k < sizeof(namedTokMap)/sizeof(namedTokMap[0]); k++) {
				if (strEq(namedTokMap[k].s, &text[*idx])) {
					ir = namedTokMap[k].ir;
					foundIR = true;
				}
			}
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
Parse_Err parseUserFunc(char *text, i32 textlen, IR *root)
{
	root->inst = IR_INST_ROOT;
	root->type = IR_TYPE_VEC2;
	root->val  = (IR_Val) {0};
	root->children = ail_da_new_empty(IR);

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
	memcpy(&newNode, &((IR *)node->children.data)[idx], sizeof(IR));
	((IR *)node->children.data)[idx].children = ail_da_new_empty(IR);
	ail_da_push(&((IR *)node->children.data)[idx].children, newNode);
	((IR *)node->children.data)[idx].inst = IR_INST_CONV;
	((IR *)node->children.data)[idx].type = IR_TYPE_FLOAT;
	((IR *)node->children.data)[idx].val  = (IR_Val) {0};
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

// @AIL_TODO: Provide error messages
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
		// Figure out type for operations that take several types
		if (root->type == IR_TYPE_ANY) {
			root->type = ((IR *)root->children.data)[0].type;
			for (i32 i = 1; i < len; i++) {
				IR_Type t = ((IR *)root->children.data)[i].type;
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
		}

		if (inst == IR_INST_VEC2) {
			for (i32 i = 0; i < len; i++) {
				IR_Type t = ((IR *)root->children.data)[i].type;
				switch (t) {
					case IR_TYPE_INT:   insertConv(root, i); break;
					case IR_TYPE_FLOAT: break;
					default:            return false;
				}
			}
		} else {
			for (i32 i = 0; i < len; i++) {
				IR_Type t = ((IR *)root->children.data)[i].type;
				if (t == root->type) continue;
				switch (t) {
					case IR_TYPE_INT:
						if (root->type == IR_TYPE_FLOAT) insertConv(root, i);
						else return false;
						break;
					case IR_TYPE_FLOAT:
					case IR_TYPE_VEC2:
						return false;
					default:
						AIL_UNREACHABLE();
				}
			}
		}
	}
	return true;
}

IR_Eval_Res evalUserFunc(IR node, Vector2 in)
{
	AIL_STATIC_ASSERT(IR_META_INST_LEN == 38);
	switch (node.inst) {
		case IR_INST_ROOT: {
			IR_Eval_Res res;
			for (u32 i = 0; i < node.children.len; i++) {
				res = evalUserFunc(((IR *)node.children.data)[i], in);
				if (!res.succ) return res;
			}
			return res;
		}
		case IR_INST_CONV: {
			if (node.type != IR_TYPE_FLOAT) AIL_TODO();
			if (node.children.len > 1) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[0], in);
			if (!res.succ) return res;
			if (((IR *)node.children.data)[0].type != IR_TYPE_FLOAT) res.val.f = (float) res.val.i;
			return (IR_Eval_Res){ .val = res.val, .succ = true };
		}
		case IR_INST_VEC2: {
			if (node.children.len != 2) return (IR_Eval_Res){0};
			if (((IR *)node.children.data)[0].type != IR_TYPE_FLOAT || ((IR *)node.children.data)[1].type != IR_TYPE_FLOAT) return (IR_Eval_Res){0};
			IR_Eval_Res x = evalUserFunc(((IR *)node.children.data)[0], in);
			IR_Eval_Res y = evalUserFunc(((IR *)node.children.data)[1], in);
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
		case IR_INST_XN: {
			return (IR_Eval_Res){ .val = (IR_Val){.f = fabsf(in.x)}, .succ = true };
		}
		case IR_INST_YN: {
			return (IR_Eval_Res){ .val = (IR_Val){.f = fabsf(in.y)}, .succ = true };
		}
		case IR_INST_LITERAL: {
			return (IR_Eval_Res){ .val = node.val, .succ = true };
		}
		case IR_INST_ABS: {
			IR_Eval_Res res = evalUserFunc(node.children.data[0], in);
			if (!res.succ) return res;
			switch (node.type) {
				case IR_TYPE_INT:   res.val.i = abs(res.val.i);   break;
				case IR_TYPE_FLOAT: res.val.f = fabsf(res.val.f); break;
				case IR_TYPE_VEC2:  res.val.v = (Vector2){.x = fabsf(res.val.v.x), .y = fabsf(res.val.v.y)}; break;
				case IR_TYPE_ANY:
				case IR_TYPE_LEN: AIL_UNREACHABLE();
			}
			return res;
		}
		case IR_INST_SQRT: {
            IR_Eval_Res res = evalUserFunc(node.children.data[0], in);
            if (!res.succ) return res;
            else return (IR_Eval_Res) { .val = (IR_Val){.f = sqrtf(res.val.f)}, .succ = true };
		}
		case IR_INST_LOG: {
            IR_Eval_Res res = evalUserFunc(node.children.data[0], in);
            if (!res.succ) return res;
            else return (IR_Eval_Res) { .val = (IR_Val){.f = logf(res.val.f)}, .succ = true };
		}
		case IR_INST_SIN: {
			if (node.children.len != 1) return (IR_Eval_Res){0};
			if (((IR *)node.children.data)[0].type != IR_TYPE_FLOAT) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[0], in);
			if (!res.succ) return res;
			return (IR_Eval_Res) { .val = (IR_Val){.f = sinf(res.val.f)}, .succ = true };
		}
		case IR_INST_COS: {
			if (node.children.len != 1) return (IR_Eval_Res){0};
			if (((IR *)node.children.data)[0].type != IR_TYPE_FLOAT) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[0], in);
			if (!res.succ) return res;
			return (IR_Eval_Res) { .val = (IR_Val){.f = cosf(res.val.f)}, .succ = true };
		}
		case IR_INST_TAN: {
			if (node.children.len != 1) return (IR_Eval_Res){0};
			if (((IR *)node.children.data)[0].type != IR_TYPE_FLOAT) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[0], in);
			if (!res.succ) return res;
			return (IR_Eval_Res) { .val = (IR_Val){.f = tanf(res.val.f)}, .succ = true };
		}
		case IR_INST_MAX: {
			if (node.children.len != 2) return (IR_Eval_Res){0};
			IR_Eval_Res a = evalUserFunc(((IR *)node.children.data)[0], in);
			IR_Eval_Res b = evalUserFunc(((IR *)node.children.data)[1], in);
			if (!a.succ || !b.succ) return (IR_Eval_Res){ .val = {0}, .succ = false };
			IR_Val v = {0};
			switch (node.type) {
				case IR_TYPE_INT:   v.i = AIL_MAX(a.val.i, b.val.i); break;
				case IR_TYPE_FLOAT: v.f = AIL_MAX(a.val.f, b.val.f); break;
				case IR_TYPE_VEC2:  AIL_TODO();
				case IR_TYPE_ANY:
				case IR_TYPE_LEN:   AIL_UNREACHABLE();
			}
			return (IR_Eval_Res) { .val = v, .succ = true };
		}
		case IR_INST_MIN: {
			if (node.children.len != 2) return (IR_Eval_Res){0};
			IR_Eval_Res a = evalUserFunc(((IR *)node.children.data)[0], in);
			IR_Eval_Res b = evalUserFunc(((IR *)node.children.data)[1], in);
			if (!a.succ || !b.succ) return (IR_Eval_Res){ .val = {0}, .succ = false };
			IR_Val v = {0};
			switch (node.type) {
				case IR_TYPE_INT:   v.i = AIL_MIN(a.val.i, b.val.i); break;
				case IR_TYPE_FLOAT: v.f = AIL_MIN(a.val.f, b.val.f); break;
				case IR_TYPE_VEC2:  AIL_TODO();
				case IR_TYPE_ANY:
				case IR_TYPE_LEN:   AIL_UNREACHABLE();
			}
			return (IR_Eval_Res) { .val = v, .succ = true };
		}
		case IR_INST_CLAMP: {
			if (node.children.len != 3) return (IR_Eval_Res){0};
			IR_Eval_Res a = evalUserFunc(((IR *)node.children.data)[0], in);
			IR_Eval_Res b = evalUserFunc(((IR *)node.children.data)[1], in);
			IR_Eval_Res c = evalUserFunc(((IR *)node.children.data)[2], in);
			if (!a.succ || !b.succ || !c.succ) return (IR_Eval_Res){ .val = {0}, .succ = false };
			IR_Val v = {0};
			switch (node.type) {
				case IR_TYPE_INT:   v.i = AIL_CLAMP(a.val.i, b.val.i, c.val.i); break;
				case IR_TYPE_FLOAT: v.f = AIL_CLAMP(a.val.f, b.val.f, c.val.f); break;
				case IR_TYPE_VEC2:  AIL_TODO();
				case IR_TYPE_ANY:
				case IR_TYPE_LEN:   AIL_UNREACHABLE();
			}
			return (IR_Eval_Res) { .val = v, .succ = true };
		}
		case IR_INST_LERP: {
			if (node.children.len != 3) return (IR_Eval_Res){0};
			IR_Eval_Res a = evalUserFunc(((IR *)node.children.data)[0], in);
			IR_Eval_Res b = evalUserFunc(((IR *)node.children.data)[1], in);
			IR_Eval_Res c = evalUserFunc(((IR *)node.children.data)[2], in);
			if (!a.succ || !b.succ || !c.succ) return (IR_Eval_Res){ .val = {0}, .succ = false };
			IR_Val v = {0};
			switch (node.type) {
				case IR_TYPE_INT:   v.i = AIL_LERP(a.val.i, b.val.i, c.val.i); break;
				case IR_TYPE_FLOAT: v.f = AIL_LERP(a.val.f, b.val.f, c.val.f); break;
				case IR_TYPE_VEC2:  AIL_TODO();
				case IR_TYPE_ANY:
				case IR_TYPE_LEN:   AIL_UNREACHABLE();
			}
			return (IR_Eval_Res) { .val = v, .succ = true };
		}
		case IR_INST_ADD: {
			if (!node.children.len) return (IR_Eval_Res){0};
			IR_Val out = {0};
			for (u32 i = 0; i < node.children.len; i++) {
				IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[i], in);
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
			if (!node.children.len) return (IR_Eval_Res){0};
			u32 i = 0;
			IR_Val out;
			if (node.children.len == 1) {
				switch (node.type) {
					case IR_TYPE_INT:   out = (IR_Val){ .i = 0 };            break;
					case IR_TYPE_FLOAT: out = (IR_Val){ .f = 0.0f };         break;
					case IR_TYPE_VEC2:  out = (IR_Val){ .v = (Vector2){0} }; break;
					default: return (IR_Eval_Res){0};
				}
			} else {
				IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[0], in);
				if (!res.succ) return res;
				out = res.val;
				i   = 1;
			}
			for (; i < node.children.len; i++) {
				IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[i], in);
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
		case IR_INST_MOD: {
			if (!node.children.len) return (IR_Eval_Res){0};
			IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[0], in);
			if (node.children.len == 1 || !res.succ) return res;
			IR_Val out = res.val;
			for (u32 i = 1; i < node.children.len; i++) {
				res = evalUserFunc(((IR *)node.children.data)[i], in);
				if (!res.succ) return res;
				switch (node.type) {
					case IR_TYPE_INT:   out.i %= res.val.i;                    break;
					case IR_TYPE_FLOAT: out.f  = fmodf(out.f, res.val.f);      break;
					case IR_TYPE_VEC2:  out.v  = modVector2(out.v, res.val.v); break;
					default: return (IR_Eval_Res){0};
				}
			}
			return (IR_Eval_Res){ .val = out, .succ = true };
		}
		case IR_INST_MUL: {
			if (!node.children.len) return (IR_Eval_Res){0};
			IR_Val out;
			switch (node.type) {
				case IR_TYPE_INT:   out = (IR_Val){ .i = 1 };                     break;
				case IR_TYPE_FLOAT: out = (IR_Val){ .f = 1.0f };                  break;
				case IR_TYPE_VEC2:  out = (IR_Val){ .v = (Vector2){1.0f, 1.0f} }; break;
				default: return (IR_Eval_Res){0};
			}
			for (u32 i = 0; i < node.children.len; i++) {
				IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[i], in);
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
			if (!node.children.len) return (IR_Eval_Res){0};
			u32 i = 0;
			IR_Val out;
			if (node.children.len == 1) {
				switch (node.type) {
					case IR_TYPE_INT:   out = (IR_Val){ .i = 1 };    break;
					case IR_TYPE_FLOAT: out = (IR_Val){ .f = 1.0f }; break;
					default:  return (IR_Eval_Res){0};
				}
			} else {
				IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[0], in);
				if (!res.succ) return res;
				out = res.val;
				i   = 1;
			}
			for (; i < node.children.len; i++) {
				IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[i], in);
				if (!res.succ) return res;
				switch (node.type) {
					case IR_TYPE_INT:
					   if (res.val.i == 0) out.i = 0;
					   else out.i /= res.val.i;
					   break;
					case IR_TYPE_FLOAT:
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
			IR_Eval_Res res = evalUserFunc(((IR *)node.children.data)[0], in);
			if (!res.succ) return res;
			IR_Val out = res.val;
			for (u32 i = 1; i < node.children.len; i++) {
				res = evalUserFunc(((IR *)node.children.data)[i], in);
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
			AIL_UNREACHABLE();
	}
}

#define RAND_MAX_DEPTH 4
#define RAND_PREFERED_CHANCE 98 // in percentage points

void addRandChildren(IR *node, i32 depth)
{
	IR_NAMED_TOK_MAP namedTokMap[] = NAMED_TOK_MAP;
	IR_Inst randLiterals[] = RAND_LITERALS;
	i32 amount = getExpectedChildAmount(node->inst);
	if (amount < 0) amount = 2 + (xorshift() % RAND_MAX_DEPTH);
	for (i32 i = 0; i < amount; i++) {
		IR child;
		if (depth >= MAX_RAND_DEPTH) {
			IR_Inst inst = randLiterals[xorshift() % (sizeof(randLiterals)/sizeof(randLiterals[0]))];
			child = (IR){ .inst = inst, .type = IR_TYPE_FLOAT, .val = {0}, .children = ail_da_new_empty(IR) };
		} else {
			do {
			    bool getPrefered = (xorshift() % 100) < RAND_PREFERED_CHANCE;
				// printf("getPrefered: %d\n", getPrefered);
			    u32 idx;
			    if (getPrefered) idx = RAND_PREFERED_NAMED_TOK_MAP_MIN + (xorshift() % RAND_PREFERED_NAMED_TOK_MAP_LEN);
				else             idx = xorshift() % AIL_ARRLEN(namedTokMap);
				child = namedTokMap[idx].ir;
			} while (AIL_UNLIKELY(child.type != IR_TYPE_ANY && child.type != IR_TYPE_FLOAT));
			addRandChildren(&child, depth + 1);
		}
		ail_da_push(&node->children, child);
	}
}

IR randFunction(void)
{
	IR root = { .inst = IR_INST_ROOT, .type = IR_TYPE_VEC2, .val = {0}, .children = ail_da_new_with_cap(IR, 1) };
	IR vec2 = { .inst = IR_INST_VEC2, .type = IR_TYPE_VEC2, .val = {0}, .children = ail_da_new_with_cap(IR, 2) };
	addRandChildren(&vec2, 1);
	ail_da_push(&root.children, vec2);
	return root;
}

void irToStrHelper(IR node, AIL_DA(char) *sb)
{
	IR_NAMED_TOK_MAP namedTokMap[] = NAMED_TOK_MAP;
	const char *name = NULL;
	for (u32 i = 0; i < sizeof(namedTokMap)/sizeof(namedTokMap[0]); i++) {
		if (namedTokMap[i].ir.inst == node.inst) {
			name = namedTokMap[i].s;
			break;
		}
	}
	AIL_ASSERT(name != NULL);

	if (node.children.len > 0) ail_da_push(sb, '(');
	ail_da_pushn(sb, name, strlen(name) + 1);
	sb->data[sb->len - 1] = ' ';
	if (node.children.len > 0) {
		for (u32 i = 0; i < node.children.len; i++) {
			irToStrHelper(((IR *)node.children.data)[i], sb);
		}
		sb->len--;
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
