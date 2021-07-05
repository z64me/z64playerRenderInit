/*
 * funcTpLinkFinal.c
 *
 * this is the code I wrote for the TP Link 64 Deluxe mod;
 * it has all these cool features:
 *  > sword, sheath, and shield model overrides
 *  > Link has different models for each tunic
 *  > when submerged while wearing the Zora Tunic, a mask appears
 *  > D-Pad equipment toggling
 *  > Iron Boots on B button (disabled in favor of D-Pad)
 *  > the bracer on his arm disappears when he upgrades his gauntlets
 *
 * if you reuse any of my code, please credit me accordingly;
 * additionally, it is all GPL 3.0, which means you are required
 * to open-source anything that is based on it; thank you
 *
 * z64me <z64.me>
 *
 */

/* XXX IMPORTANT! THIS MUST PRECEDE ALL OTHER CODE!
 * something hacky:
 * force jump from beginning of .text section to
 * the renderinit function in case of reordering
 * or functions not being inlined
 */
void renderinit(void *zobj);
asm(".section .text \n\
   renderinit_wow:  \n\
   j renderinit     \n\
   nop              \n\
");

//#define RENDERINIT_OOT_10U /* building for 10u */

#ifdef RENDERINIT_OOT_10U
#  include <z64ovl/oot/u10.h>
#else
#  include <z64ovl/oot/debug.h>
#endif
#include <z64ovl/helpers.h>
#include <z64ovl/oot/sfx.h>

#define QPT(OFS) (void*)(((char*)(zobj)) + ((OFS) & 0xFFFFFF))
#define QV32(OFS) *(uint32_t*)QPT(OFS)
#define QVLO(OFS) *(uint32_t*)QPT(OFS+4)
#define PLAYER_WATER_DEPTH (zh_get_player((void*)GLOBAL_CONTEXT)->actor.water_surface_dist)
#define ARRAY_COUNT(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

#define ICON_SIZE (32*32*4) // 0x1000

#define  ACQUIRED_UPGRADES  (AVAL(Z64GL_SAVE_CONTEXT, uint32_t, 0xA0))
#define  ACQUIRED_STRENGTH  (ACQUIRED_UPGRADES & 0x000000C0)
#define  STRENGTH_BRACELET  0x40
#define  STRENGTH_SILVER    0x80
#define  STRENGTH_GOLD      0xC0

#define  ACQUIRED_GEAR   (AVAL(Z64GL_SAVE_CONTEXT, uint16_t, 0x9C))
#define  ACQUIRED_BOOTS  (ACQUIRED_GEAR & 0x7000)
#define  ACQUIRED_TUNIC  (ACQUIRED_GEAR & 0x0700)
#define  ACQUIRED_SHIELD (ACQUIRED_GEAR & 0x0070)
#define  ACQUIRED_SWORD  (ACQUIRED_GEAR & 0x000F)

#define  EQUIPPED_GEAR   (AVAL(Z64GL_SAVE_CONTEXT, uint16_t, 0x70))
#define  EQUIPPED_SWORD  (EQUIPPED_GEAR & 0x000F)
#define  EQUIPPED_SHIELD (EQUIPPED_GEAR & 0x00F0)
#define  EQUIPPED_TUNIC  (EQUIPPED_GEAR & 0x0F00)
#define  EQUIPPED_BOOTS  (EQUIPPED_GEAR & 0xF000)

/* values */
#define  SWORD_KOKIRI    0x0001
#define  SWORD_MASTER    0x0002
#define  SWORD_GORON     0x0003
#define  SHIELD_DEKU     0x0010
#define  SHIELD_HYLIAN   0x0020
#define  SHIELD_MIRROR   0x0030
#define  TUNIC_KOKIRI    0x0100
#define  TUNIC_GORON     0x0200
#define  TUNIC_ZORA      0x0300
#define  BOOTS_KOKIRI    0x1000
#define  BOOTS_IRON      0x2000
#define  BOOTS_HOVER     0x3000

