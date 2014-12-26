//=====================================================================
//
// cencoding.c - x86 instruction encoding
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#include "cencoding.h"

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#pragma warning(disable: 4311)
#endif

void cencoding_reset(CEncoding *self)
{
	if (self->label) free(self->label);
	self->label = NULL;
	if (self->reference) free(self->reference);
	self->reference = NULL;
	if (self->data) free(self->data);
	self->data = NULL;

	self->format.P1 = 0;
	self->format.P2 = 0;
	self->format.P3 = 0;
	self->format.P4 = 0;
	self->format.REX = 0;
	self->format.O3 = 0;
	self->format.O2 = 0;
	self->format.O1 = 0;
	self->format.modRM = 0;
	self->format.SIB = 0;
	self->format.D1 = 0;
	self->format.D2 = 0;
	self->format.D3 = 0;
	self->format.D4 = 0;
	self->format.I1 = 0;
	self->format.I2 = 0;
	self->format.I3 = 0;
	self->format.I4 = 0;

	self->P1 = 0;
	self->P2 = 0;
	self->P3 = 0;
	self->P4 = 0;
	self->REX.b = 0;
	self->O3 = 0;
	self->O2 = 0;
	self->O1 = 0;
	self->modRM.b = 0;
	self->SIB.b = 0;
	self->D1 = 0;
	self->D2 = 0;
	self->D3 = 0;
	self->D4 = 0;
	self->I1 = 0;
	self->I2 = 0;
	self->I3 = 0;
	self->I4 = 0;

	self->immediate = 0;
	self->displacement = 0;
	self->message = (char*)"";
	self->size = 0;
	self->align = 0;
	self->relative = 0;
}

void cencoding_init(CEncoding *self)
{
	self->label = 0;
	self->reference = 0;
	self->data = 0;
	self->size = 0;
	cencoding_reset(self);
	self->O1 = 0xCC;	// breakpoint
	self->format.O1 = 1;	
}

void cencoding_destroy(CEncoding *self)
{
	cencoding_reset(self);
}

const char *cencoding_get_label(const CEncoding *self)
{
	return self->label;
}

const char *cencoding_get_reference(const CEncoding *self)
{
	return self->reference;
}

int cencoding_length(const CEncoding *self)
{
	int length = 0;
	if (self->data && self->size > 0) 
		return self->size;
	if (self->align > 0) 
		return self->align;
	if (self->format.P1)		length++;
	if (self->format.P2)		length++;
	if (self->format.P3)		length++;
	if (self->format.P4)		length++;
	if (self->format.REX)		length++;
	if (self->format.O3)		length++;
	if (self->format.O2)		length++;
	if (self->format.O1)		length++;
	if (self->format.modRM)		length++;
	if (self->format.SIB)		length++;
	if (self->format.D1)		length++;
	if (self->format.D2)		length++;
	if (self->format.D3)		length++;
	if (self->format.D4)		length++;
	if (self->format.I1)		length++;
	if (self->format.I2)		length++;
	if (self->format.I3)		length++;
	if (self->format.I4)		length++;

	return length;
}

int cencoding_new_copy(CEncoding *self, const CEncoding *src)
{
	*self = *src;
	if (src->label) {
		long size = (long)strlen(src->label);
		self->label = (char*)malloc(size + 1);
		assert(self->label);
		memcpy(self->label, src->label, size + 1);
		self->label[size] = 0;
	}
	if (src->reference) {
		long size = (long)strlen(src->reference);
		self->reference = (char*)malloc(size + 1);
		assert(self->reference);
		memcpy(self->reference, src->reference, size + 1);
		self->reference[size] = 0;
	}
	if (src->data) {
		self->data = (char*)malloc(src->size + 1);
		assert(self->data);
		memcpy(self->data, src->data, src->size);
		self->size = src->size;
	}
	return 0;
}

int cencoding_add_prefix(CEncoding *self, cbyte p)
{
	if (!self->format.P1) {
		self->P1 = p;
		self->format.P1 = 1;
	}
	else if (!self->format.P2) {
		self->P2 = p;
		self->format.P2 = 1;
	}
	else if (!self->format.P3) {
		self->P3 = p;
		self->format.P3 = 1;
	}
	else if (!self->format.P4) {
		self->P4 = p;
		self->format.P4 = 1;
	}	else {
		return -1;
	}
	return 0;
}

int cencoding_set_immediate(CEncoding *self, int immediate)
{
	self->immediate = immediate;
	return 0;
}

int cencoding_set_jump_offset(CEncoding *self, int offset)
{
	if ((char)offset != offset && self->format.I2 == 0) {
		self->message = (char*)"Jump offset range too big";
		return -1;
	}
	self->immediate = offset;
	return 0;
}

void cencoding_set_label(CEncoding *self, const char *label)
{
	int size = (int)strlen(label);
	if (self->label) free(self->label);
	self->label = (char*)malloc(size + 1);
	assert(self->label);
	memcpy(self->label, label, size + 1);
}

void cencoding_set_reference(CEncoding *self, const char *ref)
{
	int size = (int)strlen(ref);
	if (self->reference) free(self->reference);
	self->reference = (char*)malloc(size + 1);
	assert(self->reference);
	memcpy(self->reference, ref, size + 1);
}

void cencoding_set_data(CEncoding *self, const void *data, int size)
{
	if (self->data) free(self->data);
	self->data = NULL;
	self->size = 0;
	if (data && size > 0) {
		self->data = (char*)malloc(size + 1);
		assert(self->data);
		self->size = size;
		memcpy(self->data, data, size);
	}
}

