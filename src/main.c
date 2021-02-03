#include <z64ovl/oot/debug.h>
#include <z64ovl/helpers.h>

#define OverrideLimbDrawOpa void*
#define PostLimbDrawOpa void*

/* the function in gameplay_keep overwrites the arrow texture */
#define GK_ARROW 0x04004380

typedef void fptr(void *);

typedef void fptr_8008F470(
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

/* moved to gameplay_keep to make porting easier */
#if 0
asm("func_8008F470 = 0x80141030");
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
#endif

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
	, void *data)
{
	unsigned char *zobj = (void*)zh_seg2ram(0x06000000);
	unsigned end = *(unsigned*)(zobj + (0x5000 + 0x800 - 4));
	unsigned *has_relocd;
	fptr_8008F470 *playerDraw;
	fptr *exec;
	
	/* zobj doesn't contain embedded overlay */
	if (!end)
		goto L_next;
	
	/* do relocations */
	has_relocd = (unsigned*)(zobj + (0x5000 + 0x800 - 8));
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
	exec = (fptr*)(*has_relocd);
	exec(zobj);
	
	/* draw player model */
L_next:
	playerDraw = (fptr_8008F470*)zh_seg2ram(GK_ARROW);
	playerDraw(
		globalCtx
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