/* bitfield tests */
#define  SWORD_KOKIRI_BIT    (( 1 <<  0 ) << 0 ) /* 0007 */
#define  SWORD_MASTER_BIT    (( 1 <<  0 ) << 1 )
#define  SWORD_GORON_BIT     (( 1 <<  0 ) << 2 )
#define  SHIELD_DEKU_BIT     (( 1 <<  4 ) << 0 ) /* 0070 */
#define  SHIELD_HYLIAN_BIT   (( 1 <<  4 ) << 1 )
#define  SHIELD_MIRROR_BIT   (( 1 <<  4 ) << 2 )
#define  TUNIC_KOKIRI_BIT    (( 1 <<  8 ) << 0 ) /* 0700 */
#define  TUNIC_GORON_BIT     (( 1 <<  8 ) << 1 )
#define  TUNIC_ZORA_BIT      (( 1 <<  8 ) << 2 )
#define  BOOTS_KOKIRI_BIT    (( 1 << 12 ) << 0 ) /* 7000 */
#define  BOOTS_IRON_BIT      (( 1 << 12 ) << 1 )
#define  BOOTS_HOVER_BIT     (( 1 << 12 ) << 2 )

#include "tp-link.h"

#define  MAGIC_ARMOR_SHIRT   0x2F2D3Eff /* undertunic color */
#define  ZORA_MASK_OFF 1
#define  ZORA_MASK_ON  0

#define  BIGSWORD_IS_BROKEN  (ACQUIRED_SWORD & 0x8)
#define  B_BUTTON_EQUIP      AVAL(Z64GL_SAVE_CONTEXT, uint8_t, 0x0068)

/* make Link's head display list stop branching to Zora Mask */
static int disableZoraMask(void)
{
	uint32_t *walk = (void*)zh_seg2ram(DL_RIGGEDMESH_HEAD);
	while (walk[1] != DL_ZORA_MASK)
		walk += 2;
	walk[0] = 0xdf000000;
	walk[1] = 0x00000000;
	return ZORA_MASK_OFF;
}

/* make Link's head display list branch to Zora Mask */
static int enableZoraMask(void)
{
	uint32_t *walk = (void*)zh_seg2ram(DL_RIGGEDMESH_HEAD);
	while ((*walk >> 24) != 0xdf)
		walk += 2;
	walk[0] = 0xde010000;
	walk[1] = DL_ZORA_MASK;
	return ZORA_MASK_ON;
}

/* this function does the rendering for z64_dpadGear
 * courtesy of z64me <z64.me>
 */
