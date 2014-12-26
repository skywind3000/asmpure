//=====================================================================
//
// cparser.c - source parser
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#include "cparser.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

#define IMAX_DATA 65536

CParser *cparser_create(void)
{
	CParser *parser;
	parser = (CParser*)malloc(sizeof(CParser));
	assert(parser);
	parser->token = cscanner_create();
	assert(parser->token);
	parser->instruction = NULL;
	parser->instructionset = cinstset_create();
	csynth_init(&parser->synthesizer);
	parser->data = (char*)malloc(IMAX_DATA);
	assert(parser->data);
	parser->error = (char*)malloc(1024);
	assert(parser->error);
	parser->error[0] = 0;
	parser->errcode = 0;
	parser->vars = NULL;
	parser->inproc = 0;
	parser->stack = 0;
	return parser;
}

void cparser_release(CParser *parser)
{
	assert(parser);
	if (parser->token) {
		cscanner_release(parser->token);
		parser->token = NULL;
	}
	if (parser->instructionset) {
		cinstset_release(parser->instructionset);
		parser->instructionset = NULL;
	}
	if (parser->error) {
		free(parser->error);
		parser->error = NULL;
	}
	if (parser->data) {
		free(parser->data);
		parser->data = NULL;
	}
	while (parser->vars) {
		CVariable *var = parser->vars;
		parser->vars = parser->vars->next;
		free(var->name);
		free(var);
	}
	csynth_destroy(&parser->synthesizer);
	free(parser);
}

void cparser_reset(CParser *parser)
{
	cscanner_macro_reset(parser->token);
	while (parser->vars) {
		CVariable *var = parser->vars;
		parser->vars = parser->vars->next;
		free(var->name);
		free(var);
	}
	parser->inproc = 0;
	parser->stack = 0;
}

static int cparser_parse_label(CParser *parser);
static int cparser_parse_mnemonic(CParser *parser);
static int cparser_parse_specifier(CParser *parser);
static int cparser_parse_first_operand(CParser *parser);
static int cparser_parse_second_operand(CParser *parser);
static int cparser_parse_third_operand(CParser *parser);
static COperand cparser_parse_immediate(CParser *parser);
static COperand cparser_parse_register(CParser *parser);
static COperand cparser_parse_memory_reference(CParser *parser);

static int cparser_parse_data(CParser *parser);
static int cparser_parse_align(CParser *parser);
static int cparser_parse_prefix(CParser *parser);

static int cparser_parse_proc(CParser *parser);

static void cparser_error(CParser *parser, const char *error, int code)
{
	strncpy(parser->error, error, 100);
	parser->errcode = code;
}

const CEncoding *cparser_parse_line(CParser *parser, const char *source)
{
	int retval;

	if (source == NULL) {
		cparser_error(parser, "empty source line", 1);
		return NULL;
	}

	retval = cscanner_set_source(parser->token, source);

	if (retval != 0) {
		cparser_error(parser, parser->token->error, 2);
		return NULL;
	}

	parser->instruction = NULL;
	csynth_reset(&parser->synthesizer);

	parser->error[0] = 0;
	parser->errcode = 0;

	// parse label
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_label(parser)) {
			cparser_error(parser, "label error", 3);
			return NULL;
		}
	}

	// parse inline data
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_data(parser)) {
			return NULL;
		}
		if (parser->synthesizer.encoding.data != NULL) {
			return &parser->synthesizer.encoding;
		}
	}

	// parse align
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_align(parser)) {
			return NULL;
		}
		if (parser->synthesizer.encoding.align != 0) {
			return &parser->synthesizer.encoding;
		}
	}

	// parse proc
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_proc(parser)) {
			return NULL;
		}
	}

	// parse repnz, repz
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_prefix(parser)) {
			return NULL;
		}
	}

	// parse mnemonic
	if (!cscanner_is_endl(parser->token)) {
		if (cparser_parse_mnemonic(parser)) {
			cparser_error(parser, "mnemonic syntax error", 4);
			return NULL;
		}

		if (!parser->instruction) {
			cparser_error(parser, "mnemonic error", 5);
			return NULL;
		}

		if (cparser_parse_first_operand(parser)) {
			if (parser->errcode == 0) 
				cparser_error(parser, "first operand error", 6);
			return NULL;
		}

		if (cparser_parse_second_operand(parser)) {
			if (parser->errcode == 0)
				cparser_error(parser, "second operand error", 7);
			return NULL;
		}

		if (cparser_parse_third_operand(parser)) {
			if (parser->errcode == 0)
				cparser_error(parser, "third operand error", 8);
			return NULL;
		}
	}

	if (parser->instruction) {
		do {
			if (cinst_match_syntax(parser->instruction)) {
				break;
			}
			parser->instruction = parser->instruction->next;
		}	while (parser->instruction);

		if (parser->instruction == NULL) {
			cparser_error(parser, "operands mismatch", 9);
			return NULL;
		}
#if 0
		printf("%s (%s) (%s) specifier=%d\n",
			parser->instruction->syntax->mnemonic,
			parser->instruction->syntax->operands,
			parser->instruction->syntax->encoding,
			parser->instruction->specifier);
#endif
	}

	return csynth_encode_instruction(&parser->synthesizer, 
		parser->instruction);
}


