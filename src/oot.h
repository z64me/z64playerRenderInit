#ifndef OOT_H_INCLUDED
#define OOT_H_INCLUDED

#define OBJECT_EXCHANGE_BANK_MAX 19

extern struct {
	/* 0x0000 */ void  *spaceStart;
	/* 0x0004 */ void  *spaceEnd; // original name: "endSegment"
	/* 0x0008 */ unsigned char     num; // number of objects in bank
	/* 0x0009 */ unsigned char     unk_09;
	/* 0x000A */ unsigned char     mainKeepIndex; // "gameplay_keep" index in bank
	/* 0x000B */ unsigned char     subKeepIndex; // "gameplay_field_keep" or "gameplay_dangeon_keep" index in bank
	struct
	{
		/* 0x00 */ short      id;
		/* 0x04 */ void      *data;
		/* 0x08 */ struct {unsigned vrom; char a[0x1C];}  dmaRequest;
		/* 0x28 */ struct {char a[0x18];}  loadQueue;
		/* 0x40 */ void       *loadMsg;
		/* size = 0x44 */
	} slot[OBJECT_EXCHANGE_BANK_MAX];
} g_object_context;
extern short g_playerobjdep[2];
extern struct { int route; int link_form; } g_savectx;
extern int z_object_slot(void *ctx, int id);
extern struct { int x; } g_context;
#endif /* OOT_H_INCLUDED */