static void z64dpadGearDisplay(void)
{
	static uint16_t prevBoots = -1;
	static uint16_t prevTunic = -1;
	static uint16_t prevSword = -1;
	static uint16_t prevShield = -1;
	static uint16_t prevSwordAcquired = -1;
	static const unsigned char dpad_ci4[] __attribute__ ((aligned (8))) =
	{
		0x00, 0x00, 0x4A, 0x93, 0x52, 0x95, 0x18, 0xC7, 0x5A, 0xD7, 0x4A, 0x53,
		0x00, 0x01, 0x63, 0x19, 0x6B, 0x9B, 0x7B, 0xDF, 0x84, 0x21, 0x8C, 0x63,
		0x94, 0xE5, 0xA5, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x12, 0x22, 0x22, 0x11, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x22, 0x42, 0x42, 0x22, 0x23, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x24, 0x44, 0x44,
		0x44, 0x44, 0x25, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x44, 0x77, 0x43, 0x47, 0x74, 0x41, 0x60, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0x47, 0x77, 0x33, 0x37, 0x77, 0x41, 0x60,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x47, 0x43, 0x33,
		0x33, 0x47, 0x42, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x04, 0x78, 0x33, 0x33, 0x33, 0x38, 0x74, 0x60, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x07, 0x99, 0x33, 0x33, 0x33, 0x39, 0x97, 0x60,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x9A, 0xAB, 0xAB,
		0xAB, 0xAA, 0x98, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x52, 0x47,
		0x89, 0xAA, 0xBB, 0xCB, 0xCB, 0xBA, 0xA9, 0x87, 0x42, 0x55, 0x53, 0x00,
		0x02, 0x14, 0x47, 0x79, 0x9A, 0xAB, 0xBC, 0xCC, 0xCC, 0xBB, 0xAA, 0x99,
		0x77, 0x44, 0x12, 0x30, 0x12, 0x44, 0x77, 0x99, 0xAB, 0xCB, 0xCC, 0xCD,
		0xCC, 0xCB, 0xCB, 0xA9, 0x97, 0x74, 0x42, 0x16, 0x12, 0x24, 0x78, 0x33,
		0xAB, 0xBC, 0xCD, 0xCC, 0xCD, 0xCC, 0xBB, 0xA3, 0x38, 0x74, 0x22, 0x16,
		0x22, 0x44, 0x83, 0x33, 0xBB, 0xCC, 0xDC, 0xDD, 0xDC, 0xDC, 0xCB, 0xB3,
		0x33, 0x84, 0x42, 0x26, 0x12, 0x44, 0x33, 0x33, 0xAB, 0xCC, 0xCD, 0xDD,
		0xDD, 0xCC, 0xCB, 0xA3, 0x33, 0x34, 0x42, 0x16, 0x22, 0x43, 0x33, 0x33,
		0xBB, 0xCD, 0xCD, 0xDD, 0xDD, 0xCD, 0xCB, 0xB3, 0x33, 0x33, 0x42, 0x26,
		0x12, 0x44, 0x33, 0x33, 0xAB, 0xCC, 0xCD, 0xDD, 0xDD, 0xCC, 0xCB, 0xA3,
		0x33, 0x34, 0x42, 0x16, 0x22, 0x44, 0x83, 0x33, 0xBB, 0xCC, 0xDC, 0xDD,
		0xDC, 0xDC, 0xCB, 0xB3, 0x33, 0x84, 0x42, 0x26, 0x12, 0x24, 0x78, 0x33,
		0xAB, 0xBC, 0xCD, 0xCC, 0xCD, 0xCC, 0xBB, 0xA3, 0x38, 0x74, 0x22, 0x16,
		0x12, 0x44, 0x77, 0x99, 0xAB, 0xCB, 0xCC, 0xCD, 0xCC, 0xCB, 0xCB, 0xA9,
		0x97, 0x74, 0x42, 0x16, 0x32, 0x14, 0x47, 0x79, 0x9A, 0xAB, 0xBC, 0xCC,
		0xCC, 0xBB, 0xAA, 0x99, 0x77, 0x44, 0x12, 0x30, 0x03, 0x55, 0x52, 0x47,
		0x89, 0xAA, 0xBB, 0xCB, 0xCB, 0xBA, 0xA9, 0x87, 0x42, 0x55, 0x53, 0x00,
		0x00, 0x66, 0x66, 0x66, 0x68, 0x9A, 0xAB, 0xAB, 0xAB, 0xAA, 0x98, 0x66,
		0x66, 0x66, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x99, 0x33, 0x33,
		0x33, 0x39, 0x97, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x04, 0x78, 0x33, 0x33, 0x33, 0x38, 0x74, 0x60, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x02, 0x47, 0x43, 0x33, 0x33, 0x47, 0x42, 0x60,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x47, 0x77, 0x33,
		0x37, 0x77, 0x41, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x44, 0x77, 0x43, 0x47, 0x74, 0x41, 0x60, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x05, 0x24, 0x44, 0x44, 0x44, 0x44, 0x25, 0x60,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x22, 0x22, 0x42,
		0x42, 0x22, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x31, 0x12, 0x22, 0x22, 0x11, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	};
	uint16_t curTunic = EQUIPPED_TUNIC;
	uint16_t curSword = EQUIPPED_SWORD;
	uint16_t curShield = EQUIPPED_SHIELD;
	uint16_t curBoots = EQUIPPED_BOOTS;
	uint16_t curSwordAcquired = ACQUIRED_SWORD;
	uint8_t hasLoadedIcon = 0;
	z64_global_t *gl = (void*)GLOBAL_CONTEXT;
	z64_if_ctxt_t *p = &gl->if_ctxt;
	void *bButtonIcon = (p->icon_item_segment + (ICON_SIZE * BTN_ID_B));
	/* Link's file is low on space, so use (unused)
	 * parts of gameplay_keep for the D-Pad icons
	 */
	void *tunicIcon = (void*)zh_seg2ram(0x04000400); /* hylian shield */
	void *bootsIcon = (void*)zh_seg2ram(0x04000400 + 32 * 32 * 2);
#if OOT_DEBUG
	void *shieldIcon = (void*)zh_seg2ram(0x04015968); /* beta navi wing */
	void *swordIcon = (void*)zh_seg2ram(0x04015968 + 32 * 32 * 2);
#elif OOT_U_1_0
	void *shieldIcon = (void*)zh_seg2ram(0x04015E08); /* beta navi wing */
	void *swordIcon = (void*)zh_seg2ram(0x04015E08 + 32 * 32 * 2);
#endif
	
	
	void rgba8888_to_rgba5551(void *dst, void *src, int numPixels)
	{
		uint16_t *d = dst;
		uint8_t *s = src;

		#define RGBA5551(R, G, B, A) ( \
			((R >> 3) << (16 -  5)) |   \
			((G >> 3) << (16 - 10)) |   \
			((B >> 3) << (16 - 15)) |   \
			((A >> 7) << (16 - 16))     \
		)
		
		while (numPixels--)
		{
			*d = RGBA5551(s[0], s[1], s[2], s[3]);
			d += 1;
			s += 4;
		}
	}
	
	void refreshIcon(void *dst, int iconId)
	{
		hasLoadedIcon = 1;
		struct { uint32_t romVaddr, dst, sz; } wow = {
			Z64GL_VROM_ICON_ITEM_STATIC + iconId * ICON_SIZE
			, (uint32_t)bButtonIcon
			, ICON_SIZE
		};
		z_file_load(&wow);
		rgba8888_to_rgba5551(dst, bButtonIcon, 32 * 32);
	}
	
	if (curTunic != prevTunic)
	{
		refreshIcon(tunicIcon, 0x41 + (curTunic >> 8) - 1);
		prevTunic = curTunic;
	}
	if (curBoots != prevBoots)
	{
		refreshIcon(bootsIcon, 0x44 + (curBoots >> 12) - 1);
		prevBoots = curBoots;
	}
	if (curShield != prevShield)
	{
		int shield = curShield >> 4;
		if (!shield)
			z_bzero(shieldIcon, 32 * 32 * 2);
		else
			refreshIcon(shieldIcon, 0x3e + shield - 1);
		prevShield = curShield;
	}
	if (curSword != prevSword
		|| curSwordAcquired != prevSwordAcquired /* breaking sword */
	)
	{
		if (!curSword || B_BUTTON_EQUIP == 0xff)
			z_bzero(swordIcon, 32 * 32 * 2);
		else
			refreshIcon(swordIcon, B_BUTTON_EQUIP);
		prevSword = curSword;
		prevSwordAcquired = curSwordAcquired;
	}
	/* restore what was on the B button before
	 * it was used as an intermediate buffer
	 */
	if (hasLoadedIcon)
		gfx_update_item_icon(gl, BTN_ID_B);
	
