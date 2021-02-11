//#define RENDERINIT_OOT_10U /* building for 10u */

#ifdef RENDERINIT_OOT_10U
#  include <z64ovl/oot/u10.h>
#else
#  include <z64ovl/oot/debug.h>
#endif
#include <z64ovl/helpers.h>

struct z64OvlHead
{
	uint32_t textSz;   /* size of .text section   */
	uint32_t dataSz;   /* size of .data section   */
	uint32_t rodataSz; /* size of .rodata section */
	uint32_t bssSz;    /* size of .bss section    */
};

struct z64playerRenderInit
{
   /* offsets are relative to the start of Link's model in ram */
   uint32_t     start;           /* offset of start of overlay    */
   union
   {
      uint32_t  header;          /* offset of overlay header      */
      void      (*exec)(void*);  /* main routine after relocating */
   } u;
};

void main_wowProc(
	z64_global_t *globalCtx
	, void **skeleton
	, vec3s_t *jointTable
	, int32_t dListCount
	, int32_t lod
	, int32_t tunic
	, int32_t boots
	, int32_t face
	, void *overrideLimbDraw
	, void *postLimbDraw
	, void *data
)
{
	uint8_t *model = (void*)zh_seg2ram(0x06000000);
	struct z64playerRenderInit *rinit = (void*)(model + 0x57f8);
	struct z64OvlHead *header;
	void *start;
	uint32_t size;
	
	/* model contains embedded overlay */
	if (rinit->u.header)
	{
		/* not yet relocated (not a ram address) */
		if (!(rinit->u.header & 0x80000000))
		{
			/* relocate overlay from virtual ram to physical ram */
			header = (struct z64OvlHead*)(model + rinit->u.header);
			start = model + rinit->start;
			z_overlay_do_relocation(
				start
				, header
				, (void*)0x80800000
			);
			
			/* update `exec` to point to main routine */
			rinit->u.exec = start;
			
			/* clear instruction cache for .text section memory region */
			size = header->textSz;
			osWritebackDCache(start, size);
			osInvalICache(start, size);
		}
		
		/* run main routine */
		rinit->u.exec(model);
	}
	
	/* draw player model */
	z_player_lib_draw_link(
		(void*)GLOBAL_CONTEXT
		, skeleton
		, jointTable
		, dListCount
		, lod
		, tunic
		, boots
		, face
		, overrideLimbDraw
		, postLimbDraw
		, data
	);
}
