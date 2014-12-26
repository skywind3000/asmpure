//=====================================================================
//
// ctoken.c - token definition
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "ctoken.h"


//---------------------------------------------------------------------
// create a new token
//---------------------------------------------------------------------
CTOKEN *ctoken_new(enum CTokenType type, const void *str, int size)
{
	CTOKEN *token;

	token = (CTOKEN*)malloc(sizeof(CTOKEN));
	assert(token);

	token->type = type;
	token->str = NULL;
	token->size = 0;
	token->lineno = -1;
	token->fileno = -1;
	token->keyword = -1;

	switch (type)
	{
	case CTokenENDL:
		break;
	case CTokenENDF:
		break;
	case CTokenSTR:
	case CTokenIDENT:
		token->str = (char*)malloc(size + 1);
		assert(token->str);
		memcpy(token->str, str, size);
		token->str[size] = 0;
		token->size = size;
		break;
	case CTokenKEYWORD:
		token->keyword = *(int*)str;
		break;
	case CTokenOPERATOR:
		token->ch = *(int*)str;
		break;
	case CTokenINT:
		token->intval = *(long*)str;
		break;
	case CTokenFLOAT:
		token->fltval = *(double*)str;
		break;
	default:
		token->type = CTokenERROR;
		token->errcode = *(int*)str;
		break;
	}

	token->next = token;
	token->prev = token;

	return token;
}

//---------------------------------------------------------------------
// release token
//---------------------------------------------------------------------
void ctoken_delete(CTOKEN *token)
{
	if (token->type == CTokenSTR || token->type == CTokenIDENT) {
		if (token->str) free(token->str);
		token->str = NULL;
	}
	token->type = CTokenERROR;
	free(token);
}

//---------------------------------------------------------------------
// create a new endl
//---------------------------------------------------------------------
CTOKEN *ctoken_new_endl(void) {
	return ctoken_new(CTokenENDL, NULL, 0);
}

//---------------------------------------------------------------------
// create a new endf
//---------------------------------------------------------------------
CTOKEN *ctoken_new_endf(void) {
	return ctoken_new(CTokenENDF, NULL, 0);
}

//---------------------------------------------------------------------
// create a new identity
//---------------------------------------------------------------------
CTOKEN *ctoken_new_ident(const char *ident) {
	return ctoken_new(CTokenIDENT, ident, (int)strlen(ident));
}

//---------------------------------------------------------------------
// create a new keyword
//---------------------------------------------------------------------
CTOKEN *ctoken_new_keyword(int keyid) {
	return ctoken_new(CTokenKEYWORD, &keyid, sizeof(int));
}

//---------------------------------------------------------------------
// create a new string
//---------------------------------------------------------------------
CTOKEN *ctoken_new_string(const char *string) {
	return ctoken_new(CTokenSTR, string, (int)strlen(string));
}

//---------------------------------------------------------------------
// create a new integer
//---------------------------------------------------------------------
CTOKEN *ctoken_new_int(long intval) {
	return ctoken_new(CTokenINT, &intval, sizeof(long));
}

//---------------------------------------------------------------------
// create a new float
//---------------------------------------------------------------------
CTOKEN *ctoken_new_float(double fltval) {
	return ctoken_new(CTokenFLOAT, &fltval, sizeof(double));
}

//---------------------------------------------------------------------
// create a new operator
//---------------------------------------------------------------------
CTOKEN *ctoken_new_operator(int op) {
	return ctoken_new(CTokenOPERATOR, &op, sizeof(int));
}

//---------------------------------------------------------------------
// create a new error
//---------------------------------------------------------------------
CTOKEN *ctoken_new_error(int code) {
	return ctoken_new(CTokenERROR, &code, sizeof(int));
}

//---------------------------------------------------------------------
// token copy
//---------------------------------------------------------------------
CTOKEN *ctoken_new_copy(const CTOKEN *token) {
	CTOKEN *newtoken;

	newtoken = (CTOKEN*)malloc(sizeof(CTOKEN));
	assert(newtoken);

	newtoken->type = token->type;
	newtoken->str = NULL;
	newtoken->size = 0;
	newtoken->lineno = token->lineno;
	newtoken->fileno = token->fileno;

	switch (newtoken->type)
	{
	case CTokenSTR:
	case CTokenIDENT:
		newtoken->str = (char*)malloc(token->size + 1);
		assert(newtoken->str);
		memcpy(newtoken->str, token->str, token->size);
		newtoken->str[token->size] = 0;
		newtoken->size = token->size;
		break;
	case CTokenKEYWORD:
		newtoken->keyword = token->keyword;
		break;
	case CTokenOPERATOR:
		newtoken->ch = token->ch;
		break;
	case CTokenINT:
		newtoken->intval = token->intval;
		break;
	case CTokenFLOAT:
		newtoken->fltval = token->fltval;
		break;
	default:
		break;
	}

	newtoken->next = newtoken;
	newtoken->prev = newtoken;

	return newtoken;
}


