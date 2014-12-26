//=====================================================================
//
// cscanner.c - source scanner
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#include "cscanner.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

//---------------------------------------------------------------------
// compatible
//---------------------------------------------------------------------
int cstricmp(const char *dst, const char *src) {
	int ch1, ch2;
	do {
		if ( ((ch1 = (unsigned char)(*(dst++))) >= 'A') && (ch1 <= 'Z') )
			ch1 += 0x20;
		if ( ((ch2 = (unsigned char)(*(src++))) >= 'A') && (ch2 <= 'Z') )
			ch2 += 0x20;
	}	while ( ch1 && (ch1 == ch2) );
	return(ch1 - ch2);
}


//---------------------------------------------------------------------
// Token Reader
//---------------------------------------------------------------------
CTokenReader *ctoken_reader_create(int (*readch)(void*), void *fp)
{
	CTokenReader *reader;
	reader = (CTokenReader*)malloc(sizeof(CTokenReader));
	assert(reader);
	reader->readch = readch;
	reader->fp = fp;
	reader->ch = ' ';
	reader->unch = -1;
	reader->saved = -1;
	reader->lineno = 1;
	reader->colno = 0;
	reader->state = 0;
	reader->buffer = (char*)malloc(CMAX_IDENT * 2);
	assert(reader->buffer);
	reader->error = (char*)malloc(8192);
	assert(reader->error);
	reader->pos = 0;
	reader->keywords = NULL;
	reader->eof = 0;
	reader->error[0] = 0;
	reader->errcode = 0;
	return reader;
}

void ctoken_reader_release(CTokenReader *reader)
{
	assert(reader);
	if (reader->buffer) {
		free(reader->buffer);
		reader->buffer = NULL;
	}
	if (reader->error) {
		free(reader->error);
		reader->error = NULL;
	}
	free(reader);
}

int ctoken_reader_getch(CTokenReader *reader)
{
	assert(reader);
	if (reader->unch >= 0) {
		reader->ch = reader->unch;
		reader->unch = -1;
	}	else {
		reader->saved = reader->ch;
		reader->ch = reader->readch(reader->fp);
		if (reader->ch == '\n') reader->lineno++, reader->colno = 1;
		else if (reader->ch >= 0) reader->colno++;
	}
	return reader->ch;
}

static int ctoken_reader_ungetch(CTokenReader *reader, int ch)
{
	assert(reader->unch < 0);
	reader->unch = ch;
	return 0;
}

static int ctoken_reader_skip_space(CTokenReader *reader)
{
	while (1) {
		int skip = 0;
		for (; isspace(reader->ch) && reader->ch != '\n'; skip++)
			ctoken_reader_getch(reader);
		if (reader->ch == ';' || reader->ch == '#') {
			skip++;
			while (reader->ch != '\n' && reader->ch >= 0) {
				ctoken_reader_getch(reader);
				skip++;
			}
		}	
		else if (reader->ch == '/') {
			ctoken_reader_getch(reader);
			if (reader->ch == '/') {
				skip++;
				while (reader->ch != '\n' && reader->ch >= 0) {
					ctoken_reader_getch(reader);
					skip++;
				}
			}	else {
				ctoken_reader_ungetch(reader, reader->ch);
				reader->ch = '/';
			}
		}
		if (skip == 0) break;
	}
	return 0;
}

