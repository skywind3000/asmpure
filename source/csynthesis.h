//=====================================================================
//
// csynthesis.h - source scanner
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#ifndef __CSYNTHESIS_H__
#define __CSYNTHESIS_H__

#include "cencoding.h"
#include "ckeywords.h"
#include "cinstruct.h"


//---------------------------------------------------------------------
// CSynthesizer
//---------------------------------------------------------------------
struct CSynthesizer
{
	CEncoding encoding;
	enum COperandType firstType;
	enum COperandType secondType;
	enum CRegID firstReg;
	enum CRegID secondReg;
	enum CRegID baseReg;
	enum CRegID indexReg;
	int scale;
	int prefix;
	char *error;
	int errcode;
};

typedef struct CSynthesizer CSynthesizer;



#ifdef __cplusplus
extern "C" {
#endif
//---------------------------------------------------------------------
// interface
//---------------------------------------------------------------------
void csynth_init(CSynthesizer *synth);
void csynth_destroy(CSynthesizer *synth);
void csynth_reset(CSynthesizer *synth);

int csynth_define_label(CSynthesizer *synth, const char *label);
int csynth_reference_label(CSynthesizer *synth, const char *label);

int csynth_encode_first_operand(CSynthesizer *synth, const COperand *);
int csynth_encode_second_operand(CSynthesizer *synth, const COperand *);
int csynth_encode_third_operand(CSynthesizer *synth, const COperand *);

int csynth_encode_base(CSynthesizer *synth, const COperand *base);
int csynth_encode_index(CSynthesizer *synth, const COperand *index);

int csynth_encode_scale(CSynthesizer *synth, int scale);
int csynth_encode_immediate(CSynthesizer *synth, long immediate);
int csynth_encode_displacement(CSynthesizer *synth, long displacement);

int csynth_encode_prefix(CSynthesizer *synth, int code);

const CEncoding *csynth_encode_instruction(CSynthesizer *, CInstruction*);


#ifdef __cplusplus
}
#endif

#endif


