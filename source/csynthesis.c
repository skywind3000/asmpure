//=====================================================================
//
// csynthesis.c - source scanner
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================

#include "csynthesis.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

void csynth_init(CSynthesizer *synth)
{
	cencoding_init(&synth->encoding);
	synth->error = (char*)malloc(1024);
	assert(synth->error);
	csynth_reset(synth);
}

void csynth_destroy(CSynthesizer *synth)
{
	cencoding_destroy(&synth->encoding);
	if (synth->error) free(synth->error);
	synth->error = NULL;
	synth->errcode = 0;
}

void csynth_reset(CSynthesizer *synth)
{
	cencoding_reset(&synth->encoding);
	synth->firstType = O_UNKNOWN;
	synth->secondType = O_UNKNOWN;
	synth->firstReg = REG_UNKNOWN;
	synth->secondReg = REG_UNKNOWN;
	synth->baseReg = REG_UNKNOWN;
	synth->indexReg = REG_UNKNOWN;
	synth->scale = 0;
	synth->prefix = 0;
	synth->error[0] = 0;
	synth->errcode = 0;
}

static void csynth_error(CSynthesizer *synth, const char *error, int code)
{
	strncpy(synth->error, error, 100);
	synth->errcode = code;
}

int csynth_define_label(CSynthesizer *synth, const char *label)
{
	if (synth->encoding.label != NULL) {
		csynth_error(synth,  "Instruction can't have multiple label", 1);
		return -1;
	}
	cencoding_set_label(&synth->encoding, label);
	return 0;
}

int csynth_reference_label(CSynthesizer *synth, const char *label)
{
	if (synth->encoding.reference != NULL) {
		csynth_error(synth,  "Instruction can't have multiple refrence", 2);
		return -1;
	}
	cencoding_set_reference(&synth->encoding, label);
	return 0;
}

int csynth_encode_first_operand(CSynthesizer *synth, 
	const COperand *firstOperand)
{
	if (synth->firstType != O_UNKNOWN) {
		csynth_error(synth,  "Instrucition destination already set", 3);
		return -1;
	}

	synth->firstType = firstOperand->type;

	if (coperand_is_reg(firstOperand) || coperand_is_mem(firstOperand)) {
		synth->firstReg = firstOperand->reg;
	}
	else if (coperand_is_imm(firstOperand)) {
		csynth_encode_immediate(synth, firstOperand->value);
	}
	else if (!coperand_is_void(firstOperand)) {
		csynth_error(synth, "csynth_encode_first_operand: error", 4);
		return -2;
	}

	return 0;
}

int csynth_encode_second_operand(CSynthesizer *synth, 
	const COperand *secondOperand)
{
	if (synth->secondType != O_UNKNOWN) {
		csynth_error(synth,  "Instrucition source already set", 4);
		return -1;
	}

	synth->secondType = secondOperand->type;

	if (coperand_is_reg(secondOperand) || coperand_is_mem(secondOperand)) {
		synth->secondReg = secondOperand->reg;
	}
	else if (coperand_is_imm(secondOperand)) {
		csynth_encode_immediate(synth, secondOperand->value);
	}
	else if (!coperand_is_void(secondOperand)) {
		csynth_error(synth, "csynth_encode_second_operand: error", 5);
		return -2;
	}

	return 0;
}

int csynth_encode_third_operand(CSynthesizer *synth, 
	const COperand *thirdOperand)
{
	if (coperand_is_imm(thirdOperand)) {
		csynth_encode_immediate(synth, thirdOperand->value);
	}
	else if (!coperand_is_void(thirdOperand)) {
		csynth_error(synth, "csynth_encode_third_operand: error", 6);
		return -3;
	}
	return 0;
}

int csynth_encode_base(CSynthesizer *synth, const COperand *base)
{
	if (synth->baseReg != REG_UNKNOWN) {
		int retval;
		// base already set, use as index with scale = 1
		retval = csynth_encode_index(synth, base);
		if (retval != 0) return -1;
		retval = csynth_encode_scale(synth, 1);
		if (retval != 0) return -2;
		return 0;
	}

	synth->baseReg = base->reg;
	return 0;
}