//---------------------------------------------------------------------
// get string
//---------------------------------------------------------------------
const char *ctoken_get_string(const CTOKEN *token)
{
	if (token->type != CTokenIDENT && token->type != CTokenSTR) {
		return "";
	}
	return token->str;
}

//---------------------------------------------------------------------
// get integer
//---------------------------------------------------------------------
long ctoken_get_int(const CTOKEN *token)
{
	if (token->type != CTokenINT) {
		return 0;
	}
	return token->intval;
}

//---------------------------------------------------------------------
// get char
//---------------------------------------------------------------------
int ctoken_get_char(const CTOKEN *token)
{
	if (token->type != CTokenOPERATOR) {
		return '\0';
	}
	return token->ch;
}

//---------------------------------------------------------------------
// get float
//---------------------------------------------------------------------
double ctoken_get_float(const CTOKEN *token)
{
	if (token->type != CTokenFLOAT) {
		return 0.0;
	}
	return token->fltval;
}

//---------------------------------------------------------------------
// get keyword
//---------------------------------------------------------------------
int ctoken_get_keyword(const CTOKEN *token)
{
	if (token->type != CTokenKEYWORD) {
		return -1;
	}
	return token->keyword;
}


int ctoken_is_endl(const CTOKEN *token) {
	return token->type == CTokenENDL;
}

int ctoken_is_endf(const CTOKEN *token) {
	return token->type == CTokenENDF;
}

int ctoken_is_ident(const CTOKEN *token) {
	return token->type == CTokenIDENT;
}

int ctoken_is_keyword(const CTOKEN *token) {
	return token->type == CTokenKEYWORD;
}

int ctoken_is_string(const CTOKEN *token) {
	return token->type == CTokenSTR;
}

int ctoken_is_int(const CTOKEN *token) {
	return token->type == CTokenINT;
}

int ctoken_is_float(const CTOKEN *token) {
	return token->type == CTokenFLOAT;
}

int ctoken_is_operator(const CTOKEN *token) {
	return token->type == CTokenOPERATOR;
}

int ctoken_is_error(const CTOKEN *token) {
	return token->type == CTokenERROR;
}


//---------------------------------------------------------------------
// add node to head
//---------------------------------------------------------------------
void ctoken_list_add(CTOKEN *node, CTOKEN *head)
{
	(node)->prev = (head), (node)->next = (head)->next;
	(head)->next->prev = (node), (head)->next = (node);
}

//---------------------------------------------------------------------
// add node to head's tail
//---------------------------------------------------------------------
void ctoken_list_add_tail(CTOKEN *node, CTOKEN *head)
{
	(node)->prev = (head)->prev, (node)->next = (head);
	(head)->prev->next = (node), (head)->prev = (node);
}

//---------------------------------------------------------------------
// delete between
//---------------------------------------------------------------------
void ctoken_list_del_between(CTOKEN *p, CTOKEN *n)
{
	(n)->prev = (p), (p)->next = (n);
}

//---------------------------------------------------------------------
// remove self
//---------------------------------------------------------------------
void ctoken_list_del(CTOKEN *entry)
{
	(entry)->next->prev = (entry)->prev;
	(entry)->prev->next = (entry)->next;
	(entry)->next = 0, (entry)->prev = 0;
	(entry)->next = entry;
	(entry)->prev = entry;
}

//---------------------------------------------------------------------
// check if empty
//---------------------------------------------------------------------
int ctoken_list_is_empty(const CTOKEN *entry)
{
	return (entry) == (entry)->next;
}

//---------------------------------------------------------------------
// print to file
//---------------------------------------------------------------------
void ctoken_print(FILE *fp, const CTOKEN *token)
{
	if (fp == NULL) fp = stdout;
	if (token->type == CTokenIDENT) {
		fprintf(fp, "(%s)", token->str);
	}
	else if (token->type == CTokenSTR) {
		fprintf(fp, "(\"%s\")", token->str);
	}
	else if (token->type == CTokenENDL) {
		fprintf(fp, "ENDL");
	}
	else if (token->type == CTokenENDF) {
		fprintf(fp, "ENDF");
	}
	else if (token->type == CTokenKEYWORD) {
		fprintf(fp, "<%d>", token->keyword);
	}
	else if (token->type == CTokenINT) {
		fprintf(fp, "[%ld]", token->intval);
	}
	else if (token->type == CTokenFLOAT) {
		fprintf(fp, "[%f]", token->fltval);
	}
	else if (token->type == CTokenOPERATOR) {
		fprintf(fp, "[%c]", (char)token->ch);
	}
	else if (token->type == CTokenERROR) {
		fprintf(fp, "ERROR");
	}
	fflush(fp);
}

