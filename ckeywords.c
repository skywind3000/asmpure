#include "ckeywords.h"


const CSpecifier cspecifier_set[] =
{
	{CS_UNKNOWN,	""},
	{CS_NEAR,		"NEAR"},
	{CS_SHORT,		"SHORT"},
//	{FAR,			"FAR"},
	{CS_BYTE,		"BYTE"},
	{CS_WORD,		"WORD"},
	{CS_DWORD,		"DWORD"},
	{CS_QWORD,		"QWORD"},
	{CS_MMWORD,		"MMWORD"},
	{CS_XMMWORD,	"XMMWORD"},
};

enum CSpecifierType cspecifier_scan(const char *string)
{
	if (string) {
		int i;
		for(i = 0; i < sizeof(cspecifier_set) / sizeof(CSpecifier); i++) {
			if(cstring_strcmp(string, cspecifier_set[i].notation, 1) == 0) {
				return cspecifier_set[i].type;
			}		
		}
	}
	return CS_UNKNOWN;
}


int coperand_is_subtype_of(const COperand *self, enum COperandType baseType)
{
	return (self->type & baseType) == self->type;
}

int coperand_type_is_void(enum COperandType type)
{
	return type == O_VOID;
}

int coperand_type_is_imm(enum COperandType type)
{
	return (type & O_IMM) == type;
}

int coperand_type_is_reg(enum COperandType type)
{
	return (type & O_REG) == type;
}

int coperand_type_is_mem(enum COperandType type)
{
	return (type & O_MEM) == type;
}

int coperand_type_is_R_M(enum COperandType type)
{
	return (type & O_R_M) == type;
}

int coperand_is_void(const COperand *operand)
{
	return coperand_type_is_void(operand->type);
}

int coperand_is_imm(const COperand *operand)
{
	return coperand_type_is_imm(operand->type);
}

int coperand_is_reg(const COperand *operand)
{
	return coperand_type_is_reg(operand->type);
}

int coperand_is_mem(const COperand *operand)
{
	return coperand_type_is_mem(operand->type);
}

int coperand_is_R_M(const COperand *operand)
{
	return coperand_type_is_R_M(operand->type);
}

const COperand cregister_set[] = 
{
	{O_VOID,		""},

	{O_AL,		"AL", { 0 } },
	{O_CL,		"CL", { 1 } },
	{O_REG8,	"DL", { 2 } },
	{O_REG8,	"BL", { 3 } },
	{O_REG8,	"AH", { 4 } },
	{O_REG8,	"CH", { 5 } },
	{O_REG8,	"DH", { 6 } },
	{O_REG8,	"BH", { 7 } },

	{O_AX,		"AX", { 0 } },
	{O_CX,		"CX", { 1 } },
	{O_DX,		"DX", { 2 } },
	{O_REG16,	"BX", { 3 } },
	{O_REG16,	"SP", { 4 } },
	{O_REG16,	"BP", { 5 } },
	{O_REG16,	"SI", { 6 } },
	{O_REG16,	"DI", { 7 } },

	{O_EAX,		"EAX", { 0 } },
	{O_ECX,		"ECX", { 1 } },
	{O_REG32,	"EDX", { 2 } },
	{O_REG32,	"EBX", { 3 } },
	{O_REG32,	"ESP", { 4 } },
	{O_REG32,	"EBP", { 5 } },
	{O_REG32,	"ESI", { 6 } },
	{O_REG32,	"EDI", { 7 } },

	{O_ES,		"ES", { 0 } },
	{O_CS,		"CS", { 1 } },
	{O_SS,		"SS", { 2 } },
	{O_DS,		"DS", { 3 } },
	{O_FS,		"FS", { 4 } },
	{O_GS,		"GS", { 5 } },

	{O_ST0,		"ST0", { 0 } },
	{O_FPUREG,	"ST1", { 1 } },
	{O_FPUREG,	"ST2", { 2 } },
	{O_FPUREG,	"ST3", { 3 } },
	{O_FPUREG,	"ST4", { 4 } },
	{O_FPUREG,	"ST5", { 5 } },
	{O_FPUREG,	"ST6", { 6 } },
	{O_FPUREG,	"ST7", { 7 } },

	{O_MMREG,	"MM0", { 0 } },
	{O_MMREG,	"MM1", { 1 } },
	{O_MMREG,	"MM2", { 2 } },
	{O_MMREG,	"MM3", { 3 } },
	{O_MMREG,	"MM4", { 4 } },
	{O_MMREG,	"MM5", { 5 } },
	{O_MMREG,	"MM6", { 6 } },
	{O_MMREG,	"MM7", { 7 } },

	{O_XMMREG,	"XMM0", { 0 } },
	{O_XMMREG,	"XMM1", { 1 } },
	{O_XMMREG,	"XMM2", { 2 } },
	{O_XMMREG,	"XMM3", { 3 } },
	{O_XMMREG,	"XMM4", { 4 } },
	{O_XMMREG,	"XMM5", { 5 } },
	{O_XMMREG,	"XMM6", { 6 } },
	{O_XMMREG,	"XMM7", { 7 } }
};