static CTOKEN *ctoken_reader_read_string(CTokenReader *reader, int *state)
{
	CTOKEN *token = NULL;

	if (reader->ch == '\'' || reader->ch == '\"') {
		int mode = (reader->ch == '\"')? 0 : 1;
		reader->pos = 0;
		while (1) {
			int ch = ctoken_reader_getch(reader);
			if (ch == '\\') {
				ctoken_reader_getch(reader);
				reader->buffer[reader->pos++] = '\\';
				reader->buffer[reader->pos++] = (char)reader->ch;
			}	
			else if (mode == 0 && ch == '\'') {
				reader->buffer[reader->pos++] = '\'';
			}
			else if (mode == 1 && ch == '\"') {
				reader->buffer[reader->pos++] = '\"';
			}
			else if (mode == 0 && ch == '\"') {
				ch = ctoken_reader_getch(reader);
				if (ch == '\"') {
					reader->buffer[reader->pos++] = '\"';
					reader->buffer[reader->pos++] = '\"';
				}	else {
					*state = 1;
					reader->buffer[reader->pos] = 0;
					token = ctoken_new_string(reader->buffer);
					break;
				}
			}
			else if (mode == 1 && ch == '\'') {
				ch = ctoken_reader_getch(reader);
				if (ch == '\'') {
					reader->buffer[reader->pos++] = '\'';
					reader->buffer[reader->pos++] = '\'';
				}	else {
					*state = 1;
					reader->buffer[reader->pos] = 0;
					token = ctoken_new_string(reader->buffer);
					break;
				}
			}
			else if (ch == '\n') {
				*state = -1;
				break;
			}
			else if (ch >= 0) {
				reader->buffer[reader->pos++] = (char)ch;
			}
			else {			// ch < 0
				*state = -2;
				break;
			}

			if (reader->pos >= CMAX_IDENT) {
				*state = -3;
				strncpy(reader->error, "string too long", 64);
				break;
			}
		}
	}

	return token;
}

static CTOKEN *ctoken_reader_read_number(CTokenReader *reader, int *state)
{
	int lineno = reader->lineno;
	char *text = reader->buffer;
	CTOKEN *token;
	int ec1, ec2, pos;
	long value;

	if (reader->ch < '0' || reader->ch > '9') {
		*state = 0;
		return NULL;
	}

	for (reader->pos = 0; isalnum(reader->ch) || reader->ch == '.'; ) {
		reader->buffer[reader->pos++] = (char)reader->ch;
		ctoken_reader_getch(reader);
		if (reader->pos >= CMAX_IDENT) {
			strncpy(reader->error, "number too long", 64);
			*state = -1;
			reader->errcode = 1;
			return NULL;
		}
	}

	reader->buffer[reader->pos] = 0;
	for (pos = reader->pos; pos > 0; pos--) {
		if (isdigit(text[pos - 1]) || text[pos - 1] == '.') {
			break;
		}
		else if (text[pos - 1] >= 'a' && text[pos - 1] <= 'f') {
			break;
		}
		else if (text[pos - 1] >= 'A' && text[pos - 1] <= 'F') {
			break;
		}
	}

	if (reader->pos - pos > 2) {
		strncpy(reader->error, "number format error", 64);
		*state = -2;
		reader->errcode = 2;
		return NULL;
	}

	if (reader->pos - pos == 2) ec1 = text[pos], ec2 = text[pos + 1];
	else if (reader->pos - pos == 1) ec1 = text[pos], ec2 = 0;
	else ec1 = ec2 = 0;
	text[pos] = 0;
	
	// hex
	if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
		value = (long)strtoul(text + 2, NULL, 16);
		token = ctoken_new_int(value);
	}	// hex
	else if (ec1 == 'h' && ec2 == 0) {
		value = (long)strtoul(text, NULL, 16);
		token = ctoken_new_int(value);
	}	// binary
	else if (ec1 == 'b' && ec2 == 0) {
		value = (long)strtoul(text, NULL, 2);
		token = ctoken_new_int(value);
	}	// octal
	else if (ec1 == 'q' && ec2 == 0) {
		value = (long)strtol(text, NULL, 8);
		token = ctoken_new_int(value);
	}	// decimal or float
	else {
		int decimal = 1;
		int i;
		for (i = 0; text[i]; i++) 
			if (text[i] == '.') decimal = 0;
		if (decimal) {
			value = (long)strtoul(text, NULL, 10);
			token = ctoken_new_int(value);
		}	else {
			float ff;
			sscanf(text, "%f", &ff);
			token = ctoken_new_float(ff);
		}
	}
	token->lineno = lineno;
	*state = 0;
	return token;
}

