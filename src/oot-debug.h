#ifndef OOT_DEBUG_H_INCLUDED
#define OOT_DEBUG_H_INCLUDED
/* external variables */
asm("g_playerobjdep    = 0x80127520;");
asm("g_savectx         = 0x8015E660;");
asm("g_context         = 0x80212020;");
asm("g_object_context  = (g_context + 0x117A4);");

/* external functions */
asm("z_object_slot     = 0x8009812C;");

#include "oot.h"

#endif /* OOT_DEBUG_H_INCLUDED */
