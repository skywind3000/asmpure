//=====================================================================
//
// cinstruct.h - 
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#ifndef __CINSTRUCT_H__
#define __CINSTRUCT_H__

#include "ckeywords.h"

//---------------------------------------------------------------------
// CInstructionType
//---------------------------------------------------------------------
enum CInstructionType
{
	CT_CPU_UNKNOWN	= 0x00000000,

	CT_CPU_8086		= 0x00000001,
	CT_CPU_186		= 0x00000002,
	CT_CPU_286		= 0x00000004,
	CT_CPU_386		= 0x00000008,
	CT_CPU_486		= 0x00000010,
	CT_CPU_PENT		= 0x00000020,   // Pentium
	CT_CPU_P6		= 0x00000040,   // Pentium Pro

	CT_CPU_FPU		= 0x00000080,
	CT_CPU_MMX		= 0x00000100,
	CT_CPU_KATMAI	= 0x00000200,
	CT_CPU_SSE		= 0x00000400,

//	CT_CPU_AMD		= 0x00000800,   // AMD specific system calls
	CT_CPU_CYRIX	= 0x00001000,
	CT_CPU_3DNOW	= 0x00002000,
	CT_CPU_ATHLON	= 0x00004000,
//	CT_CPU_SMM		= 0x00008000,   // System Management Mode, standby mode

	CT_CPU_P7		= 0x00010000 | CT_CPU_SSE,
	CT_CPU_WILLAMETTE = CT_CPU_P7,
	CT_CPU_SSE2		= 0x00020000 | CT_CPU_WILLAMETTE,
	CT_CPU_PNI		= 0x00040000,
	CT_CPU_SSE3		= 0x00080000,

// Undocumented, also not supported by inline assembler
//	CT_CPU_UNDOC	= 0x00010000,  
// Priviledged, run-time compiled OS kernel anyone?
//	CT_CPU_PRIV	= 0x00020000    
};


//---------------------------------------------------------------------
// CInstructionSyntax
//---------------------------------------------------------------------
struct CInstSyntax
{
	const char *mnemonic;
	const char *operands;
	const char *encoding;
	int flags;
};

typedef struct CInstSyntax CInstSyntax;


//---------------------------------------------------------------------
// CInstruction
//---------------------------------------------------------------------
struct CInstruction
{
	int syntaxMnemonic : 1;
	int syntaxSpecifier : 1;
	int syntaxFirstOperand : 1;
	int syntaxSecondOperand : 1;
	int syntaxThirdOperand : 1;

	const struct CInstSyntax *syntax;
	enum CSpecifierType specifier;
	enum COperandType firstOperand;
	enum COperandType secondOperand;
	enum COperandType thirdOperand;
	int flags;

	struct CInstruction *next;
};

typedef struct CInstruction CInstruction;


#ifdef __cplusplus
extern "C" {
#endif


CInstruction *cinst_create(const CInstSyntax *syntax);
void cinst_release(CInstruction *self);

CInstruction *cinst_get_next(CInstruction *self);

void cinst_attach_new(CInstruction *self, const CInstSyntax *instruction);
		
void cinst_reset_match(CInstruction *self);
int cinst_match_syntax(CInstruction *self);
void cinst_match_mnemonic(CInstruction *self, const char *mnemonic);
void cinst_match_specifier(CInstruction *self, enum CSpecifierType sizeSpec);
void cinst_match_first_operand(CInstruction *self, const COperand *operand);
void cinst_match_second_operand(CInstruction *self, const COperand *operand);
void cinst_match_third_operand(CInstruction *self, const COperand *operand);

enum COperandType cinst_getFirstOperand(CInstruction *self);
enum COperandType cinst_getSecondOperand(CInstruction *self);
enum COperandType cinst_getThirdOperand(CInstruction *self);

const char *cinst_getMnemonic(CInstruction *self);
const char *cinst_getOperandSyntax(CInstruction *self);
const char *cinst_getEncoding(CInstruction *self);
		
int cinst_is_32bit(CInstruction *self);



#ifdef __cplusplus
}
#endif

#endif


