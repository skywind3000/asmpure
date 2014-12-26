//=====================================================================
//
// cinstruct.c - 
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#include "cinstruct.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

static void cinst_extract_operands(CInstruction *self, const char *syntax);

CInstruction *cinst_create(const CInstSyntax *syntax)
{
	CInstruction *self;
	self = (CInstruction*)malloc(sizeof(CInstruction));
	assert(self);
	self->syntax = syntax;
	cinst_extract_operands(self, syntax->operands);
	self->syntaxMnemonic = 0;
	self->syntaxSpecifier = 0;
	self->syntaxFirstOperand = 0;
	self->syntaxSecondOperand = 0;
	self->syntaxThirdOperand = 0;
	self->flags = syntax->flags;
	self->next = NULL;
	return self;
}

void cinst_release(CInstruction *self)
{
	if (self->next) cinst_release(self->next);
	self->next = NULL;
	memset(self, 0, sizeof(CInstruction));
	free(self);
}

static void cinst_extract_operands(CInstruction *self, const char *syntax)
{
	char *token;
	char *string;
	char *sep;

	assert(syntax && self);

	self->specifier = CS_UNKNOWN;
	self->firstOperand = O_VOID;
	self->secondOperand = O_VOID;
	self->thirdOperand = O_VOID;

	string = strdup(syntax);
	cstring_strip(string);

	sep = string;
	token = cstring_strsep(&sep, " ,");

	if (token == NULL) return;

	cstring_strip(token);
	self->specifier = cspecifier_scan(token);

	if (self->specifier != CS_UNKNOWN) {
		token = cstring_strsep(&sep, " ,");
		if (token == 0) {
			free(string);
			return;
		}
	}

	cstring_strip(token);
	self->firstOperand = coperand_scan_syntax(token);

	if (self->firstOperand != O_UNKNOWN) {
		token = cstring_strsep(&sep, " ,");
		if (token == 0) {
			free(string);
			return;
		}
	}

	cstring_strip(token);
	self->secondOperand = coperand_scan_syntax(token);

	if (self->secondOperand != O_UNKNOWN) {
		token = cstring_strsep(&sep, " ,");
		if (token == 0) {
			free(string);
			return;
		}
	}

	cstring_strip(token);
	self->thirdOperand = coperand_scan_syntax(token);

	if (self->thirdOperand != O_UNKNOWN) {
		token = cstring_strsep(&sep, " ,");
		if (token == 0) {
			free(string);
			return;
		}
	}

	if (token != 0) {
		fprintf(stderr, "casm: Invalid operand encoding '%s'\n", syntax);
		fflush(stderr);
		assert(0);
		return;
	}

	free(string);
}

CInstruction *cinst_get_next(CInstruction *self)
{
	assert(self);
	return self->next;
}

void cinst_attach_new(CInstruction *self, const CInstSyntax *instruction)
{
	if (!self->next) {
		self->next = cinst_create(instruction);
	}	else {
		cinst_attach_new(self->next, instruction);
	}
}
		
void cinst_reset_match(CInstruction *self)
{
	self->syntaxMnemonic = 0;
	self->syntaxSpecifier = 0;
	self->syntaxFirstOperand = 0;
	self->syntaxSecondOperand = 0;
	self->syntaxThirdOperand = 0;

	if (self->next) {
		cinst_reset_match(self->next);
	}
}

int cinst_match_syntax(CInstruction *self)
{
	return  self->syntaxMnemonic != 0 &&
			self->syntaxSpecifier != 0 &&
			self->syntaxFirstOperand != 0 &&
			self->syntaxSecondOperand != 0 &&
			self->syntaxThirdOperand != 0;	
}

void cinst_match_mnemonic(CInstruction *self, const char *mnemonic)
{
	if (stricmp(self->syntax->mnemonic, mnemonic) == 0) {
		self->syntaxMnemonic = 1;
	}
	if (self->next) {
		cinst_match_mnemonic(self->next, mnemonic);
	}
}