int cencoding_check_format(const CEncoding *self)
{
	// Bytes cannot be changed without updating format, 
	// except immediate and displacement
	if ((self->P1 && !self->format.P1) ||
	   (self->P2 && !self->format.P2) ||
	   (self->P3 && !self->format.P3) ||
	   (self->P4 && !self->format.P4) ||
	   (self->REX.b && !self->format.REX) ||
	   (self->O2 && !self->format.O2) ||
	   (self->O1 && !self->format.O1) ||
	   (self->modRM.b && !self->format.modRM) ||
	   (self->SIB.b && !self->format.SIB)) {
		return -1;   
	}

	if ((self->format.P4 && !self->format.P3) ||
	   (self->format.P3 && !self->format.P2) ||
	   (self->format.P2 && !self->format.P1)) {
		return -2;
	}

	if (self->format.O2 &&
	   (self->O2 != 0x0F &&
	    self->O2 != 0xD8 &&
		self->O2 != 0xD9 &&
		self->O2 != 0xDA &&
		self->O2 != 0xDB &&
		self->O2 != 0xDC &&
		self->O2 != 0xDD &&
		self->O2 != 0xDE &&
		self->O2 != 0xDF)) {
		return -3;
	}

	if (self->format.SIB) {
		if(!self->format.modRM) {
			return -4;
		}
		if(self->modRM.r_m != E_ESP) {
			return -5;
		}
	}

	// Byte, word or doubleword
	if ((self->format.D4 && !self->format.D3) ||
	   (self->format.D3 && !self->format.D4) ||
	   (self->format.D3 && !self->format.D2) ||
	   (self->format.D2 && !self->format.D1)) {
		return -6;
	}

	// Byte, word or doubleword
	if ((self->format.I4 && !self->format.I3) ||
	   (self->format.I3 && !self->format.I4) ||
	   (self->format.I3 && !self->format.I2) ||
	   (self->format.I2 && !self->format.I1)) {
		return -7;
	}

	return 0;
}

int cencoding_write_code(const CEncoding *self, unsigned char *output)
{
	unsigned char *start = output;

	#define cencoding_output(b) { if (start) *output = (b); output++; } 

	if (self->data && self->size > 0) {
		if (output) memcpy(output, self->data, self->size);
		return (int)self->size;
	}

	if (self->align > 0) {
		unsigned long linear = (((unsigned long)output) & 0xfffffffful);
		int size = self->align - (linear % self->align);
		for (; size >= 2; size -= 2) {
			cencoding_output(0x66);
			cencoding_output(0x90);
		}
		for (; size > 0; size--) {
			cencoding_output(0x90);
		}
		return (int)(output - start);
	}

	if (self->format.P1)		cencoding_output(self->P1);
	if (self->format.P2)		cencoding_output(self->P2);
	if (self->format.P3)		cencoding_output(self->P3);
	if (self->format.P4)		cencoding_output(self->P4);
	if (self->format.REX)		cencoding_output(self->REX.b);
	if (self->format.O3)		cencoding_output(self->O3);
	if (self->format.O2)		cencoding_output(self->O2);
	if (self->format.O1)		cencoding_output(self->O1);
	if (self->format.modRM)		cencoding_output(self->modRM.b);
	if (self->format.SIB)		cencoding_output(self->SIB.b);
	if (self->format.D1)		cencoding_output(self->D1);
	if (self->format.D2)		cencoding_output(self->D2);
	if (self->format.D3)		cencoding_output(self->D3);
	if (self->format.D4)		cencoding_output(self->D4);
	if (self->format.I1)		cencoding_output(self->I1);
	if (self->format.I2)		cencoding_output(self->I2);
	if (self->format.I3)		cencoding_output(self->I3);
	if (self->format.I4)		cencoding_output(self->I4);

	#undef cencoding_output

	return (int)(output - start);
}


void cencoding_to_string(const CEncoding *self, char *output)
{
	const char *fmt = "0123456789ABCDEF";
	int hr = cencoding_check_format(self);

	assert(hr == 0);

	#define cencoding_format(data) { \
			if (output) { \
				unsigned char ch = (unsigned char)(data & 0xff); \
				*output++ = fmt[ch / 16]; \
				*output++ = fmt[ch % 16]; \
				*output++ = ' '; \
			}	\
		}

	if (self->data) {
		long i;
		for (i = 0; i < self->size; i++) {
			unsigned int bb = (unsigned char)self->data[i];
			cencoding_format(bb);
		}
		*output++ = '\0';
		return;
	}

	if (self->align > 0) {
		*output++ = '\0';
		return;
	}

	if (self->format.P1)		cencoding_format(self->P1);
	if (self->format.P2)		cencoding_format(self->P2);
	if (self->format.P3)		cencoding_format(self->P3);
	if (self->format.P4)		cencoding_format(self->P4);
	if (self->format.REX)		cencoding_format(self->REX.b);
	if (self->format.O3)		cencoding_format(self->O3);
	if (self->format.O2)		cencoding_format(self->O2);
	if (self->format.O1)		cencoding_format(self->O1);
	if (self->format.modRM)		cencoding_format(self->modRM.b);
	if (self->format.SIB)		cencoding_format(self->SIB.b);
	if (self->format.D1)		cencoding_format(self->D1);
	if (self->format.D2)		cencoding_format(self->D2);
	if (self->format.D3)		cencoding_format(self->D3);
	if (self->format.D4)		cencoding_format(self->D4);
	if (self->format.I1)		cencoding_format(self->I1);
	if (self->format.I2)		cencoding_format(self->I2);
	if (self->format.I3)		cencoding_format(self->I3);
	if (self->format.I4)		cencoding_format(self->I4);

	#undef cencoding_format

	*output++ = '\0';
}


void cencoding_to_stdout(const CEncoding *self)
{
	static char text[8192];
	cencoding_to_string(self, text);
	printf("%s\n", text);
}