CTOKEN *ctoken_reader_read(CTokenReader *reader)
{
	CTOKEN *token = NULL;

	assert(reader);

	// skip memo and space
	ctoken_reader_skip_space(reader);

	// this is a endl
	if (reader->ch == '\n') {
		int lineno = reader->lineno - 1;
		token = ctoken_new_endl();
		token->lineno = lineno;
		ctoken_reader_getch(reader);
		return token;
	}

	// this is a endf
	if (reader->ch < 0) {
		if (reader->eof++) return NULL;
		token = ctoken_new_endf();
		token->lineno = reader->lineno;
		return token;
	}

	// this is a string 
	if (reader->ch == '\'' || reader->ch == '\"') {
		int lineno = reader->lineno;
		int state;
		token = ctoken_reader_read_string(reader, &state);
		if (state < 0) {
			strncpy(reader->error, "expected closing quotation mark", 100);
			reader->errcode = 3;
			return NULL;
		}
		token->lineno = lineno;
		return token;
	}

	#define issym2f(c) ((c) == '_' || isalpha(c) || (c) == '$' || (c) == '@')
	#define issym2x(c) ((c) == '_' || isalnum(c) || (c) == '$' || (c) == '@')

	// this is a identity or a keyword
	if (issym2f(reader->ch)) {
		int lineno = reader->lineno;
		for (reader->pos = 0; issym2x(reader->ch); ) {
			reader->buffer[reader->pos++] = (char)reader->ch;
			ctoken_reader_getch(reader);
			if (reader->pos >= CMAX_IDENT) {
				strncpy(reader->error, "ident too long", 100);
				reader->errcode = 4;
				return NULL;
			}
		}
		reader->buffer[reader->pos] = 0;
		if (reader->keywords) {
			int i;
			for (i = 0; reader->keywords[i]; i++) {
				if (stricmp(reader->buffer, reader->keywords[i]) == 0) {
					token = ctoken_new_keyword(i);
					token->lineno = lineno;
					return token;
				}
			}
		}
		token = ctoken_new_ident(reader->buffer);
		token->lineno = lineno;
		//printf("{%s:%d:%d}\n", token->str, token->lineno, reader->lineno);
		return token;
	}

	#undef issym2f
	#undef issym2x

	// this is a number
	if (reader->ch >= '0' && reader->ch <= '9') {
		int lineno = reader->lineno;
		int state;
		//printf("number\n");
		token = ctoken_reader_read_number(reader, &state);
		if (state < 0) {
			strncpy(reader->error, "number format error", 100);
			reader->errcode = 5;
			return NULL;
		}
		token->lineno = lineno;
		return token;
	}

	// operators
	token = ctoken_new_operator(reader->ch);
	assert(token);
	token->lineno = reader->lineno;
	ctoken_reader_getch(reader);

	return token;
}


//---------------------------------------------------------------------
// token stream
//---------------------------------------------------------------------
static int ctoken_stream_text_getch(void *fp) {
	char **ptr = (char**)fp;
	if (**ptr == 0) return -1;
	return *((*ptr)++);
}

static void ctoken_stream_free(CTOKEN *root) {
	assert(root);
	while (!ctoken_list_is_empty(root)) {
		CTOKEN *token = root->next;
		ctoken_list_del(token);
		ctoken_delete(token);
	}
	ctoken_delete(root);
}

static CTOKEN *ctoken_stream_load(const char *text, char *error)
{
	CTokenReader *reader;
	CTOKEN *root;
	char *string;
	int retval;

	string = (char*)text;
	reader = ctoken_reader_create(ctoken_stream_text_getch, &string);
	assert(reader);

	root = ctoken_new_endf();
	assert(root);

	for (retval = 0, *error = 0; ; ) {
		CTOKEN *token;
		token = ctoken_reader_read(reader);
		if (token == NULL) {
			if (error) strncpy(error, reader->error, 100);
			retval = -1;
			break;
		}
		if (token->type == CTokenENDF) {
			break;
		}
		ctoken_list_add_tail(token, root);
	}

	ctoken_reader_release(reader);

	if (retval != 0) {
		ctoken_stream_free(root);
		return NULL;
	}

	return root;
}


