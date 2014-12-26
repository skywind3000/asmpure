//=====================================================================
//
// cinstset.h - 
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#ifndef __CINSTSET_H__
#define __CINSTSET_H__

#include "cinstruct.h"


struct CInstructionEntry
{
	const char *mnemonic;
	CInstruction *instruction;
};

typedef struct CInstructionEntry CInstructionEntry;

struct CInstructionSet
{
	CInstructionEntry *instructionMap;
};

typedef struct CInstructionSet CInstructionSet;


#ifdef __cplusplus
extern "C" {
#endif

extern CInstSyntax cinstruction_set[];

int cinstset_num_instructions(void);
int cinstset_num_mnemonics(void);

CInstructionSet *cinstset_create(void);
void cinstset_release(CInstructionSet *self);

CInstruction *cinstset_query(const CInstructionSet *self, const char *name);


#ifdef __cplusplus
}
#endif

#endif