static int cparser_parse_label(CParser *parser)
{
	const CTOKEN *current = cscanner_token_current(parser->token);
	const CTOKEN *next = cscanner_token_lookahead(parser->token);
	if (ctoken_is_ident(current) && ctoken_get_char(next) == ':') {
		csynth_define_label(&parser->synthesizer, current->str);
		cscanner_token_advance(parser->token, 2);
	}
	else if (ctoken_get_char(current) == '.' && ctoken_is_ident(next)) {
		csynth_define_label(&parser->synthesizer, next->str);
		cscanner_token_advance(parser->token, 2);
	}
	return 0;
}

static int cparser_parse_mnemonic(CParser *parser)
{
	const char *name = cscanner_get_string(parser->token);
	parser->instruction = cinstset_query(parser->instructionset, name);
	if (parser->instruction) {
		cinst_match_mnemonic(parser->instruction, name);
		cscanner_token_advance(parser->token, 1);
	}	else {
		cparser_error(parser, "Mnemonic not recognised", 10);
		return -1;
	}
	return 0;
}

static int cparser_parse_specifier(CParser *parser)
{
	enum CSpecifierType type = CS_UNKNOWN;

	if (cscanner_is_ident(parser->token)) {
		type = cspecifier_scan(cscanner_get_string(parser->token));
	}

	cinst_match_specifier(parser->instruction, type);

	if (type != CS_UNKNOWN) {
		cscanner_token_advance(parser->token, 1);
	}

	return 0;
}

static int cparser_parse_first_operand(CParser *parser)
{
	COperand firstOperand = CINIT;

	assert(parser->instruction);

	cparser_parse_specifier(parser);

	if (cscanner_is_endl(parser->token)) {
	}
	else if (cscanner_is_operator(parser->token)) {
		switch (cscanner_get_char(parser->token)) {
		case '[':
			firstOperand = cparser_parse_memory_reference(parser);
			if (parser->errcode) return -5;
			break;
		case '+':
		case '-':
		case '~':
			firstOperand = cparser_parse_immediate(parser);
			break;
		default:
			cparser_error(parser, "Unexpected punctuator after mnemonic", 1);
			return -1;
			break;
		}
	}
	else if (cscanner_is_int(parser->token)) {
		firstOperand = cparser_parse_immediate(parser);
		if (parser->errcode) return -5;
	}
	else if (cscanner_is_ident(parser->token)) {
		firstOperand = cparser_parse_register(parser);
		if (parser->errcode) return -5;
	}
	else {
		cparser_error(parser, "Invalid destination operand", 11);
		return -2;
	}

	cinst_match_first_operand(parser->instruction, &firstOperand);
	csynth_encode_first_operand(&parser->synthesizer, &firstOperand);

	return 0;
}