//---------------------------------------------------------------------
// Scanner
//---------------------------------------------------------------------
CScanner *cscanner_create(void)
{
	CScanner *scan;
	scan = (CScanner*)malloc(sizeof(CScanner));
	assert(scan);
	scan->root = NULL;
	scan->link = NULL;
	scan->source = NULL;
	scan->length = 0;
	scan->position = 0;
	scan->reader = NULL;
	scan->error = (char*)malloc(1024);
	assert(scan->error);
	scan->errcode = 0;
	scan->macros = NULL;
	scan->endf.type = CTokenENDF;
	scan->endf.lineno = 0;
	scan->endf.fileno = 0;
	scan->jmplabel = 0;
	return scan;
}

static void cscanner_token_reset(CScanner *scan)
{
	assert(scan);
	if (scan->root) {
		while (!ctoken_list_is_empty(scan->root)) {
			CTOKEN *token = scan->root->next;
			ctoken_list_del(token);
			ctoken_delete(token);
		}
		ctoken_delete(scan->root);
		scan->root = NULL;
		scan->link = NULL;
	}
	if (scan->reader) {
		ctoken_reader_release(scan->reader);
		scan->reader = NULL;
	}
	scan->source = NULL;
	scan->length = 0;
	scan->position = 0;
	scan->errcode = 0;
}

void cscanner_macro_reset(CScanner *scan)
{
	while (scan->macros) {
		CMacro *macro = scan->macros;
		scan->macros = scan->macros->next;
		free(macro->ident);
		free(macro->value);
		free(macro);
	}
	scan->jmplabel = 0;
}

void cscanner_release(CScanner *scan)
{
	cscanner_token_reset(scan);
	cscanner_macro_reset(scan);
	if (scan->error) {
		free(scan->error);
		scan->error = NULL;
	}
	free(scan);
}

int cscanner_macro_set(CScanner *scan, const char *name, const char *value)
{
	CMacro *macro;

	for (macro = scan->macros; macro; macro = macro->next) {
		if (strcmp(macro->ident, name) == 0) {
			return -1;
		}
	}

	macro = (CMacro*)malloc(sizeof(CMacro));
	assert(macro);

	macro->ident = strdup(name);
	macro->value = strdup(value);

	assert(macro->ident);
	assert(macro->value);

	macro->next = scan->macros;
	scan->macros = macro;

	return 0;
}

int cscanner_macro_del(CScanner *scan, const char *name)
{
	CMacro *macro;
	CMacro *prev;

	for (macro = scan->macros, prev = NULL; macro; ) {
		if (strcmp(macro->ident, name) == 0) {
			break;
		}
		prev = macro;
		macro = macro->next;
	}

	if (macro == NULL) {		// not find macro
		return -1;
	}

	if (prev) prev->next = macro->next;
	else scan->macros = macro->next;

	free(macro->ident);
	free(macro->value);
	free(macro);

	return 0;
}

const char *cscanner_macro_search(CScanner *scan, const char *name)
{
	CMacro *macro;
	for (macro = scan->macros; macro; macro = macro->next) {
		if (strcmp(macro->ident, name) == 0) {
			return macro->value;
		}
	}

	return NULL;
}

static int cscanner_reader_getch(void *fp) 
{
	CScanner *scan = (CScanner*)fp;
	if (scan->source == NULL) return -1;
	if (scan->position >= scan->length) return -1;
	return scan->source[scan->position++];
}

