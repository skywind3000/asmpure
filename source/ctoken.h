//=====================================================================
//
// ctoken.h - token definition
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#ifndef __CTOKEN_H__
#define __CTOKEN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

//---------------------------------------------------------------------
// TOKEN Type
//---------------------------------------------------------------------
enum CTokenType
{
	CTokenENDL		= 0,
	CTokenENDF		= 1,
	CTokenIDENT		= 2,
	CTokenKEYWORD	= 3,
	CTokenSTR		= 4,
	CTokenOPERATOR	= 5,
	CTokenINT		= 6,
	CTokenFLOAT		= 7,
	CTokenERROR		= 8,
};


//---------------------------------------------------------------------
// CTOKEN DEFINITION
//---------------------------------------------------------------------
struct CTOKEN
{
	enum CTokenType type;
	union {
		long intval; 
		double fltval;
		int keyword;
		int ch;
		int errcode;
		char *str;
	};
	long size;
	int lineno;
	int fileno;
	struct CTOKEN *next;
	struct CTOKEN *prev;
};

typedef struct CTOKEN CTOKEN;

#define ctoken_type(token) ((token)->type)
#define ctoken_int(token) ((token)->intval)
#define ctoken_str(token) ((token)->str)
#define ctoken_chr(token) ((token)->ch)
#define ctoken_len(token) ((token)->size)
#define ctoken_key(token) ((token)->keyword)


#ifdef __cplusplus
extern "C" {
#endif


//---------------------------------------------------------------------
// BASIC INTERFACE
//---------------------------------------------------------------------
// create a new token
CTOKEN *ctoken_new(enum CTokenType type, const void *data, int size);

// delete and free memory
void ctoken_delete(CTOKEN *token);


CTOKEN *ctoken_new_endl(void);                  // create a new endl
CTOKEN *ctoken_new_endf(void);                  // create a new endf
CTOKEN *ctoken_new_ident(const char *ident);    // create a new identity
CTOKEN *ctoken_new_keyword(int keyid);          // create a new keyword
CTOKEN *ctoken_new_string(const char *string);  // create a new string
CTOKEN *ctoken_new_int(long intval);            // create a new integer
CTOKEN *ctoken_new_float(double fltval);        // create a new float
CTOKEN *ctoken_new_operator(int op);            // create a new operator
CTOKEN *ctoken_new_error(int errcode);          // create a new errcode
CTOKEN *ctoken_new_copy(const CTOKEN *token);   // create a new copy


//---------------------------------------------------------------------
// type & value operation
//---------------------------------------------------------------------
const char *ctoken_get_string(const CTOKEN *token);  // get string
long ctoken_get_int(const CTOKEN *token);            // get integer value
int ctoken_get_char(const CTOKEN *token);            // get operator char
double ctoken_get_float(const CTOKEN *token);        // get float value
int ctoken_get_keyword(const CTOKEN *token);         // get keyword

int ctoken_is_endl(const CTOKEN *token);
int ctoken_is_endf(const CTOKEN *token);
int ctoken_is_ident(const CTOKEN *token);
int ctoken_is_keyword(const CTOKEN *token);
int ctoken_is_string(const CTOKEN *token);
int ctoken_is_int(const CTOKEN *token);
int ctoken_is_float(const CTOKEN *token);
int ctoken_is_operator(const CTOKEN *token);
int ctoken_is_error(const CTOKEN *token);


//---------------------------------------------------------------------
// list operation
//---------------------------------------------------------------------
void ctoken_list_add(CTOKEN *node, CTOKEN *head);
void ctoken_list_add_tail(CTOKEN *node, CTOKEN *head);
void ctoken_list_del_between(CTOKEN *p, CTOKEN *n);
void ctoken_list_del(CTOKEN *p);
int ctoken_list_is_empty(const CTOKEN *p);


//---------------------------------------------------------------------
// misc
//---------------------------------------------------------------------
void ctoken_print(FILE *fp, const CTOKEN *token);


#ifdef __cplusplus
}
#endif

#endif