static int cparser_parse_second_operand(CParser *parser)
{
	COperand secondOperand = CINIT;
	assert(parser->instruction);

	if (cscanner_get_char(parser->token) == ',') {
		cscanner_token_advance(parser->token, 1);
	}	
	else if (!cscanner_is_endl(parser->token)) {
		cparser_error(parser, "Operands must be separated by comma", 12);
		return -3;
	}
	else {
		cinst_match_second_operand(parser->instruction, &secondOperand);
		return 0;
	}

	cparser_parse_specifier(parser);

	if (cscanner_is_endl(parser->token)) {
	}
	else if (cscanner_is_operator(parser->token)) {
		switch (cscanner_get_char(parser->token)) {
		case '[':
			secondOperand = cparser_parse_memory_reference(parser);
			if (parser->errcode) return -5;
			break;
		case '+':
		case '-':
		case '~':
			secondOperand = cparser_parse_immediate(parser);
			if (parser->errcode) return -5;
			break;
		default:
			cparser_error(parser, "Unexpected punctuator after mnemonic", 1);
			return -1;
			break;
		}
	}
	else if (cscanner_is_int(parser->token)) {
		secondOperand = cparser_parse_immediate(parser);
		if (parser->errcode) return -5;
	}
	else if (cscanner_is_ident(parser->token)) {
		secondOperand = cparser_parse_register(parser);
		if (parser->errcode) return -5;
	}
	else {
		cparser_error(parser, "Invalid source operand", 13);
		return -2;
	}

	cinst_match_second_operand(parser->instruction, &secondOperand);
	csynth_encode_second_operand(&parser->synthesizer, &secondOperand);

	return 0;
}

static int cparser_parse_third_operand(CParser *parser)
{
	COperand thirdOperand = CINIT;

	assert(parser->instruction);

	if (cscanner_get_char(parser->token) == ',') {
		cscanner_token_advance(parser->token, 1);
	}
	else if (!cscanner_is_endl(parser->token)) {
		cparser_error(parser, "Operands must be separated by comma", 14);
		return -3;
	}
	else {
		cinst_match_third_operand(parser->instruction, &thirdOperand);
		return 0;
	}

	if (cscanner_is_endl(parser->token)) {
	}
	else if (cscanner_is_operator(parser->token)) {
		switch (cscanner_get_char(parser->token)) {
		case '+':
		case '-':
		case '~':
			thirdOperand = cparser_parse_immediate(parser);
			if (parser->errcode) return -5;
			break;
		default:
			cparser_error(parser, "Unexpected punctuator after mnemonic", 1);
			return -1;
			break;
		}
	}
	else if (cscanner_is_int(parser->token)) {
		thirdOperand = cparser_parse_immediate(parser);
		if (parser->errcode) return -5;
	}
	else {
		cparser_error(parser, "Too many operands", 15);
		return -2;
	}

	cinst_match_third_operand(parser->instruction, &thirdOperand);
	csynth_encode_third_operand(&parser->synthesizer, &thirdOperand);

	return 0;
}

static COperand cparser_parse_immediate(CParser *parser)
{
	COperand imm = CINIT;
	if (cscanner_is_operator(parser->token)) {
		int ch = cscanner_get_char(parser->token);
		if (ch == '+') {
			cscanner_token_advance(parser->token, 1);
			imm.value = +cscanner_get_value(parser->token);
		}
		else if (ch == '-') {
			cscanner_token_advance(parser->token, 1);
			imm.value = -cscanner_get_value(parser->token);
		}
		else if (ch == '~') {
			cscanner_token_advance(parser->token, 1);
			imm.value = ~cscanner_get_value(parser->token);
		}
		else {
			cparser_error(parser, "error operator", 16);
			return imm;
		}
	}
	else if (cscanner_is_int(parser->token)) {
		imm.value = cscanner_get_value(parser->token);
	}
	else {
		cparser_error(parser, "immediate error", 17);
		return imm;
	}

	if ((unsigned char)imm.value == imm.value) {
		imm.type = O_IMM8;
	}
	else if ((unsigned short)imm.value == imm.value) {
		imm.type = O_IMM16;
	}
	else {
		imm.type = O_IMM32;
	}

	cscanner_token_advance(parser->token, 1);

	return imm;
}

static COperand cparser_parse_register(CParser *parser)
{
	COperand reg = CINIT;
	const char *name;

	name = cscanner_get_string(parser->token);
	reg = coperand_scan_reg(name);

	// It's not a register, so it must be a reference
	if (reg.type == O_UNKNOWN) {
		csynth_reference_label(&parser->synthesizer, name);
		// first operand should be immediate
		reg.type = O_IMM8;	// also matchs IMM32
		cinst_match_first_operand(parser->instruction, &reg);
	}

	cscanner_token_advance(parser->token, 1);

	return reg;
}