const COperand csyntax_set[] = 
{
	{O_VOID,	""},

	{O_ONE,		"1"},
	{O_IMM,		"imm"},
	{O_IMM8,	"imm8"},
	{O_IMM16,	"imm16"},
	{O_IMM32,	"imm32"},

	{O_AL,		"AL"},
	{O_AX,		"AX"},
	{O_EAX,		"EAX"},
	{O_DX,		"DX"},
	{O_CL,		"CL"},
	{O_CX,		"CX"},
	{O_ECX,		"ECX"},
	{O_CS,		"CS"},
	{O_DS,		"DS"},
	{O_ES,		"ES"},
	{O_SS,		"SS"},
	{O_FS,		"FS"},
	{O_GS,		"GS"},
	{O_ST0,		"ST0"},

	{O_REG8,	"reg8"},
	{O_REG16,	"reg16"},
	{O_REG32,	"reg32"},
	{O_SEGREG,	"segreg"},
	{O_FPUREG,	"fpureg"},
	{O_CR,		"CR0/2/3/4"},
	{O_DR,		"DR0/1/2/3/6/7"},
	{O_TR,		"TR3/4/5/6/7"},
	{O_MMREG,	"mmreg"},
	{O_XMMREG,	"xmmreg"},

	{O_MEM,		"mem"},
	{O_MEM8,	"mem8"},
	{O_MEM16,	"mem16"},
	{O_MEM32,	"mem32"},
	{O_MEM64,	"mem64"},
	{O_MEM80,	"mem80"},
	{O_MEM128,	"mem128"},

	{O_R_M8,	"r/m8"},
	{O_R_M16,	"r/m16"},
	{O_R_M32,	"r/m32"},
	{O_R_M64,	"r/m64"},
	{O_R_M128,	"r/m128"},

	{O_XMM32,	"xmmreg/mem32"},
	{O_XMM32,	"xmmreg/mem64"},
	{O_M512B,	"m512byte"},
	{O_MOFF8,	"memoffs8"},
	{O_MOFF16,	"memoffs16"},
	{O_MOFF32,	"memoffs32"}
};

const COperand CINIT = { O_VOID };
const COperand CNOT_FOUND = { O_UNKNOWN };

COperand coperand_scan_reg(const char *string)
{
	if (string) {
		size_t i;
		for (i = 0; i < sizeof(cregister_set) / sizeof(COperand); i++) {
			if (cstring_strcmp(string, cregister_set[i].notation, 1) == 0) {
				return cregister_set[i];
			}
		}
	}
	return CNOT_FOUND;
}

enum COperandType coperand_scan_syntax(const char *string)
{
	if (string) {
		size_t i;
		for (i = 0; i < sizeof(csyntax_set) / sizeof(COperand); i++) {
			if (cstring_strcmp(string, csyntax_set[i].notation, 1) == 0) {
				return csyntax_set[i].type;
			}
		}
	}
	return O_UNKNOWN;
}


//---------------------------------------------------------------------
// string operation
//---------------------------------------------------------------------
char *cstring_strip(char *str)
{
	int size = (int)strlen(str);
	char *p = str;
	int i;
	while (size > 0) {
		if (!isspace(str[size - 1])) break;
		size--;
	}
	str[size] = '\0';
	while (p[0]) {
		if (!isspace(p[0])) break;
		p++;
	}
	if (p == str) return str;
	for (i = 0; p[i]; i++) str[i] = p[i];
	str[i] = '\0';
	return str;
}

int cstring_strcmp(const char *s1, const char *s2, int caseoff)
{
	const char *p1, *p2, *p3, *p4;
	int k1, k2, i;
	for (p1 = s1; isspace(*p1); p1++);
	for (p2 = s2; isspace(*p2); p2++);
	for (k1 = (int)strlen(p1); k1 > 0; k1--) if (!isspace(p1[k1 - 1])) break;
	for (k2 = (int)strlen(p2); k2 > 0; k2--) if (!isspace(p2[k2 - 1])) break;
	p3 = p1 + k1;
	p4 = p2 + k2;
	for (i = 0; i < k1 && i < k2; i++) {
		char c1 = p1[i];
		char c2 = p2[i];
		if (caseoff && c1 >= 'a' && c1 <= 'z') c1 -= 'a' - 'A';
		if (caseoff && c2 >= 'a' && c2 <= 'z') c2 -= 'a' - 'A';
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
	}
	if (k1 < k2) return -1;
	if (k1 > k2) return 1;
	return 0;
}

char *cstring_strsep(char **stringp, const char *delim)
{
	register char *s;
	register const char *spanp;
	register int c, sc;
	char *tok;

	if ((s = *stringp) == NULL)
		return (NULL);
	for (tok = s;;) {
		c = *s++;
		spanp = delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0) s = NULL;
				else s[-1] = 0;
				*stringp = s;
				return tok;
			}
		}	while (sc != 0);
	}
}



