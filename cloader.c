//=====================================================================
//
// cloader.c - source loader
//
// NOTE:
// for more information, please see the readme file.
//
//=====================================================================
#include "cloader.h"

//---------------------------------------------------------------------
// CLink interface
//---------------------------------------------------------------------
static CLink *clink_create(const CEncoding *encoding)
{
	CLink *link;

	link = (CLink*)malloc(sizeof(CLink));
	assert(link);

	iqueue_init(&link->head);
	cencoding_new_copy(&link->encoding, encoding);
	link->offset = 0;
	link->size = 0;

	return link;
}

static void clink_release(CLink *link)
{
	assert(link);
	cencoding_destroy(&link->encoding);
}


//---------------------------------------------------------------------
// CLoader interface
//---------------------------------------------------------------------
CLoader *cloader_create(void)
{
	CLoader *loader;
	loader = (CLoader*)malloc(sizeof(CLoader));
	assert(loader);
	iqueue_init(&loader->head);
	loader->error = (char*)malloc(1024);
	assert(loader->error);
	loader->error[0] = 0;
	loader->errcode = 0;
	loader->linear = 0;
	loader->output = NULL;
	loader->lineno = 0;
	return loader;
}

void cloader_reset(CLoader *loader)
{
	assert(loader);
	while (!iqueue_is_empty(&loader->head)) {
		CLink *link = iqueue_entry(loader->head.next, CLink, head);
		iqueue_del(&link->head);
		clink_release(link);
	}
	loader->error[0] = 0;
	loader->errcode = 0;
	loader->linear = 0;
	loader->output = NULL;
	loader->lineno = 0;
}

void cloader_release(CLoader *loader)
{
	assert(loader);
	cloader_reset(loader);
	if (loader->error) {
		free(loader->error);
		loader->error = NULL;
	}
}

int cloader_new_encoding(CLoader *loader, const CEncoding *encoding)
{
	CLink *link;
	link = clink_create(encoding);
	assert(link);
	link->lineno = ++loader->lineno;
	iqueue_add_tail(&link->head, &loader->head);
	return 0;
}

int cloader_get_codesize(CLoader *loader)
{
	struct IQUEUEHEAD *p;
	int size = 0;
	assert(loader);
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		size += cencoding_length(&link->encoding);
	}
	return size;
}

unsigned long cloader_resolve_label(CLoader *loader, const char *label)
{
	struct IQUEUEHEAD *p;
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		CEncoding *encoding = &link->encoding;
		if (cencoding_get_label(encoding)) {
			if (strcmp(encoding->label, label) == 0) {
				return (long)link->offset;
			}
		}
	}
	return 0;
}

int cloader_output(CLoader *loader, unsigned char *output)
{
	struct IQUEUEHEAD *p;
	assert(loader);

	loader->output = output;
	loader->linear = (cuint32)output;

	// encoding instructions
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		CEncoding *encoding = &link->encoding;
		int size;
		size = cencoding_write_code(encoding, loader->output);
		link->offset = loader->linear;
		link->size = size;
		loader->linear += size;
		loader->output += size;
	}

	// resolve labels
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		CEncoding *encoding = &link->encoding;
		unsigned char *offset = (unsigned char*)link->offset;
		if (cencoding_get_reference(encoding)) {
			const char *label = cencoding_get_reference(encoding);
			long linear = cloader_resolve_label(loader, label);
			if (linear == 0) {
				strncpy(loader->error, "not find label: ", 40);
				strncat(loader->error, label, 100);
				loader->errcode = link->lineno;
				return -1;
			}
			if (encoding->relative == 0) {
				cencoding_set_immediate(encoding, linear);
			}
			else {
				long diff = linear - (link->offset + link->size);
				cencoding_set_jump_offset(encoding, diff);
			}
			cencoding_write_code(encoding, offset);
		}
	}
	
	return 0;
}


void cloader_print(const CLoader *loader)
{
	struct IQUEUEHEAD *p;
	static char line[400];
	for (p = loader->head.next; p != &loader->head; p = p->next) {
		CLink *link = iqueue_entry(p, CLink, head);
		CEncoding *encoding = &link->encoding;
		cencoding_to_string(encoding, line);
		printf("%s\n", line);
	}
}