static COperand cparser_parse_memory_reference(CParser *parser)
{
	COperand mem = CINIT;

	for (; ; ) {
		const CTOKEN *next;
		const CTOKEN *prev;
		const CTOKEN *token;
		int type;

		type = cscanner_token_lookahead(parser->token)->type;
		if (type == CTokenENDL || type == CTokenENDF) break;

		prev = cscanner_token_current(parser->token);
		cscanner_token_advance(parser->token, 1);
		next = cscanner_token_lookahead(parser->token);
		token = cscanner_token_current(parser->token);

		if (token->type == CTokenIDENT) {
			COperand reg = coperand_scan_reg(token->str);
			if (reg.type == O_UNKNOWN) {
				cparser_error(parser, "unknow reg reference", 18);
				return reg;
			}
			if (ctoken_get_char(prev) == '*' || ctoken_get_char(next) == '*')
			{
				csynth_encode_index(&parser->synthesizer, &reg);
			}
			else 
			{
				csynth_encode_base(&parser->synthesizer, &reg);
			}
		}
		else if (token->type == CTokenOPERATOR) {
			switch (ctoken_get_char(token)) {
			case ']':
				mem.type = O_MEM;
				cscanner_token_advance(parser->token, 1);
				return mem;
				break;
			case '+':
				if ((prev->type != CTokenINT && prev->type != CTokenIDENT) ||
					(next->type != CTokenINT && next->type != CTokenIDENT)) {
					cparser_error(parser, 
						"Syntax error '+' in memory reference", 19);
					return mem;
				}
				break;
			case '-':
				if ((prev->type != CTokenINT && prev->type != CTokenIDENT &&
					ctoken_get_char(prev) != '[') ||
					next->type != CTokenINT) {
					cparser_error(parser, 
						"Syntax error '-' in memory reference", 20);
					return mem;
				}
				break;
			case '*':
				if ((prev->type != CTokenINT || next->type != CTokenIDENT) &&
					(next->type != CTokenINT || prev->type != CTokenIDENT)) {
					cparser_error(parser,
						"Syntax error '*' in memory reference", 21);
					return mem;
				}
				break;
			default:
				cparser_error(parser, 
					"Unexpected punctuator in memory reference", 22);
				return mem;
				break;
			}
		}
		else if (token->type == CTokenINT) {
			int prevch = ctoken_get_char(prev);
			int nextch = ctoken_get_char(next);
			int value = ctoken_get_int(token);
			if (prevch == '*' || nextch == '*') {
				if (value == 1 || value == 2 || value == 4 || value == 8) {
					csynth_encode_scale(&parser->synthesizer, value);
				}	else {
					cparser_error(parser, 
						"Invalid scale in memory reference", 23);
					return mem;
				}
			}
			else if (prevch == '-') {
				csynth_encode_displacement(&parser->synthesizer, -value);
			}
			else if (prevch == '+' || nextch == '+') {
				csynth_encode_displacement(&parser->synthesizer, value);
			}
			else if (prevch == '[' && nextch == ']') {
				cparser_error(parser,
					"Invalid number in memory reference", 30);
				return mem;
			}
			else {
				cparser_error(parser, 
					"Invalid number in memory reference", 24);
				return mem;
			}
		}
		else {
			cparser_error(parser, 
				"Unexpected token in memory reference", 25);
			return mem;
		}
	}

	cparser_error(parser, "Unexpected end of line in memory reference", 26);

	return mem;
}