void cinst_match_specifier(CInstruction *self, enum CSpecifierType specifier)
{
	if (self->specifier == CS_UNKNOWN) 
	{
		if (self->specifier != CS_UNKNOWN) {
			if (self->firstOperand == O_R_M8 || 
				self->secondOperand == O_R_M8) {
				self->syntaxSpecifier = self->specifier == CS_BYTE;
			}
			else if (self->firstOperand == O_R_M16 || 
					self->secondOperand == O_R_M16) {
				self->syntaxSpecifier = self->specifier == CS_WORD;
			}
			else if (self->firstOperand == O_R_M32 || 
					self->secondOperand == O_R_M32) {
				self->syntaxSpecifier = self->specifier == CS_DWORD;
			}
			else if (self->firstOperand == O_R_M64 || 
					self->secondOperand == O_R_M64) {
				self->syntaxSpecifier = 
					(self->specifier == CS_QWORD || 
					self->specifier == CS_MMWORD);
			}
			else if (self->firstOperand == O_R_M128 || 
					self->secondOperand == O_R_M128) {
				self->syntaxSpecifier = self->specifier == CS_XMMWORD;
			}	
			else {
				self->syntaxSpecifier = 1;
			}
		}	else {
			self->syntaxSpecifier = 1;
		}
	}
	else if (self->specifier != CS_UNKNOWN)   // Explicit specifier
	{
		if (self->specifier == specifier) {
			self->syntaxSpecifier = 1;
		}
		else if (specifier == CS_UNKNOWN) {  
			self->syntaxSpecifier = 1;	// Specifiers are optional
		}
		else {
			self->syntaxSpecifier = 0;
		}
	}

	if (self->next) {
		cinst_match_specifier(self->next, specifier);
	}
}

void cinst_match_first_operand(CInstruction *self, const COperand *operand)
{
	if (coperand_is_subtype_of(operand, self->firstOperand)) {
		self->syntaxFirstOperand = 1;
	}
	else if (operand->type == O_MEM && self->firstOperand & O_MEM) {
		if(self->syntaxSpecifier) {  // Explicit size specfier
			self->syntaxFirstOperand = 1;
		}
		else if(self->secondOperand != O_UNKNOWN) { //Implicit size specifier
			self->syntaxFirstOperand = 1;
		}
	}

	if (self->next) {
		cinst_match_first_operand(self->next, operand);
	}
}

void cinst_match_second_operand(CInstruction *self, const COperand *operand)
{
	if (coperand_is_subtype_of(operand, self->secondOperand)) {
		self->syntaxSecondOperand = 1;
	}
	else if (operand->type == O_MEM && self->secondOperand & O_MEM) {
		if (self->syntaxSpecifier) {  // Explicit size specfier
			self->syntaxSecondOperand = 1;
		}
		else if (self->firstOperand != O_UNKNOWN) {
			self->syntaxSecondOperand = 1;
		}
	}
	if (self->next) {
		cinst_match_second_operand(self->next, operand);
	}
}

void cinst_match_third_operand(CInstruction *self, const COperand *operand)
{
	if (coperand_is_subtype_of(operand, self->thirdOperand)) {
		self->syntaxThirdOperand = 1;
	}
	if (self->next) {
		cinst_match_third_operand(self->next, operand);
	}
}

enum COperandType cinst_getFirstOperand(CInstruction *self)
{
	return self->firstOperand;
}

enum COperandType cinst_getSecondOperand(CInstruction *self)
{
	return self->secondOperand;
}

enum COperandType cinst_getThirdOperand(CInstruction *self)
{
	return self->thirdOperand;
}

const char *cinst_getMnemonic(CInstruction *self)
{
	return self->syntax->mnemonic;
}

const char *cinst_getOperandSyntax(CInstruction *self)
{
	return self->syntax->operands;
}

const char *cinst_getEncoding(CInstruction *self)
{
	return self->syntax->encoding;
}
		
int cinst_is_32bit(CInstruction *self)
{
	return (self->flags & CT_CPU_386) == CT_CPU_386;
}