#if 1
	z64_disp_buf_t *buf = &ZQDL(gl, overlay);
	
	/* initialize material */
	#if OOT_DEBUG
		gSPDisplayList(buf->p++, 0x801269D0);
	#elif	OOT_U_1_0
		gSPDisplayList(buf->p++, 0x800F84A0);
   #elif MM_U_1_0
      gSPDisplayList(buf->p++, 0x801C1640);
	#endif
	gDPSetCombineLERP(
		buf->p++
		, PRIMITIVE
		, ENVIRONMENT
		, TEXEL0
		, ENVIRONMENT
		, TEXEL0
		, 0
		, PRIMITIVE
		, 0
		, PRIMITIVE
		, ENVIRONMENT
		, TEXEL0
		, ENVIRONMENT
		, TEXEL0
		, 0
		, PRIMITIVE
		, 0
	);
	gDPSetPrimColor(buf->p++, 0, 0, -1, -1, -1, p->a_alpha);
	gDPSetEnvColor(buf->p++, 0, 0, 0, 255);
	gDPPipeSync(buf->p++);
	
	/* for each icon */
	void *texList[] = { tunicIcon, bootsIcon, shieldIcon, swordIcon };
	const int diffSquare = 40;
	const int diffSquareHalf = 20;
	struct {
		int x;
		int y;
	} advance[] = {
		{ -2, diffSquare } /* tunic -> boots */
		, { -diffSquare / 2, -diffSquare / 2 } /* boots -> shield */
		, { diffSquare, 0 } /* shield -> sword */
		, { 0, 0 }
	};
	float screenX = 32;
	float screenY = 78;
	const float iconSquare = 16;
	zh_set_palette(buf, (uintptr_t)dpad_ci4);
		gDPLoadTextureBlock_4b(
		   buf->p++
		   , (dpad_ci4 + 0x20), G_IM_FMT_CI
		   , 32, 32
		   , 0
		   , G_TX_CLAMP, G_TX_CLAMP
		   , G_TX_NOMASK, G_TX_NOMASK
		   , G_TX_NOLOD, G_TX_NOLOD
		);
		gSPTextureRectangle(
			buf->p++
			, qs102(screenX - 3) & ~3
			, qs102((screenY - 2)+ diffSquareHalf) & ~3
			, qs102((screenX - 3)+ 20.0f) & ~3
			, qs102((screenY - 2)+ diffSquareHalf + 20.0f) & ~3
			, G_TX_RENDERTILE
			, qu105(0), qu105(0)
			, qu510(32.0f/*imgW*/ / 20.0f)
			, qu510(32.0f/*imgH*/ / 20.0f)

		);
	/* disable palette now */
	//gDPFullSync(buf->p++); // not necessary?
	gSPSetOtherMode(buf->p++, G_SETOTHERMODE_H, G_MDSFT_TEXTLUT, 2, G_TT_NONE);
