//=====================================================================
//
// cparser.h - source parser
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#ifndef __CPARSER_H__
#define __CPARSER_H__

#include "csynthesis.h"
#include "cinstset.h"
#include "cscanner.h"


//---------------------------------------------------------------------
// CVariable
//---------------------------------------------------------------------
struct CVariable
{
	char *name;
	int pos;
	struct CVariable *next;
};

typedef struct CVariable CVariable;


//---------------------------------------------------------------------
// CParser
//---------------------------------------------------------------------
struct CParser
{
	char *data;
	char *error;
	int errcode;
	int inproc;
	int stack;
	CScanner *token;
	CVariable *vars;
	CInstruction *instruction;
	CInstructionSet *instructionset;
	CSynthesizer synthesizer;
};

typedef struct CParser CParser;


#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------
// interfaces
//---------------------------------------------------------------------
CParser *cparser_create(void);
void cparser_release(CParser *parser);

void cparser_reset(CParser *parser);

const CEncoding *cparser_parse_line(CParser *parser, const char *source);


#ifdef __cplusplus
}
#endif


#endif



