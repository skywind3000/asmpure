//=====================================================================
//
// casmpure.c - assembly pure compiler
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#include "casmpure.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define IMAX_LINESIZE		4096

//---------------------------------------------------------------------
// CORE INTERFACE
//---------------------------------------------------------------------

// create assembler
CAssembler *casm_create(void)
{
	CAssembler *self;
	self = (CAssembler*)malloc(sizeof(CAssembler));
	assert(self);
	self->parser = cparser_create();
	assert(self->parser);
	self->loader = cloader_create();
	assert(self->loader);
	self->source = (char*)malloc(1024 + 1);
	assert(self->source);
	self->srcblock = 1024;
	self->srcsize = 0;
	self->source[0] = 0;
	self->line = (char*)malloc(IMAX_LINESIZE + 10);
	assert(self->line);
	self->error = (char*)malloc(2048);
	assert(self->error);
	self->error[0] = 0;
	self->errcode = 0;
	return self;
}

// reset assembler
void casm_reset(CAssembler *self)
{
	assert(self);
	if (self->source) free(self->source);
	self->source = (char*)malloc(1024 + 1);
	assert(self->source);
	self->srcblock = 1024;
	self->srcsize = 0;
	self->source[0] = 0;
	cloader_reset(self->loader);
	cparser_reset(self->parser);
}

// release assembler
void casm_release(CAssembler *self)
{
	assert(self);
	if (self->parser) {
		cparser_release(self->parser);
		self->parser = NULL;
	}
	if (self->loader) {
		cloader_release(self->loader);
		self->loader = NULL;
	}
	if (self->source) {
		free(self->source);
		self->source = NULL;
	}
	if (self->line) {
		free(self->line);
		self->line = NULL;
	}
	if (self->error) {
		free(self->error);
		self->error = NULL;
	}
	self->srcblock = 0;
	self->srcsize = 0;
	free(self);
}

// add source to source buffer
int casm_source(CAssembler *self, const char *text)
{
	int datasize = (int)strlen(text);
	int newsize = datasize + self->srcsize;
	int newblock = 1;
	while (newblock < newsize) newblock <<= 1;
	if (newblock != self->srcblock) {
		char *buffer = (char*)malloc(newblock + 1);
		assert(buffer);
		memcpy(buffer, self->source, self->srcsize);
		buffer[self->srcsize] = 0;
		free(self->source);
		self->source = buffer;
		self->srcblock = newblock;
	}
	memcpy(self->source + self->srcsize, text, datasize);
	self->srcsize = newsize;
	self->source[newsize] = 0;
	return 0;
}

// prompt error
static void casm_error(CAssembler *self, const char *msg, int code)
{
	sprintf(self->error, "line(%d): error(%d): %s", self->lineno, code, msg);
	self->errcode = code;
}

// compile single line
static int casm_compile_line(CAssembler *self, const char *line)
{
	const CEncoding *encoding;

	assert(self);

	self->error[0] = 0;
	self->errcode = 0;

	encoding = cparser_parse_line(self->parser, line);

	if (encoding == NULL) {
		casm_error(self, self->parser->error, self->parser->errcode);
		return -1;
	}

	cloader_new_encoding(self->loader, encoding);

	return 0;
}

// compile source buffer
// if (code == NULL) returns compiled code size
// if (code != NULL) and (maxsize >= codesize) compile and returns codesize
// if (code != NULL) and (maxsize < codesize) returns error
int casm_compile(CAssembler *self, unsigned char *code, long maxsize)
{
	int lineno, p1, p2;
	const char *text;
	long codesize;

	assert(self);

	text = self->source;

	cloader_reset(self->loader);
	cparser_reset(self->parser);

	for (lineno = 1, p1 = 0; p1 < self->srcsize; ) {
		for (p2 = p1; text[p2] != 0 && text[p2] != '\n'; p2++);
		self->lineno = lineno++;
		if (p2 - p1 >= IMAX_LINESIZE) {
			casm_error(self, "line size too long", 1);
			return -1;
		}

		memcpy(self->line, self->source + p1, p2 - p1);
		self->line[p2 - p1] = 0;
		p1 = p2 + 1;

		if (casm_compile_line(self, self->line) != 0) {
			return -2;
		}
	}

	codesize = cloader_get_codesize(self->loader) + 10;

	if (code == NULL) 
		return codesize;

	if (maxsize < codesize) {
		casm_error(self, "need a larger memory block to get code", 2);
		return -3;
	}

	memset(code, 0xcc, codesize);

	if (cloader_output(self->loader, code) != 0) {
		self->lineno = self->loader->errcode;
		casm_error(self, self->loader->error, 3);
		return -4;
	}

	return codesize;
}