#if 1
	for (int i = 0; i < ARRAY_COUNT(texList); ++i)
	{
		gDPLoadTextureBlock(
		   buf->p++
		   , texList[i], G_IM_FMT_RGBA, G_IM_SIZ_16b
		   , 32, 32
		   , 0
		   , G_TX_CLAMP, G_TX_CLAMP
		   , G_TX_NOMASK, G_TX_NOMASK
		   , G_TX_NOLOD, G_TX_NOLOD
		);
		gSPTextureRectangle(
			buf->p++
			, qs102(screenX) & ~3
			, qs102(screenY) & ~3
			, qs102(screenX + iconSquare) & ~3
			, qs102(screenY + iconSquare) & ~3
			, G_TX_RENDERTILE
			, qu105(0), qu105(0)
			, qu510(32.0f/*imgW*/ / iconSquare)
			, qu510(32.0f/*imgH*/ / iconSquare)
		);
		screenX += advance[i].x;
		screenY += advance[i].y;
	}
#endif
#endif
	
#if 0 /* testing */
	zh_draw_ui_sprite(
		&ZQDL(gl, overlay)
		, &(gfx_texture_t){
			.timg = (uintptr_t)tunicIcon
			, .width = 32
			, .height = 32
			, .fmt = G_IM_FMT_RGBA
			, .bitsiz = G_IM_SIZ_16b
		}
		, &(gfx_screen_tile_t){
			.x = 32
			, .y = 32
			, .origin_anchor = G_TX_ANCHOR_C
			, .width = 32
			, .height = 32
		}
		, 0xffffff00 | p->a_alpha
	);
#endif
}

/* this function allows player to cycle through
 * acquired tunics/boots/swords/shields with the D-Pad
 * D-Pad Up: Next Tunic
 * D-Pad Down: Next Boots
 * D-Pad Left: Next Shield
 * D-Pad Right: Next Sword
 * courtesy of z64me <z64.me>
 */