static int cparser_parse_data(CParser *parser)
{
	const char *name;
	long pos = 0;
	int size = -1;

	if (cscanner_is_ident(parser->token) == 0) {
		return 0;
	}

	name = cscanner_get_string(parser->token);

	if (stricmp(name, "DB") == 0) size = 1;
	else if (stricmp(name, "DW") == 0) size = 2;
	else if (stricmp(name, "DD") == 0) size = 4;

	if (size < 0) return 0;

	cscanner_token_advance(parser->token, 1);

	for (pos = 0; ; ) {
		unsigned char *ptr = (unsigned char*)parser->data;
		const CTOKEN *token;

		if (cscanner_is_endl(parser->token)) break;

		token = cscanner_token_current(parser->token);

		if (token->type == CTokenINT) {
			cuint32 value = (cuint32)token->intval;
			if (pos + size >= IMAX_DATA) {
				cparser_error(parser, "data too long", 41);
				return -1;
			}
			if (size == 1) {
				ptr[pos++] = (unsigned char)((value >>  0) & 0xff);
			}
			else if (size == 2) {
				ptr[pos++] = (unsigned char)((value >>  0) & 0xff);
				ptr[pos++] = (unsigned char)((value >>  8) & 0xff);
			}
			else if (size == 4) {
				ptr[pos++] = (unsigned char)((value >>  0) & 0xff);
				ptr[pos++] = (unsigned char)((value >>  8) & 0xff);
				ptr[pos++] = (unsigned char)((value >> 16) & 0xff);
				ptr[pos++] = (unsigned char)((value >> 24) & 0xff);
			}
		}
		else if (token->type == CTokenSTR) {
			const char *text = token->str;
			long size, i, c;
			char hex[3];
			size = (long)strlen(text);
			for (i = 0; i < size; ) {
				if (i + 1 >= IMAX_DATA) {
					cparser_error(parser, "data too long", 41);
					return -2;
				}
				if (text[i] == '\\') {
					switch (text[i + 1])
					{
					case '\\': ptr[pos++] = '\\'; i += 2; break;
					case 'n' : ptr[pos++] = '\n'; i += 2; break;
					case 'r' : ptr[pos++] = '\r'; i += 2; break;
					case 't' : ptr[pos++] = '\t'; i += 2; break;
					case '0' : ptr[pos++] = '\0'; i += 2; break;
					case '?' : ptr[pos++] = '?'; i += 2; break;
					case '\'': ptr[pos++] = '\''; i += 2; break;
					case '\"': ptr[pos++] = '\"'; i += 2; break;
					case 'a' : ptr[pos++] = '\a'; i += 2; break;
					case 'b' : ptr[pos++] = '\b'; i += 2; break;
					case 'f' : ptr[pos++] = '\f'; i += 2; break;
					case 'v' : ptr[pos++] = '\v'; i += 2; break;
					case 'x' :
						i += 2;
						hex[0] = text[i++];
						hex[1] = text[i++];
						hex[2] = 0;
						c = strtol(hex, NULL, 16);
						ptr[pos++] = (unsigned char)(c & 255);
						break;
					default:
						cparser_error(parser, "string format error", 42);
						return -3;
						break;
					}
				}
				else if (text[i] == '\'') {
					if (text[i + 1] == '\'') {
						ptr[pos++] = '\'';
						i += 2;
					}	else {
						ptr[pos++] = '\'';
						i += 1;
					}
				}
				else if (text[i] == '\"') {
					if (text[i + 1] == '\"') {
						ptr[pos++] = '\"';
						i += 2;
					}	else {
						ptr[pos++] = '\"';
						i += 1;
					}
				}
				else {
					ptr[pos++] = (unsigned char)text[i++];
				}
			}
		}
		else {
			cparser_error(parser, "unrecongnize data", 43);
			return -4;
		}

		cscanner_token_advance(parser->token, 1);

		if (!cscanner_is_endl(parser->token)) {
			if (cscanner_get_char(parser->token) != ',') {
				cparser_error(parser, "expected comma", 40);
				return -3;
			}
			cscanner_token_advance(parser->token, 1);
		}
	}

	if (pos > 0) {
		cencoding_set_data(&parser->synthesizer.encoding, parser->data, pos);
	}

	return 0;
}


static int cparser_parse_prefix(CParser *parser)
{
	const char *name;

	if (cscanner_is_ident(parser->token) == 0) {
		return 0;
	}

	name = cscanner_get_string(parser->token);

	if (stricmp(name, "REP") == 0 ||
		stricmp(name, "REPE") == 0 ||
		stricmp(name, "REPZ") == 0) {
		if (csynth_encode_prefix(&parser->synthesizer, 0xf3)) {
			cparser_error(parser, parser->synthesizer.error, 70);
			return -1;
		}
		cscanner_token_advance(parser->token, 1);
	}
	else if (stricmp(name, "REPNE") == 0 || stricmp(name, "REPNZ") == 0) {
		parser->synthesizer.prefix = 0xf2;
		if (csynth_encode_prefix(&parser->synthesizer, 0xf2)) {
			cparser_error(parser, parser->synthesizer.error, 71);
			return -2;
		}
		cscanner_token_advance(parser->token, 1);
	}
	else if (stricmp(name, "LOCK") == 0) {
		if (csynth_encode_prefix(&parser->synthesizer, 0xf0)) {
			cparser_error(parser, parser->synthesizer.error, 72);
			return -3;
		}
		cscanner_token_advance(parser->token, 1);
	}

	return 0;
}

