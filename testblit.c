#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <ctype.h>

#include "cencoding.h"
#include "ckeywords.h"
#include "cinstruct.h"
#include "cinstset.h"
#include "ctoken.h"
#include "cscanner.h"
#include "csynthesis.h"
#include "cparser.h"
#include "casmpure.h"

//dst:DWORD, src:DWORD, dpitch:DWORD, spitch:DWORD, mask:DWORD, width:DWORD, height:DWORD
typedef void (*MaskBlitProc)(void *dst, const void *src, long dpitch, long spitch,
	int width, int height, unsigned long mask);

int BMP1[16] = {
	1, 1, 1, 1,
	1, 1, 1, 1,
	1, 1, 1, 1,
	1, 1, 1, 1,
};

int BMP2[16] = {
	0, 0, 0, 0,
	0, 2, 2, 0,
	0, 2, 2, 0,
	0, 0, 0, 0,
};

void testBlit()
{
	CAssembler *casm;
	MaskBlitProc MaskBlit;
	int ret, i, j;
	
	// create assembler
	casm = casm_create();

	// load assembly source
	ret = casm_loadfile(casm, "testblit.asm");

	if (ret != 0) {
		printf("error: %s\n", casm->error);
		casm_release(casm);
	}

	MaskBlit = (MaskBlitProc)casm_callable(casm, NULL);

	if (ret != 0) {
		printf("error: %s\n", casm->error);
		casm_release(casm);
	}

	casm_dumpinst(casm, stdout);

	MaskBlit(BMP1, BMP2, 16, 16, 4, 4, 0);

	for (j = 0; j < 4; j++) {
		for (i = 0; i < 4; i++) 
			printf("%x ", BMP1[j * 4 + i]);
		printf("\n");
	}

	casm_release(casm);
}


//! src: ctoken.c, cscanner.c, csynthesis.c, cparser.c, casmpure.c
//! exe: cencoding.c, cinstruct.c, cinstset.c, ckeywords.c, cloader.c
int main(void)
{
	testBlit();
	return 0;
}


