/* something hacky:
 * force jump from beginning of .text section to
 * the renderinit function in case of reordering
 * or functions not being inlined
 */
void renderinit(uint8_t *zobj);
asm(".section .text \n\
   renderinit_wow:  \n\
   j renderinit     \n\
   nop              \n\
");

#include <z64ovl/oot/debug.h> /* TODO use different header for ntsc10 */
#include <z64ovl/helpers.h>

#define QPT(OFS) (void*)(((char*)(zobj)) + ((OFS) & 0xFFFFFF))
#define QV32(OFS) *(uint32_t*)QPT(OFS)
#define QVLO(OFS) *(uint32_t*)QPT(OFS+4)

void renderinit(uint8_t *zobj)
{
	/* simple test: invisible "hand + master sword" display list */
	QV32(0x06005100) = 0xdf000000;
}