int cscanner_set_source(CScanner *scan, const char *source)
{
	int retval = 0;

	cscanner_token_reset(scan);
	scan->source = source;
	scan->length = (int)strlen(source);
	scan->position = 0;
	scan->reader = ctoken_reader_create(cscanner_reader_getch, scan);
	
	scan->root = ctoken_new_endf();
	scan->link = scan->root;
	scan->error[0] = 0;
	scan->errcode = 0;

	for (; ; ) {
		CTOKEN *token;

		token = ctoken_reader_read(scan->reader);

		if (token == NULL) {
			scan->lineno = scan->reader->lineno;
			scan->errcode = scan->reader->errcode;
			strncpy(scan->error, scan->reader->error, 80);
			retval = -1;
			break;
		}

		if (token->type == CTokenIDENT) {
			const char *macro = cscanner_macro_search(scan, token->str);
			if (macro != NULL) {
				CTOKEN *ts = ctoken_stream_load(macro, scan->error);
				if (ts == NULL) {
					scan->lineno = scan->reader->lineno;
					scan->errcode = 88;
					retval = -2;
					break;
				}
				while (!ctoken_list_is_empty(ts)) {
					CTOKEN *next = ts->next;
					if (next->type == CTokenENDF) break;
					ctoken_list_del(next);
					next->lineno = scan->lineno;
					ctoken_list_add_tail(next, scan->root);
				}
				ctoken_stream_free(ts);
				continue;
			}	
			else {
				if (strcmp(token->str, "@@") == 0) {
					scan->jmplabel++;
					free(token->str);
					token->str = (char*)malloc(20);
					assert(token->str);
					sprintf(token->str, "@@%d", scan->jmplabel);
					//printf("label %d\n", scan->jmplabel);
				}
				else if (stricmp(token->str, "@b") == 0) {
					free(token->str);
					token->str = (char*)malloc(20);
					assert(token->str);
					sprintf(token->str, "@@%d", scan->jmplabel);
				}
				else if (stricmp(token->str, "@f") == 0) {
					free(token->str);
					token->str = (char*)malloc(20);
					assert(token->str);
					sprintf(token->str, "@@%d", scan->jmplabel + 1);
				}
			}
		}

		ctoken_list_add_tail(token, scan->root);
		if (token->type == CTokenENDF) {
			break;
		}
	}

	if (retval != 0) {
		cscanner_token_reset(scan);
		return retval;
	}

	scan->link = scan->root->next;
	scan->root->lineno = scan->root->prev->lineno;

	return 0;
}

const CTOKEN *cscanner_token_current(const CScanner *scan)
{
	if (scan->root == NULL || scan->link == NULL) {
		return &(scan->endf);
	}
	return scan->link;
}

const CTOKEN *cscanner_token_lookahead(const CScanner *scan)
{
	if (scan->root == NULL || scan->link == NULL) {
		return &(scan->endf);
	}
	if (scan->link == scan->root) {
		return &(scan->endf);
	}
	return scan->link->next;
}

const CTOKEN *cscanner_token_advance(CScanner *scan, int n)
{
	if (n < 0) n = 0;
	if (scan->root == NULL || scan->link == NULL) {
		return &(scan->endf);
	}
	while (n--) {
		if (scan->link == scan->root) {
			return &(scan->endf);
		}
		scan->link = scan->link->next;
	}
	return scan->link;
}

int cscanner_get_type(const CScanner *scan)
{
	return cscanner_token_current(scan)->type;
}

const char *cscanner_get_string(const CScanner *scan)
{
	const CTOKEN *token = cscanner_token_current(scan);
	if (token->type != CTokenIDENT && token->type != CTokenSTR) {
		return "";
	}
	return token->str;
}

int cscanner_get_char(const CScanner *scan)
{
	const CTOKEN *token = cscanner_token_current(scan);
	if (token->type != CTokenOPERATOR) {
		return '\0';
	}
	return token->ch;
}

int cscanner_get_value(const CScanner *scan)
{
	const CTOKEN *token = cscanner_token_current(scan);
	if (token->type != CTokenINT) {
		return 0;
	}
	return token->intval;
}

int cscanner_get_lineno(const CScanner *scan)
{
	const CTOKEN *token = cscanner_token_current(scan);
	return token->lineno;
}

int cscanner_is_endl(const CScanner *scan) {
	const CTOKEN *token = cscanner_token_current(scan);
	return (token->type == CTokenENDF || token->type == CTokenENDL);
}

int cscanner_is_endf(const CScanner *scan) {
	return cscanner_get_type(scan) == CTokenENDF;
}

int cscanner_is_ident(const CScanner *scan)  {
	return cscanner_get_type(scan) == CTokenIDENT;
}

int cscanner_is_operator(const CScanner *scan) {
	return cscanner_get_type(scan) == CTokenOPERATOR;
}

int cscanner_is_int(const CScanner *scan) {
	return cscanner_get_type(scan) == CTokenINT;
}

int cscanner_is_string(const CScanner *scan) {
	return cscanner_get_type(scan) == CTokenSTR;
}