// get error
const char *casm_geterror(const CAssembler *self, int *errcode)
{
	if (errcode) *errcode = self->errcode;
	return self->error;
}


//---------------------------------------------------------------------
// HIGH LEVEL
//---------------------------------------------------------------------
int casm_pushline(CAssembler *self, const char *fmt, ...)
{
	char *buffer = self->error;
	va_list argptr;

	va_start(argptr, fmt);
	vsprintf(buffer, fmt, argptr);
	va_end(argptr);

	casm_source(self, buffer);
	casm_source(self, "\n");

	self->error[0] = 0;

	return 0;
}


void *casm_callable(CAssembler *self, long *codesize)
{
	unsigned char *code;
	long size;

	if (codesize) *codesize = 0;

	size = casm_compile(self, NULL, 0);

	if (size < 0) {
		return NULL;
	}

	code = (unsigned char*)malloc(size + 1);
	assert(code);

	if (casm_compile(self, code, size) < 0) {
		free(code);
		return NULL;
	}

	if (codesize) *codesize = size;

	return code;
}


// load assembly source file
int casm_loadfile(CAssembler *self, const char *filename)
{
	char line[80];
	FILE *fp;
	casm_reset(self);
	if ((fp = fopen(filename, "r")) == NULL) 
		return -1;
	while (!feof(fp)) {
		int size = (int)fread(line, 1, 60, fp);
		line[size] = 0;
		casm_source(self, line);
	}
	fclose(fp);
	return 0;
}

int casm_savefile(CAssembler *self, const char *filename)
{
	char *codedata, *p;
	long codesize;
	FILE *fp;

	codedata = (char*)casm_callable(self, &codesize);
	if (codedata == NULL) return -1;

	if ((fp = fopen(filename, "wb")) == NULL) {
		free(codedata);
		return -2;
	}

	for (p = codedata; p < codedata + codesize; ) {
		int canwrite = codesize - (int)(p - codedata);
		int hr = (int)fwrite(p, 1, canwrite, fp);
		if (hr > 0) p += hr;
	}

	fclose(fp);

	free(codedata);

	return 0;
}

int casm_dumpinst(CAssembler *self, FILE *fp)
{
	CLoader *loader = self->loader;
	int lineno, p1, p2, maxsize, pos;
	const char *text;
	char *codedata;
	iqueue_head *node;

	text = self->source;

	codedata = (char*)casm_callable(self, NULL);
	if (codedata == NULL) return -1;
	free(codedata);

	node = loader->head.next;

	for (maxsize = 0; node != &loader->head; node = node->next) {
		CLink *link = iqueue_entry(node, CLink, head);
		int length = cencoding_length(&link->encoding);
		if (length > maxsize) maxsize = length;
	}

	node = loader->head.next;
	fp = (fp != NULL)? fp : stdout;

	for (lineno = 1, p1 = 0, pos = 0; p1 < self->srcsize; lineno++) {
		for (p2 = p1; text[p2] != 0 && text[p2] != '\n'; p2++);
		if (p2 - p1 >= IMAX_LINESIZE) {
			casm_error(self, "line size too long", 1);
			return -1;
		}

		memcpy(self->line, self->source + p1, p2 - p1);
		self->line[p2 - p1] = 0;
		p1 = p2 + 1;

		while (node != &loader->head) {
			CLink *link = iqueue_entry(node, CLink, head);
			if (link->lineno >= lineno) break;
			node = node->next;
		}

		if (node != &loader->head) {
			CLink *link = iqueue_entry(node, CLink, head);
			if (link->lineno == lineno) {
				static char output[4096];
				int length, size;
				length = cencoding_length(&link->encoding);
				if (link->encoding.align > 0) {
					int align, i, k;
					align = link->encoding.align;
					length = align - (pos % align);
					for (i = length, k = 0; i > 0; ) {
						if (i >= 2) {
							output[k++] = '6';
							output[k++] = '6';
							output[k++] = ' ';
							i--;
						}
						output[k++] = '9';
						output[k++] = '0';
						output[k++] = ' ';
						i--;
					}
					output[k++] = 0;
				}	else {
					cencoding_to_string(&link->encoding, output);
				}
				for (size = (int)strlen(output); size < (maxsize) * 3; )
					output[size++] = ' ';
				output[size] = 0;
				if (length == 0) fprintf(fp, "         ");
				else fprintf(fp, "%08X:", pos);
				pos += length;
				fprintf(fp, "  %s\t%s\n", output, self->line);
			}
		}
	}

	return 0;
}


