//=====================================================================
//
// ckeywords.h - 
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#ifndef __CKEYWORDS_H__
#define __CKEYWORDS_H__

#include "cencoding.h"

//---------------------------------------------------------------------
// CSpecifierType
//---------------------------------------------------------------------
enum CSpecifierType
{
	CS_UNKNOWN = 0,
	CS_NEAR,
	CS_SHORT = CS_NEAR,
	//	FAR,
	CS_BYTE,
	CS_WORD,
	CS_DWORD,
	CS_QWORD,
	CS_MMWORD = CS_QWORD,
	CS_XMMWORD
};

//---------------------------------------------------------------------
// CSpecifier
//---------------------------------------------------------------------
struct CSpecifier
{
	enum CSpecifierType type;
	const char *notation;
};

typedef struct CSpecifier CSpecifier;


//---------------------------------------------------------------------
// COperandType
//---------------------------------------------------------------------
enum COperandType
{
	O_UNKNOWN	= 0,

	O_VOID	= 0x00000001,

	O_ONE	= 0x00000002,
	O_IMM8	= 0x00000004 | O_ONE,
	O_IMM16	= 0x00000008 | O_IMM8 | O_ONE,
	O_IMM32	= 0x00000010 | O_IMM16 | O_IMM8 | O_ONE,
	O_IMM	= O_IMM32 | O_IMM16 | O_IMM8 | O_ONE,

	O_AL	= 0x00000020,
	O_CL	= 0x00000040,
	O_REG8	= O_CL | O_AL,

	O_AX	= 0x00000080,
	O_DX	= 0x00000100,
	O_CX	= 0x00000200,
	O_REG16	= O_CX | O_DX | O_AX,

	O_EAX		= 0x00000400,
	O_ECX		= 0x00000800,
	O_REG32	= O_ECX | O_EAX,

	// No need to touch these in 32-bit protected mode
	O_CS		= O_UNKNOWN,   
	O_DS		= O_UNKNOWN,
	O_ES		= O_UNKNOWN,
	O_SS		= O_UNKNOWN,
	O_FS		= O_UNKNOWN,
	O_GS		= O_UNKNOWN,
	O_SEGREG	= O_GS | O_FS | O_SS | O_ES | O_DS | O_CS,

	O_ST0		= 0x00001000,
	O_FPUREG	= 0x00002000 | O_ST0,

	// You won't need these in a JIT assembler
	O_CR		= O_UNKNOWN,   
	O_DR		= O_UNKNOWN,
	O_TR		= O_UNKNOWN,

	O_MMREG		= 0x00004000,
	O_XMMREG	= 0x00008000,

	O_REG		=	O_XMMREG | O_MMREG | O_TR | O_DR | O_CR | O_FPUREG | 
					O_SEGREG | O_REG32 | O_REG16 | O_REG8,
	O_MEM8		=	0x00010000,
	O_MEM16		=	0x00020000,
	O_MEM32		=	0x00040000,
	O_MEM64		=	0x00080000,
	O_MEM80		=	O_UNKNOWN,   // Extended double not supported by NT
	O_MEM128	=	0x00100000,
	O_M512B		=	O_UNKNOWN,   // Only for state save/restore instructions
	O_MEM		=	O_M512B | O_MEM128 | O_MEM80 | O_MEM64 | O_MEM32 | 
					O_MEM16 | O_MEM8,
		
	O_XMM32		=	O_MEM32 | O_XMMREG,
	O_XMM64		=	O_MEM64 | O_XMMREG,

	O_R_M8		=	O_MEM8 | O_REG8,
	O_R_M16		=	O_MEM16 | O_REG16,
	O_R_M32		=	O_MEM32 | O_REG32,
	O_R_M64		=	O_MEM64 | O_MMREG,
	O_R_M128	=	O_MEM128 | O_XMMREG,
	O_R_M		=	O_MEM | O_REG,

	O_MOFF8		=	O_UNKNOWN,   // Not supported
	O_MOFF16	=	O_UNKNOWN,   // Not supported
	O_MOFF32	=	O_UNKNOWN   // Not supported
};