static inline void z64_dpadGear(void)
{
	z64_global_t *gl = (void*)GLOBAL_CONTEXT;
	z64_player_t *player = zh_get_player(gl);
	z64_controller_t *input = &gl->common.input[0].raw;
	static int8_t dPadUp = -1;
	static int8_t dPadDown = -1;
	static int8_t dPadLeft = -1;
	static int8_t dPadRight = -1;
	const uint16_t tunicList[] = {
		TUNIC_KOKIRI_BIT
		, TUNIC_GORON_BIT
		, TUNIC_ZORA_BIT
	};
	const uint16_t swordList[] = {
		SWORD_KOKIRI_BIT
		, SWORD_MASTER_BIT
		, SWORD_GORON_BIT
	};
	const uint16_t bootsList[] = {
		BOOTS_KOKIRI_BIT
		, BOOTS_IRON_BIT
		, BOOTS_HOVER_BIT
	};
	const uint16_t shieldList[] = {
		SHIELD_DEKU_BIT
		, SHIELD_HYLIAN_BIT
		, SHIELD_MIRROR_BIT
		, 0 /* special case for unequipping shield */
	};
	int rval;
	
	/* returns index within provided list if successful ( >= 0 )
	 * returns value < 0 otherwise
	 */
	int gearShouldUpdate(
		int8_t *prevButton
		, int8_t curButton
		, const uint16_t *list
		, int listNum
		, int next
		, int acquired
	)
	{
		int rval = -1;
		int exitPoint = -1;
		
		/* button pressed for one frame */
		if (curButton && curButton != *prevButton)
		{
			/* determine next item in list */
			while (1)
			{
				/* roll back to beginning of list */
				if (next >= listNum && (next = 0) == exitPoint)
					break;
				
				/* exit point hasn't been set yet */
				if (exitPoint < 0)
				{
					exitPoint = next - 1;
					if (exitPoint < 0)
						exitPoint = listNum - 1;
				}
				
				if (!list[next] /* special unequip case */
					|| (acquired & list[next]) /* player has this item */
				)
				{
					play_sound_global_once(NA_SE_IT_SWORD_IMPACT);
					//play_sound_global_once(NA_SE_IT_SWORD_PUTAWAY);
					rval = next;
					break;
				}
				
				/* reached exit point */
				if ((++next) == exitPoint)
					break;
			}
			
			/* player has only one or zero items to choose from */
			if (rval == -1)
				play_sound_global_once(NA_SE_SY_ERROR);
		}
		*prevButton = curButton;
		
		return rval;
	}
	
	/* do not execute this function on the pause screen */
	if (zh_gameIsPaused())
		return;
	
	/* do not execute if the interface is disabled */
	if (Z64GL_INTERFACE_DISABLED)
		return;
		
	/* do not execute if Fairy Bow on B */
	if (B_BUTTON_EQUIP == 0x03)
		return;
	
	/* d-pad changed sword */
	if ((rval = gearShouldUpdate(
		&dPadRight
		, input->dr
		, swordList
		, ARRAY_COUNT(swordList)
		, EQUIPPED_SWORD
		, ACQUIRED_SWORD
	)) >= 0)
	{
		char action[] = {
			0x04 /* PLAYER_AP_SWORD_KOKIRI */
			, 0x03 /* PLAYER_AP_SWORD_MASTER */
			, 0x05 /* PLAYER_AP_SWORD_BGS */
		};
		if (rval == 2 && BIGSWORD_IS_BROKEN)
		{
			action[2] = 0x35; /* PLAYER_AP_SWORD_BROKEN */
			B_BUTTON_EQUIP = 85; /*brokenBGS*/
		}
		else
			B_BUTTON_EQUIP = rval + 59/*kokiri*/;
		
		EQUIPPED_GEAR = (EQUIPPED_GEAR & 0xfff0) | (rval + 1);
		player->sword_idx = action[rval];
	}
	
	/* d-pad changed shield */
	if ((rval = gearShouldUpdate(
		&dPadLeft
		, input->dl
		, shieldList
		, ARRAY_COUNT(shieldList)
		, (EQUIPPED_SHIELD >> 4) ? (EQUIPPED_SHIELD >> 4) : ARRAY_COUNT(shieldList)
		, ACQUIRED_SHIELD
	)) >= 0)
	{
		player->shield_idx = (rval + 1/*PLAYER_SHIELD_DEKU*/) & 3/*wrap*/;
		EQUIPPED_GEAR = (EQUIPPED_GEAR & 0xff0f) | (player->shield_idx << 4);
	}
	
	/* d-pad changed tunic */
	if ((rval = gearShouldUpdate(
		&dPadUp
		, input->du
		, tunicList
		, ARRAY_COUNT(tunicList)
		, EQUIPPED_TUNIC >> 8
		, ACQUIRED_TUNIC
	)) >= 0)
	{
		EQUIPPED_GEAR = (EQUIPPED_GEAR & 0xf0ff) | ((rval + 1) << 8);
		player->tunic_idx = rval;
	}
	
	/* d-pad changed boots */
	if ((rval = gearShouldUpdate(
		&dPadDown
		, input->dd
		, bootsList
		, ARRAY_COUNT(bootsList)
		, EQUIPPED_BOOTS >> 12
		, ACQUIRED_BOOTS
	)) >= 0)
	{
		/* if Link is in water, toggle back to Kokiri Boots
		 * instead of Hover Boots
		 */
		if (rval == 2 && PLAYER_WATER_DEPTH >= 0)
			rval = 0;
		EQUIPPED_GEAR = (EQUIPPED_GEAR & 0x0fff) | ((rval + 1) << 12);
		player->boot_idx = rval + 0/* PLAYER_BOOTS_NORMAL */;
		Player_SetBootData(gl, player);
	}
	
	z64dpadGearDisplay();
}


