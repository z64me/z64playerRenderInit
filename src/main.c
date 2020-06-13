#include <z64ovl/oot/debug.h>
#include <z64ovl/helpers.h>
#include "oot-debug.h" /* TODO remove oot-debug; rely only on z64ovl */

void main(void)
{
	short playerobj;
	int form = *(int*)(0x8015E660 + 4); /* TODO make game/version agnostic */
	playerobj = g_playerobjdep[form];
	
	int slot = z_object_slot(&g_object_context, playerobj);
	if (slot < 0)
		return;
	
	unsigned char *zobj = g_object_context.slot[slot].data;
	if (!zobj)
		return;
	unsigned end = *(unsigned*)(zobj + (0x5000 + 0x800 - 4));
	
	/* zobj doesn't contain embedded overlay */
	if (!end)
		return;
	
	/* do relocations */
	unsigned *has_relocd = (unsigned*)(zobj + (0x5000 + 0x800 - 8));
	if (!*has_relocd)
	{
		unsigned ofs = *(unsigned*)(zobj + (end - 4));
		unsigned *section = (void*)(zobj + (end - ofs));
		unsigned start = ((unsigned)section) - (
			section[0] /* text */
			+ section[1] /* data */
			+ section[2] /* rodata */
		);
		start -= start & 15;
		z_overlay_do_relocation(
			(void*)start
			, section
			, (void*)0x80800000
		);
		*has_relocd = start;
	}
	typedef void fptr(void *);
	fptr *exec = (fptr*)(*has_relocd);
	exec(zobj);
}