//---------------------------------------------------------------------
// COperand
//---------------------------------------------------------------------
struct COperand
{
	enum COperandType type;
	const char *notation;
	union
	{
		cint32 value;			// For immediates
		enum CRegID reg;		// For registers
	};
};

typedef struct COperand COperand;


#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------
// interface
//---------------------------------------------------------------------
extern const CSpecifier cspecifier_set[];
extern const COperand cregister_set[];
extern const COperand csyntax_set[];
extern const COperand CINIT;
extern const COperand CNOT_FOUND;

enum CSpecifierType cspecifier_scan(const char *string);

int coperand_is_subtype_of(const COperand *self, enum COperandType baseType);

int coperand_type_is_void(enum COperandType type);
int coperand_type_is_imm(enum COperandType type);
int coperand_type_is_reg(enum COperandType type);
int coperand_type_is_mem(enum COperandType type);
int coperand_type_is_R_M(enum COperandType type);

int coperand_is_void(const COperand *operand);
int coperand_is_imm(const COperand *operand);
int coperand_is_reg(const COperand *operand);
int coperand_is_mem(const COperand *operand);
int coperand_is_R_M(const COperand *operand);

COperand coperand_scan_reg(const char *string);
enum COperandType coperand_scan_syntax(const char *string);


char *cstring_strip(char *str);
int cstring_strcmp(const char *s1, const char *s2, int caseoff);
char *cstring_strsep(char **stringp, const char *delim);


#ifdef __cplusplus
}
#endif


#endif


/*
		Encoding syntax:
		----------------
		+r Add register value to opcode
		/# Value for Mod R/M register field encoding
		/r Effective address encoding
		ib Byte immediate
		iw Word immediate
		id Dword immediate
		-b Byte relative address
		-i Word or dword relative address
		p0 LOCK instruction prefix (F0h)
		p2 REPNE/REPNZ instruction prefix (F2h)
		p3 REP/REPE/REPZ instruction prefix (F3h) (also SSE prefix)
		po Offset override prefix (66h)
		pa Address override prefix (67h)

	{"JMP",			"imm",						"E9 -i",			CT_CPU_8086},
	{"JMP",			"SHORT imm",				"EB -b",			CT_CPU_8086},
//	{"JMP",			"imm:imm16",				"po EA iw iw",		CT_CPU_8086},
//	{"JMP",			"imm:imm32",				"po EA id iw",		CT_CPU_386},
	{"JMP",			"mem",						"po FF /5",			CT_CPU_8086},
//	{"JMP",			"FAR mem",					"po FF /5",			CT_CPU_386},
	{"JMP",			"WORD r/m16",				"po FF /4",			CT_CPU_8086},
	{"JMP",			"DWORD r/m32",				"po FF /4",			CT_CPU_386},
	{"MOV",			"r/m8,reg8",				"88 /r",			CT_CPU_8086},
	{"MOV",			"r/m16,reg16",				"po 89 /r",			CT_CPU_8086},
	{"MOV",			"r/m32,reg32",				"po 89 /r",			CT_CPU_386},
	{"MOV",			"reg8,r/m8",				"8A /r",			CT_CPU_8086},
	{"MOV",			"reg16,r/m16",				"po 8B /r",			CT_CPU_8086},
	{"MOV",			"reg32,r/m32",				"po 8B /r",			CT_CPU_386},
	{"MOV",			"reg8,imm8",				"B0 +r ib",			CT_CPU_8086},
	{"MOV",			"reg16,imm16",				"po B8 +r iw",		CT_CPU_8086},
	{"MOV",			"reg32,imm32",				"po B8 +r id",		CT_CPU_386},
	{"MOV",			"r/m8,imm8",				"C6 /0 ib",			CT_CPU_8086},
	{"MOV",			"r/m16,imm16",				"po C7 /0 iw",		CT_CPU_8086},
	{"MOV",			"r/m32,imm32",				"po C7 /0 id",		CT_CPU_386},
*/