#if 0
#define WANT_IRON_BOOTS_ON_B_BUTTON
/* this function allows toggling Iron Boots with the B button
 * while swimming, as long as the player has acquired them!
 * courtesy of z64me <z64.me>
 */
static inline void z64_swimIronBootsB(void)
{
	z64_global_t *gl = (void*)GLOBAL_CONTEXT;
	z64_player_t *player = zh_get_player(gl);
	z64_if_ctxt_t *p = &gl->if_ctxt;
	static int bButtonIronBoots = 1;
	const int iconNormal = 1;
	const int iconChanged = 2;
	const int iconIronBoots = 0x45; /* index within icon bank */
	
	/* do not execute this function on the pause screen */
	if (zh_gameIsPaused())
		return;
	
	/* do not execute if the interface is disabled */
	if (Z64GL_INTERFACE_DISABLED)
		return;
	
	/* Link has not acquired Iron Boots; exit */
	if (!(ACQUIRED_BOOTS & BOOTS_IRON))
		return;
	
	/* Link is in deep water, so allow the toggle */
	/* TODO better swimming detection */
	if (p->b_alpha <= p->a_alpha
		&& PLAYER_WATER_DEPTH >= 40
	)
	{
		z64_controller_t *input = &gl->common.input[0].raw;
		z64_controller_t *inputPrev = &gl->common.input[0].raw_prev;
		
		/* set B button alpha = A button alpha */
		p->b_alpha = p->a_alpha;
		
		/* Iron Boots aren't on B yet */
		if (bButtonIronBoots != iconChanged)
		{
			struct { uint32_t romVaddr, dst, sz; } wow = {
				Z64GL_VROM_ICON_ITEM_STATIC + iconIronBoots * ICON_SIZE
				, (uint32_t)(p->icon_item_segment + (ICON_SIZE * BTN_ID_B))
				, ICON_SIZE
			};
			z_file_load(&wow);
			bButtonIronBoots = iconChanged;
		}
		
		static int prevB = -1;
		if (input->b && prevB != input->b)
		{
			if (EQUIPPED_BOOTS == BOOTS_IRON)
			{
				EQUIPPED_GEAR = (EQUIPPED_GEAR & 0x0fff) | BOOTS_KOKIRI;
				player->boot_idx = 0; /* PLAYER_BOOTS_NORMAL */
			}
			else
			{
				EQUIPPED_GEAR = (EQUIPPED_GEAR & 0x0fff) | BOOTS_IRON;
				player->boot_idx = 1; /* PLAYER_BOOTS_IRON */
			}
		}
		prevB = input->b;
	}
	/* no longer in water; restore previous B button icon */
	else if (bButtonIronBoots == iconChanged)
	{
		gfx_update_item_icon(gl, BTN_ID_B);
		bButtonIronBoots = iconNormal;
	}
}
#endif