int csynth_encode_index(CSynthesizer *synth, const COperand *index)
{
	if (synth->indexReg != REG_UNKNOWN) {
		csynth_error(synth, 
			"Memory reference can't have multiple index registers", 7);
		return -1;
	}
	synth->indexReg = index->reg;
	return 0;
}

int csynth_encode_scale(CSynthesizer *synth, int scale)
{
	if (synth->scale != 0) {
		csynth_error(synth, 
			"Memory reference can't have multiple scale factors", 8);
		return -1;
	}
	if (scale != 1 && scale != 2 && scale != 4 && scale != 8) {
		csynth_error(synth, "Invalid scale value", 9);
		return -2;
	}
	synth->scale = scale;
	return 0;
}

int csynth_encode_immediate(CSynthesizer *synth, long immediate)
{
	if (synth->encoding.immediate != 0) {
		csynth_error(synth, 
			"Instruction can't have multiple immediate operands", 10);
		return -1;
	}
	synth->encoding.immediate = (cint32)immediate;
	return 0;
}

int csynth_encode_displacement(CSynthesizer *synth, long displacement)
{
	synth->encoding.displacement += (cint32)displacement;
	return 0;
}

static int csynth_encode_mod_field(CSynthesizer *synth)
{
	synth->encoding.format.modRM = 1;
	if (coperand_type_is_reg(synth->firstType) && (
		coperand_type_is_reg(synth->secondType) ||
		coperand_type_is_imm(synth->secondType) ||
		coperand_type_is_void(synth->secondType))) {
		synth->encoding.modRM.mod = MOD_REG;
	}
	else if ((coperand_type_is_mem(synth->firstType) ||
			  coperand_type_is_mem(synth->secondType)) &&
			 (coperand_type_is_reg(synth->firstType) ||
			  coperand_type_is_reg(synth->secondType))) {
		if (!synth->encoding.displacement) {
			synth->encoding.modRM.mod = MOD_NO_DISP;
		}
		else if ((char)synth->encoding.displacement == 
				synth->encoding.displacement) {
			synth->encoding.modRM.mod = MOD_BYTE_DISP;
			synth->encoding.format.D1 = 1;
		}
		else {
			synth->encoding.modRM.mod = MOD_DWORD_DISP;
			synth->encoding.format.D1 = 1;
			synth->encoding.format.D2 = 1;
			synth->encoding.format.D3 = 1;
			synth->encoding.format.D4 = 1;
		}
	}
	else {
		csynth_error(synth, "mod field error", 30);
		return -1;
	}
	return 0;
}

static int csynth_encode_sib_byte(CSynthesizer *synth)
{
	if (synth->scale == 0 && synth->indexReg == REG_UNKNOWN) {
		if (synth->baseReg == REG_UNKNOWN || (
			synth->encoding.modRM.r_m != E_ESP && 
			synth->encoding.modRM.r_m != E_EBP)) {
			if (synth->encoding.format.SIB) {
				csynth_error(synth, "SIB byte error", 31);
				return -1;
			}
			// No SIB byte needed
			return 0;
		}
	}

	// Indicates use of SIB in mod R/M
	synth->encoding.format.SIB = 1;
	synth->encoding.modRM.r_m = E_ESP;

	if (synth->baseReg == E_EBP && synth->encoding.modRM.mod == MOD_NO_DISP)
	{
		synth->encoding.modRM.mod = MOD_BYTE_DISP;
		synth->encoding.format.D1 = 1;
	}

	if (synth->indexReg == E_ESP) {
		if (synth->scale != 1) {
			csynth_error(synth, 
				"ESP can't be scaled index in memory reference", 32);
			return -2;
		}
		else {
			enum CRegID tempReg;
			tempReg = synth->indexReg;
			synth->indexReg = synth->baseReg;
			synth->baseReg = tempReg;
		}
	}

	if (synth->baseReg == REG_UNKNOWN) {
		synth->encoding.SIB.base = E_EBP;
		synth->encoding.modRM.mod = MOD_NO_DISP;
		synth->encoding.format.D1 = 1;
		synth->encoding.format.D2 = 1;
		synth->encoding.format.D3 = 1;
		synth->encoding.format.D4 = 1;
	}	else {
		synth->encoding.SIB.base = synth->baseReg;
	}
	
	if (synth->indexReg != REG_UNKNOWN) {
		synth->encoding.SIB.index = synth->indexReg;
	}	else {
		synth->encoding.SIB.index = E_ESP;
	}

	switch (synth->scale)
	{
	case 0:
	case 1:
		synth->encoding.SIB.scale = SCALE_1;
		break;
	case 2:
		synth->encoding.SIB.scale = SCALE_2;
		break;
	case 4:
		synth->encoding.SIB.scale = SCALE_4;
		break;
	case 8:
		synth->encoding.SIB.scale = SCALE_8;
		break;
	default:
		csynth_error(synth, "scale number error", 33);
		return -3;
		break;
	}

	return 0;
}

