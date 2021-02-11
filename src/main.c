//#define RENDERINIT_OOT_10U /* building for 10u */

#ifdef RENDERINIT_OOT_10U
#  include <z64ovl/oot/u10.h>
#else
#  include <z64ovl/oot/debug.h>
#endif
#include <z64ovl/helpers.h>

#define OverrideLimbDrawOpa void*
#define PostLimbDrawOpa void*

struct ovlHead
{
	uint32_t text; /* size of .text section */
};

typedef void fptr(void *);

#ifdef RENDERINIT_OOT_10U
  asm("func_8008F470 = 0x80079F48");
#else
  asm("func_8008F470 = 0x8008F470");
#endif
extern void func_8008F470(
	z64_global_t *globalCtx
	, void **skeleton
	, vec3s_t *jointTable
	, int32_t dListCount
	, int32_t lod
	, int32_t tunic
	, int32_t boots
	, int32_t face
	, OverrideLimbDrawOpa overrideLimbDraw
	, PostLimbDrawOpa postLimbDraw
	, void *data
);

void main_wowProc(
	z64_global_t *globalCtx
	, void **skeleton
	, vec3s_t *jointTable
	, int32_t dListCount
	, int32_t lod
	, int32_t tunic
	, int32_t boots
	, int32_t face
	, OverrideLimbDrawOpa overrideLimbDraw
	, PostLimbDrawOpa postLimbDraw
	, void *data
)
{
	uint8_t *zobj = (void*)zh_seg2ram(0x06000000);
	uintptr_t *ptr = (uintptr_t*)(zobj + (0x5000 + 0x800 - 4));
	struct ovlHead *header;
	void *start;
	fptr *exec;
	uint32_t size;
	
	/* `ptr` points to 0x57FC in Link's ZOBJ, which will be:
	 *  -> zero, if no embedded overlay is present
	 *  -> or the offset of the overlay's header within `zobj`
	 *     (the u32 preceding this u32 is overlay start offset)
	 *  -> or a pointer to the overlay's main routine
	 */
	
	/* zobj doesn't contain embedded overlay */
	if (!*ptr)
		goto L_next;
	
	/* not yet relocated */
	if (!(*ptr & 0x80000000))
	{
		/* relocate overlay */
		header = (struct ovlHead*)(zobj + *ptr);
		start = zobj + *(ptr-1);
		z_overlay_do_relocation(
			start
			, header
			, (void*)0x80800000
		);
		
		/* update `ptr` to main routine */
		*ptr = (uintptr_t)start;
		
		/* clear instruction cache for .text section memory region */
		size = header->text;
		osWritebackDCache(start, size);
		osInvalICache(start, size);
	}
	exec = (fptr*)(*ptr);
	exec(zobj);
	
	/* draw player model */
L_next:
	func_8008F470(
		(void*)GLOBAL_CONTEXT/*globalCtx*/
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
