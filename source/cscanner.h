//=====================================================================
//
// cscanner.h - source scanner
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#ifndef __CSCANNER_H__
#define __CSCANNER_H__

#include "ctoken.h"


#define CMAX_IDENT	8192

//---------------------------------------------------------------------
// CTokenReader (assembly)
//---------------------------------------------------------------------
struct CTokenReader
{
	int (*readch)(void *fp);
	void *fp;
	int ch;
	int unch;
	int saved;
	char *buffer;
	char *error;
	char **keywords;
	int state;
	int pos;
	int lineno;
	int eof;
	int colno;
	int errcode;
};

typedef struct CTokenReader CTokenReader;

#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------
// Assembly Token Reader
//---------------------------------------------------------------------
CTokenReader *ctoken_reader_create(int (*getch)(void*), void *fp);

void ctoken_reader_release(CTokenReader *reader);

CTOKEN *ctoken_reader_read(CTokenReader *reader);

#ifdef __cplusplus
}
#endif


//---------------------------------------------------------------------
// CMacro
//---------------------------------------------------------------------
struct CMacro
{
	char *ident;
	char *value;
	struct CMacro *next;
};

typedef struct CMacro CMacro;

//---------------------------------------------------------------------
// CScanner
//---------------------------------------------------------------------
struct CScanner
{
	const char *source;
	long length;
	long position;
	int errcode;
	char *error;
	int jmplabel;
	int lineno;
	CTOKEN endf;
	CTOKEN *root;
	CMacro *macros;
	const CTOKEN *link;
	CTokenReader *reader;
};

typedef struct CScanner CScanner;


#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------
// Scanner
//---------------------------------------------------------------------
CScanner *cscanner_create(void);

void cscanner_release(CScanner *scan);

void cscanner_macro_reset(CScanner *scan);

int cscanner_macro_set(CScanner *scan, const char *name, const char *value);
int cscanner_macro_del(CScanner *scan, const char *name);

int cscanner_set_source(CScanner *scan, const char *source);

const CTOKEN *cscanner_token_current(const CScanner *scan);
const CTOKEN *cscanner_token_lookahead(const CScanner *scan);
const CTOKEN *cscanner_token_advance(CScanner *scan, int n);

int cscanner_get_type(const CScanner *scan);
const char *cscanner_get_string(const CScanner *scan);
int cscanner_get_char(const CScanner *scan);
int cscanner_get_value(const CScanner *scan);
int cscanner_get_lineno(const CScanner *scan);

int cscanner_is_endl(const CScanner *scan);
int cscanner_is_endf(const CScanner *scan);
int cscanner_is_ident(const CScanner *scan);
int cscanner_is_operator(const CScanner *scan);
int cscanner_is_int(const CScanner *scan);
int cscanner_is_string(const CScanner *scan);


#ifdef __cplusplus
}
#endif

#endif