int csynth_encode_prefix(CSynthesizer *synth, int code)
{
	if (cencoding_add_prefix(&synth->encoding, code)) {
		csynth_error(synth, "cannot add prefix", 90);
		return -1;
	}
	return 0;
}

const CEncoding *csynth_encode_instruction(CSynthesizer *synth, 
	CInstruction *instruction)
{
	enum COperandType p1, p2;
	const char *format;
	unsigned long O;

	if (!instruction) {
		return &synth->encoding;
	}

	format = cinst_getEncoding(instruction);

	if (!format) {
		csynth_error(synth, "csynth_encode_instruction: internal error", 11);
		return NULL;
	}

	#define IFORMAT_WORD(x, y) ( (((short)(x)) << 8) | ((short)(y)) )

	while (*format) {
		int head = ((short)format[1] | format[0] << 8);
		switch (head)
		{
		case IFORMAT_WORD('p', '0'):
			if (cencoding_add_prefix(&synth->encoding, 0xf0)) {
				csynth_error(synth, "prefix error 0xf0", 40);
				return NULL;
			}
			break;
		case IFORMAT_WORD('p', '2'):
			if (cencoding_add_prefix(&synth->encoding, 0xf2)) {
				csynth_error(synth, "prefix error 0xf2", 41);
				return NULL;
			}
			break;
		case IFORMAT_WORD('p', '3'):
			if (cencoding_add_prefix(&synth->encoding, 0xf3)) {
				csynth_error(synth, "prefix error 0xf3", 42);
				return NULL;
			}
			break;
		case IFORMAT_WORD('p', 'o'):
			if (!cinst_is_32bit(instruction)) {
				if (cencoding_add_prefix(&synth->encoding, 0x66)) {
					csynth_error(synth, "prefix error 0x66", 43);
					return NULL;
				}
			}
			break;
		case IFORMAT_WORD('p', 'a'):
			if (!cinst_is_32bit(instruction)) {
				if (cencoding_add_prefix(&synth->encoding, 0x67)) {
					csynth_error(synth, "prefix error 0x67", 44);
					return NULL;
				}
			}
			break;
		case IFORMAT_WORD('+', 'r'):
			if (synth->encoding.format.O1) {
				if (coperand_type_is_reg(synth->firstType)) {
					synth->encoding.O1 += synth->firstReg;
				}
				else if(coperand_type_is_reg(synth->secondType)) {
					synth->encoding.O1 += synth->secondReg;
				}
				else {
					csynth_error(synth, 
						"'+r' not compatible with operands", 12);
					return NULL;
				}
			}
			else {
				csynth_error(synth, "'+r' needs first opcode byte", 13);
				return NULL;
			}
			break;
		case IFORMAT_WORD('/', 'r'):
			if (csynth_encode_mod_field(synth) != 0) {
				return NULL;
			}
			p1 = cinst_getFirstOperand(instruction);
			p2 = cinst_getSecondOperand(instruction);
			if (coperand_type_is_reg(p1) && coperand_type_is_R_M(p2)) {
				if (coperand_type_is_mem(synth->secondType)) {
					synth->encoding.modRM.r_m = synth->baseReg;
				}
				else if (coperand_type_is_reg(synth->secondType)) {
					synth->encoding.modRM.r_m = synth->secondReg;
				}
				else {
					csynth_error(synth, "syntax error", 14);
					return NULL;
				}
				synth->encoding.modRM.reg = synth->firstReg;
			}
			else if (coperand_type_is_R_M(p1) && coperand_type_is_reg(p2)) {
				if (coperand_type_is_mem(synth->firstType)) {
					synth->encoding.modRM.r_m = synth->baseReg;
				}
				else if (coperand_type_is_reg(synth->firstType)) {
					synth->encoding.modRM.r_m = synth->firstReg;
				}
				else {
					csynth_error(synth, "syntax error", 15);
					return NULL;
				}
				synth->encoding.modRM.reg = synth->secondReg;
			}
			else {
				csynth_error(synth, "format error", 16);
				return NULL;
			}
			if (csynth_encode_sib_byte(synth) != 0) {
				return NULL;
			}
			break;
		case IFORMAT_WORD('/', '0'):
		case IFORMAT_WORD('/', '1'):
		case IFORMAT_WORD('/', '2'):
		case IFORMAT_WORD('/', '3'):
		case IFORMAT_WORD('/', '4'):
		case IFORMAT_WORD('/', '5'):
		case IFORMAT_WORD('/', '6'):
		case IFORMAT_WORD('/', '7'):
			if (csynth_encode_mod_field(synth) != 0) {
				return NULL;
			}
			synth->encoding.modRM.reg = format[1] - '0';
			if (coperand_type_is_mem(synth->firstType)) {
				synth->encoding.modRM.r_m = synth->baseReg;
			}
			else if (coperand_type_is_reg(synth->firstType)) {
				synth->encoding.modRM.r_m = synth->firstReg;
			}
			else {
				csynth_error(synth, "syntax error", 17);
				return NULL;
			}
			if (csynth_encode_sib_byte(synth) != 0) {
				return NULL;
			}
			break;
		case IFORMAT_WORD('i', 'd'):
			synth->encoding.format.I1 = 1;
			synth->encoding.format.I2 = 1;
			synth->encoding.format.I3 = 1;
			synth->encoding.format.I4 = 1;
			synth->encoding.relative = 0;
			break;
		case IFORMAT_WORD('i', 'w'):
			synth->encoding.format.I1 = 1;
			synth->encoding.format.I2 = 1;
			synth->encoding.relative = 0;
			break;
		case IFORMAT_WORD('i', 'b'):
			synth->encoding.format.I1 = 1;
			synth->encoding.relative = 0;
			break;
		case IFORMAT_WORD('-', 'b'):
			synth->encoding.format.I1 = 1;
			synth->encoding.relative = 1;
			break;
		case IFORMAT_WORD('-', 'i'):
			synth->encoding.format.I1 = 1;
			synth->encoding.format.I2 = 1;
			synth->encoding.format.I3 = 1;
			synth->encoding.format.I4 = 1;
			synth->encoding.relative = 1;
			break;
		default:
			O = strtoul(format, 0, 16);
			if (O > 0xFF) {
				csynth_error(synth, "format error", 18);
				return NULL;
			}
			if (!synth->encoding.format.O1) {
				synth->encoding.O1 = (cbyte)O;
				synth->encoding.format.O1 = 1;
			}
			else if (synth->encoding.format.O2 == 0 &&
					(synth->encoding.O1 == 0x0f ||
					 synth->encoding.O1 == 0xd8 ||
					 synth->encoding.O1 == 0xd9 ||
					 synth->encoding.O1 == 0xda ||
					 synth->encoding.O1 == 0xdb ||
					 synth->encoding.O1 == 0xdc ||
					 synth->encoding.O1 == 0xde ||
					 synth->encoding.O1 == 0xdf)) {
				synth->encoding.O2 = synth->encoding.O1;
				synth->encoding.O1 = (cbyte)O;
				synth->encoding.format.O2 = 1;
			}
			else {
				csynth_error(synth, "synth error", 19);
				return NULL;
			}
			break;
		}

		format += 2;
		if (*format == ' ') {
			format++;
		}
		else if (*format == '\0') {
			break;
		}
		else {
			csynth_error(synth, "instruction error", 20);
			return NULL;
		}
		#undef IFORMAT_WORD
	}

	return &synth->encoding;
}

