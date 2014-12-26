//=====================================================================
//
// cencoding.h - x86 instruction encoding
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================

#ifndef __CENCODING_H__
#define __CENCODING_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

//---------------------------------------------------------------------
// Platform Word Size Detect
//---------------------------------------------------------------------
#if (!defined(__CUINT32_DEFINED)) && (!defined(__CINT32_DEFINED))
#define __CUINT32_DEFINED
#define __CINT32_DEFINED
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) \
	 || defined(__i386__) || defined(__i386) || defined(_M_X86)
	typedef unsigned int cuint32;
	typedef int cint32;
#elif defined(__MACOS__)
	typedef UInt32 cuint32;
	typedef Int32 cint32;
#elif defined(__APPLE__) && defined(__MACH__)
	#include <sys/types.h>
	typedef u_int32_t cuint32;
	typedef int32_t cint32;
#elif defined(__BEOS__)
	#include <sys/inttypes.h>
	typedef u_int32_t cuint32;
	typedef int32_t cint32;
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || \
	defined(__amd64) || defined(_M_IA64) || defined(_M_AMD64)
	typedef unsigned int cuint32;
	typedef int cint32;
#elif defined(_MSC_VER) || defined(__BORLANDC__)
	typedef unsigned __int32 cuint32;
	typedef __int32 cint32;
#elif defined(__GNUC__)
	#include <stdint.h>
	typedef uint32_t cuint32;
	typedef int32_t cint32;
#else 
	typedef unsigned long cuint32;     
	typedef long cint32;
#endif
#endif

#ifndef __CINT8_DEFINED
#define __CINT8_DEFINED
typedef char cint8;
#endif

#ifndef __CUINT8_DEFINED
#define __CUINT8_DEFINED
typedef unsigned char cuint8;
#endif

#ifndef __CUINT16_DEFINED
#define __CUINT16_DEFINED
typedef unsigned short cuint16;
#endif

#ifndef __CINT16_DEFINED
#define __CINT16_DEFINED
typedef short CINT16;
#endif

#ifndef __CINT64_DEFINED
#define __CINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 cint64;
#else
typedef long long cint64;
#endif
#endif

#ifndef __CUINT64_DEFINED
#define __CUINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 cuint64;
#else
typedef unsigned long long cuint64;
#endif
#endif

#ifndef INLINE
#ifdef __GNUC__

#if __GNUC_MINOE__ >= 1  && __GNUC_MINOE__ < 4
#define INLINE         __inline__ __attribute__((always_inline))
#else
#define INLINE         __inline__
#endif

#elif (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE 
#endif
#endif

#ifndef inline
#define inline INLINE
#endif

typedef cuint8 cbyte;


//---------------------------------------------------------------------
// CReg
//---------------------------------------------------------------------
enum CRegID
{
	REG_UNKNOWN = -1,
	E_AL = 0, E_AX = 0, E_EAX = 0, E_ST0 = 0, E_MM0 = 0, E_XMM0 = 0,
	E_CL = 1, E_CX = 1, E_ECX = 1, E_ST1 = 1, E_MM1 = 1, E_XMM1 = 1,
	E_DL = 2, E_DX = 2, E_EDX = 2, E_ST2 = 2, E_MM2 = 2, E_XMM2 = 2,
	E_BL = 3, E_BX = 3, E_EBX = 3, E_ST3 = 3, E_MM3 = 3, E_XMM3 = 3,
	E_AH = 4, E_SP = 4, E_ESP = 4, E_ST4 = 4, E_MM4 = 4, E_XMM4 = 4,
	E_CH = 5, E_BP = 5, E_EBP = 5, E_ST5 = 5, E_MM5 = 5, E_XMM5 = 5,
	E_DH = 6, E_SI = 6, E_ESI = 6, E_ST6 = 6, E_MM6 = 6, E_XMM6 = 6,
	E_BH = 7, E_DI = 7, E_EDI = 7, E_ST7 = 7, E_MM7 = 7, E_XMM7 = 7,
	E_R0 = 0, E_R1 = 1, E_R2 = 2, E_R3 = 3, E_R4 = 4, E_R5 = 5,
	E_R6 = 6, E_R7 = 7, E_R8 = 8, E_R9 = 9, E_R10 = 10, E_R11 = 11,
	E_R12 = 12, E_R13 = 13, E_R14 = 14, E_R15 = 15
};