static int cparser_parse_align(CParser *parser)
{
	const char *name;

	if (cscanner_is_ident(parser->token) == 0) {
		return 0;
	}

	name = cscanner_get_string(parser->token);
	
	if (stricmp(name, "ALIGN") == 0) {
		int align = 4;
		cscanner_token_advance(parser->token, 1);
		if (cscanner_is_int(parser->token)) {
			align = cscanner_get_value(parser->token);
		}
		while (!cscanner_is_endf(parser->token)) {
			cscanner_token_advance(parser->token, 1);
		}
		if (align < 1) {
			cparser_error(parser, "error align size", 80);
			return -1;
		}
		parser->synthesizer.encoding.align = align;
	}

	return 0;
}

static int cparser_parse_size(CParser *parser)
{
	const CTOKEN *token = cscanner_token_current(parser->token);
	cscanner_token_advance(parser->token, 1);
	if (token->type == CTokenIDENT) {
		if (stricmp(token->str, "BYTE") == 0) return 1;
		if (stricmp(token->str, "CHAR") == 0) return 1;
		if (stricmp(token->str, "INT8") == 0) return 1;
		if (stricmp(token->str, "UINT8") == 0) return 1;
		if (stricmp(token->str, "WORD") == 0) return 2;
		if (stricmp(token->str, "SHORT") == 0) return 2;
		if (stricmp(token->str, "USHORT") == 0) return 2;
		if (stricmp(token->str, "INT16") == 0) return 2;
		if (stricmp(token->str, "UINT16") == 0) return 2;
		if (stricmp(token->str, "DWORD") == 0) return 4;
		if (stricmp(token->str, "INT") == 0) return 4;
		if (stricmp(token->str, "UINT") == 0) return 4;
		if (stricmp(token->str, "LONG") == 0) return 4;
		if (stricmp(token->str, "ULONG") == 0) return 4;
		if (stricmp(token->str, "INT32") == 0) return 4;
		if (stricmp(token->str, "UINT32") == 0) return 4;
	}
	return 0;
}

static int cparser_parse_newvar(CParser *parser, const char *name, int stack)
{
	char *macro = (char*)parser->data;
	CVariable *var;

	if (stricmp(name, "RET") == 0) {
		sprintf(macro, "'%s' conflicted with keyword", name);
		cparser_error(parser, macro, 96);
		return -1;
	}

	for (var = parser->vars; var; var = var->next) {
		if (strcmp(var->name, name) == 0) {
			sprintf(macro, "'%s' redefined", name);
			cparser_error(parser, macro, 97);
			return -2;
		}
	}

	if (stack >= 0) sprintf(macro, "[EBP + %d]", stack);
	else sprintf(macro, "[EBP - %d]", -stack);

	if (cscanner_macro_set(parser->token, name, macro)) {
		sprintf(macro, "name '%s' redefined", name);
		cparser_error(parser, macro, 95);
		return -3;
	}

	var = (CVariable*)malloc(sizeof(CVariable));
	assert(var);
	var->name = strdup(name);
	assert(var->name);
	var->pos = stack;

	var->next = parser->vars;
	parser->vars = var;

	return 0;
}