void renderinit(void *zobj)
{
	static uint16_t prevTunic = -1;
	static uint16_t prevSword = -1;
	static uint8_t hasBracer = 1;
	static uint8_t zoraMask = ZORA_MASK_OFF;
	uint16_t curTunic = EQUIPPED_TUNIC;
	uint16_t curSword = EQUIPPED_SWORD;
	
#if 0
	{
		z64_global_t *gl = (void*)GLOBAL_CONTEXT;
		z64_actor_t *player = &zh_get_player(gl)->actor;
		zh_draw_debug_text(gl, -1, 1, 1, "float %f", player->water_surface_dist);
		//zh_draw_debug_text(gl, -1, 1, 1, "upgrades %08X", ACQUIRED_UPGRADES);
	}
#endif
#if 0
	{
		z64_global_t *gl = (void*)GLOBAL_CONTEXT;
		zh_draw_debug_text(gl, -1, 1, 1, "%04x", ACQUIRED_GEAR);
	}
#endif
	
#ifdef WANT_IRON_BOOTS_ON_B_BUTTON
	z64_swimIronBootsB();
#endif
	z64_dpadGear();

	/* underwater Zora Tunic mask */
	if (curTunic == TUNIC_ZORA)
	{
		/* if underwater */
		if (PLAYER_WATER_DEPTH >= 50)
		{
			/* and not wearing Zora Mask */
			if (zoraMask == ZORA_MASK_OFF)
				zoraMask = enableZoraMask();
		}
		/* above water and wearing mask */
		else if (zoraMask == ZORA_MASK_ON)
			zoraMask = disableZoraMask();
	}
	/* player changed tunic, but Zora Mask is still on */
	else if (zoraMask == ZORA_MASK_ON)
		zoraMask = disableZoraMask();
	
	/* hide arm bracer if player has gold/silver gauntlets */
	/* TODO Link gets the upgrade before the chest is open */
	if (hasBracer && ACQUIRED_STRENGTH >= STRENGTH_SILVER)
	{
		uint32_t *walk = (void*)zh_seg2ram(DL_RIGGEDMESH_FOREARM_L);
		hasBracer = 0;
		for ( ; (*walk >> 24) != 0xdf; walk += 2)
		{
			if ((*walk >> 24) != 0xde)
				continue;
			
			/* we detect bracer by its material */
			if (walk[1] == MTL_BRACER)
			{
				walk[0] = 0xdf000000;
				walk[1] = 0x00000000;
				break;
			}
			/* another exit condition */
			if (*walk & 0x00ff0000)
				break;
		}
	}
	
#if 0 /* we do the ordon shield using the manifest now */
	/* player equipped a different shield */
	if (curShield != prevShield)
	{
		switch (curShield)
		{
			case SHIELD_DEKU:
				/* override Hylian Shield with Ordon Shield */
				QVLO(DL_SHIELD_2_PROXY) = DL_SHIELD_1;
				break;
			case SHIELD_HYLIAN:
				QVLO(DL_SHIELD_2_PROXY) = DL_SHIELD_2;
				break;
		}
		prevShield = curShield;
	}
#endif
	
	/* override Master Sword and Sheath models */
	if (curSword != prevSword)
	{
		/* tweak positioning of sword handle in sheath
		 * XXX careful: assumes a certain play-as manifest structure
		 */
		uint16_t *matrix_Xloc = (void*)zh_seg2ram(0x06005028);
		*matrix_Xloc = (int16_t)-768;
		switch (curSword)
		{
			case SWORD_KOKIRI:
				QVLO(DL_HILT_2_PROXY) = DL_HILT_1;
				QVLO(DL_BLADE_2_PROXY) = DL_BLADE_1;
				QVLO(DL_SHEATH_PROXY) = DL_SHEATH_KOKIRI;
				break;
			case SWORD_MASTER:
				QVLO(DL_HILT_2_PROXY) = DL_HILT_2;
				QVLO(DL_BLADE_2_PROXY) = DL_BLADE_2;
				QVLO(DL_SHEATH_PROXY) = DL_SHEATH;
				break;
			case SWORD_GORON:
				QVLO(DL_HILT_2_PROXY) = DL_HILT_3;
				QVLO(DL_SHEATH_PROXY) = DL_SHEATH_BIGGORON;
				*matrix_Xloc = (int16_t)-915;
				break;
		}
		prevSword = curSword;
	}
	
	/* override riggedmesh with one of the available tunics */
	if (curTunic != prevTunic)
	{
		unsigned proxySize = sizeof(proxy_Kokiri);
		const void *proxy = 0;
		void *overwrite = (void*)zh_seg2ram(PROXY_RIGGEDMESH);
		/* update primcolor of undershirt */
		switch (curTunic)
		{
			case TUNIC_KOKIRI:
				proxy = proxy_Kokiri;
				QVLO(MTL_UNDERSHIRT_PRIMCOLOR) = -1; /* white */
				break;
			case TUNIC_GORON:
				proxy = proxy_Goron;
				QVLO(MTL_UNDERSHIRT_PRIMCOLOR) = MAGIC_ARMOR_SHIRT;
				break;
			case TUNIC_ZORA:
				proxy = proxy_Zora;
				QVLO(MTL_UNDERSHIRT_PRIMCOLOR) = -1; /* white */
				break;
		}
		z_bcopy(proxy, overwrite, proxySize);
		prevTunic = curTunic;
	}
}