enum CSMod
{
	MOD_NO_DISP = 0,
	MOD_BYTE_DISP = 1,
	MOD_DWORD_DISP = 2,
	MOD_REG = 3
};

enum CScale
{
	SCALE_UNKNOWN = 0,
	SCALE_1 = 0,
	SCALE_2 = 1,
	SCALE_4 = 2,
	SCALE_8 = 3
};


//---------------------------------------------------------------------
// CEncoding 
//---------------------------------------------------------------------
struct CEncoding
{
	char *label;
	char *reference;
	char *message;
	char *data;
	int size;
	int align;
	int relative;

	struct {
		unsigned char P1 : 1;
		unsigned char P2 : 1;
		unsigned char P3 : 1;
		unsigned char P4 : 1;
		unsigned char REX : 1;
		unsigned char O3 : 1;
		unsigned char O2 : 1;
		unsigned char O1 : 1;
		unsigned char modRM : 1;
		unsigned char SIB : 1;
		unsigned char D1 : 1;
		unsigned char D2 : 1;
		unsigned char D3 : 1;
		unsigned char D4 : 1;
		unsigned char I1 : 1;
		unsigned char I2 : 1;
		unsigned char I3 : 1;
		unsigned char I4 : 1;		
	}	format;

	unsigned char P1;   // Prefixes
	unsigned char P2;
	unsigned char P3;
	unsigned char P4;

	struct {
		union {
			struct 	{
				unsigned char B : 1;
				unsigned char X : 1;
				unsigned char R : 1;
				unsigned char W : 1;
				unsigned char prefix : 4;
			};
			unsigned char b;
		};
	}	REX;

	unsigned char O1;   // Opcode
	unsigned char O2;
	unsigned char O3;

	struct {
		union {
			struct {
				unsigned char r_m : 3;
				unsigned char reg : 3;
				unsigned char mod : 2;
			};
			unsigned char b;
		};
	}	modRM;

	struct {
		union {
			struct {
				unsigned char base : 3;
				unsigned char index : 3;
				unsigned char scale : 2;
			};
			unsigned char b;
		};
	}	SIB;

	union {
		cint32 displacement;
		struct {
			unsigned char D1;
			unsigned char D2;
			unsigned char D3;
			unsigned char D4;
		};
	};

	union {
		cint32 immediate;
		struct {
			unsigned char I1;
			unsigned char I2;
			unsigned char I3;
			unsigned char I4;
		};
	};
};

typedef struct CEncoding CEncoding;

#ifdef __cplusplus
extern "C" {
#endif


//---------------------------------------------------------------------
// CEncoding 
//---------------------------------------------------------------------
void cencoding_init(CEncoding *self);
void cencoding_reset(CEncoding *self);
void cencoding_destroy(CEncoding *self);

const char *cencoding_get_label(const CEncoding *self);
const char *cencoding_get_reference(const CEncoding *self);

int cencoding_length(const CEncoding *self);
int cencoding_new_copy(CEncoding *self, const CEncoding *src);

int cencoding_add_prefix(CEncoding *self, unsigned char prefix);
int cencoding_set_immediate(CEncoding *self, int immediate);
int cencoding_set_jump_offset(CEncoding *self, int offset);
void cencoding_set_label(CEncoding *self, const char *label);
void cencoding_set_reference(CEncoding *self, const char *ref);

void cencoding_set_data(CEncoding *self, const void *data, int size);

int cencoding_check_format(const CEncoding *self);
int cencoding_write_code(const CEncoding *self, unsigned char *output);

void cencoding_to_string(const CEncoding *self, char *output);
void cencoding_to_stdout(const CEncoding *self);


#ifdef __cplusplus
}
#endif

#endif