static int cparser_parse_proc(CParser *parser)
{
	unsigned char instruction[20];
	const char *name;

	if (cscanner_is_ident(parser->token) == 0) {
		return 0;
	}

	name = cscanner_get_string(parser->token);

	if (stricmp(name, "PROC") == 0) {
		const char *replace = "DB 0x8B, 0xE5, 0x5D, 0xC3\n";
		int stack = 8;

		if (parser->inproc) {
			cparser_error(parser, "cannot define proc in a proc block", 90);
			return -1;
		}
		parser->inproc = 1;
		parser->stack = 0;

		// replace ret to "mov esp, ebp; pop ebp; ret"
		cscanner_macro_set(parser->token, "ret", replace);
		cscanner_macro_set(parser->token, "RET", replace);
		cscanner_macro_set(parser->token, "Ret", replace);
		cscanner_macro_set(parser->token, "rEt", replace);
		cscanner_macro_set(parser->token, "reT", replace);
		cscanner_macro_set(parser->token, "rET", replace);
		cscanner_macro_set(parser->token, "ReT", replace);
		cscanner_macro_set(parser->token, "REt", replace);

		cscanner_token_advance(parser->token, 1);

		for (stack = 8; !cscanner_is_endl(parser->token); ) {
			const CTOKEN *token = cscanner_token_current(parser->token);
			const CTOKEN *next = cscanner_token_lookahead(parser->token);
			char *macro = (char*)parser->data;
			if (ctoken_get_char(token) == ',') {
				cscanner_token_advance(parser->token, 1);
			}
			else if (token->type == CTokenIDENT && next->ch == ':') {
				int size;
				cscanner_token_advance(parser->token, 2);
				size = cparser_parse_size(parser);
				if (size == 0) {
					cparser_error(parser, "variable type unknown", 93);
					return -1;
				}
				if (cparser_parse_newvar(parser, token->str, stack)) {
					return -4;
				}
				stack += size;
			}
			else {
				if (token->type == CTokenIDENT) {
					sprintf(macro, "parameter '%s' error", token->str);
				}	else {
					sprintf(macro, "parameter error");
				}
				cparser_error(parser, macro, 93);
				return -3;
			}
		}

		instruction[0] = 0x55;		// push ebp
		instruction[1] = 0x8B;		// mov ebp, esp
		instruction[2] = 0xEC;

		cencoding_set_data(&parser->synthesizer.encoding, instruction, 3);
	}
	else if (stricmp(name, "LOCAL") == 0) {
		int localsize = 0;
		int IS;

		cscanner_token_advance(parser->token, 1);

		if (parser->inproc == 0) {
			cparser_error(parser, "local is forbbiden outside a proc", 90);
			return -5;
		}

		for (; !cscanner_is_endl(parser->token); ) {
			const CTOKEN *token = cscanner_token_current(parser->token);
			const CTOKEN *next = cscanner_token_lookahead(parser->token);
			char *macro = (char*)parser->data;
			if (ctoken_get_char(token) == ',') {
				cscanner_token_advance(parser->token, 1);
			}
			else if (token->type == CTokenIDENT && next->ch == ':') {
				int pos, size;
				cscanner_token_advance(parser->token, 2);
				size = cparser_parse_size(parser);
				if (size == 0) {
					cparser_error(parser, "variable type unknown", 93);
					return -1;
				}
				pos = -(parser->stack + size);
				if (cparser_parse_newvar(parser, token->str, pos)) {
					return -6;
				}
				parser->stack += size;
				localsize += size;
				//printf("LOCAL %s=[EBP+(%d)]\n", token->str, pos);
			}
			else {
				if (token->type == CTokenIDENT) {
					sprintf(macro, "parameter '%s' error", token->str);
				}	else {
					sprintf(macro, "parameter error");
				}
				cparser_error(parser, macro, 92);
				return -3;
			}
		}

		if (localsize <= 127) {
			instruction[0] = 0x83;		// sub esp, imm8
			instruction[1] = 0xEC;
			instruction[2] = (unsigned char)(localsize & 0xff);
			IS = 3;
		}	else {
			instruction[0] = 0x81;		// sub esp, imm32
			instruction[1] = 0xEC;
			instruction[2] = (unsigned char)((localsize >>  0) & 0xff);
			instruction[3] = (unsigned char)((localsize >>  8) & 0xff);
			instruction[4] = (unsigned char)((localsize >> 16) & 0xff);
			instruction[5] = (unsigned char)((localsize >> 24) & 0xff);
			IS = 6;
		}

		cencoding_set_data(&parser->synthesizer.encoding, instruction, IS);
	}
	else if (stricmp(name, "ENDP") == 0) {
		if (parser->inproc == 0) {
			cparser_error(parser, "not find proc definition", 91);
			return -2;
		}
		parser->inproc = 0;
		parser->stack = 0;
		while (parser->vars) {
			CVariable *var = parser->vars;
			parser->vars = parser->vars->next;
			cscanner_macro_del(parser->token, var->name);
			free(var->name);
			free(var);
		}
		cscanner_macro_del(parser->token, "ret");
		cscanner_macro_del(parser->token, "RET");
		cscanner_macro_del(parser->token, "Ret");
		cscanner_macro_del(parser->token, "rEt");
		cscanner_macro_del(parser->token, "reT");
		cscanner_macro_del(parser->token, "rET");
		cscanner_macro_del(parser->token, "ReT");
		cscanner_macro_del(parser->token, "REt");
	}
	else {
		return 0;
	}

	while (!cscanner_is_endf(parser->token)) {
		cscanner_token_advance(parser->token, 1);
	}
	
	return 0;
}

