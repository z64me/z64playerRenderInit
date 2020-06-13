#include <z64ovl/oot/debug.h>
#include <z64ovl/helpers.h>

void main(void *zobj_)
{
	/* persistent storage (.bss) */
	static vec3s_t earMod; /* _DAT_80858ac8, _DAT_80858aca, _DAT_80858acc */
	static int16_t persist0; /* _DAT_80858ace */
	static int16_t persist1; /* _DAT_80858ad0 */
	
	z64_global_t *gl = (void*)(GLOBAL_CONTEXT);
	unsigned char *zobj = zobj_;
	unsigned ofs = 0x5800 /*eyes + mouths + lut (0600005800)*/;
	Mtx *ear_mtx = (void*)(zobj + ofs);
	z64_player_t *link = zh_get_player(gl);
	vec3s_t *r = &earMod;
	vec3s_t ear;
	
	/* this is from FUN_8085002c */
	int16_t angle;
	float fVar1;
	float fVar2;
	float fVar3;
	float fVar4;
	float fVar5;
	
	/* update ear motion variables */
	persist0 = (persist0 - (persist0 >> 3)) + (-earMod.x >> 2);
	persist1 = (persist1 - (persist1 >> 3)) + (-earMod.y >> 2);
	angle = (link->actor.dir.y - link->actor.rot.y) * 0x10000 >> 0x10;
	fVar1 = z_cos_s(angle);
	fVar2 = external_func_80033F20(2.0f);
	fVar5 = link->actor.xz_speed;
	fVar3 = z_sin_s(angle);
	fVar4 = external_func_80033F20(2.0f);
	persist0 += ((fVar2 + 10.00000000) * fVar5 * -200.00000000 * fVar1 / 4.0f); // FIXME / 4 was >> 2 before
	persist1 += ((fVar4 + 10.00000000) * link->actor.xz_speed * 100.00000000 * fVar3 / 4.0f); // FIXME / 4 was >> 2 before
	if (persist0 < 0x1771) {
		if (persist0 < -6000) {
			persist0 = -6000;
		}
	}
	else {
		persist0 = 6000;
	}
	if (persist1 < 0x1771) {
		if (persist1 < -6000) {
			persist1 = -6000;
		}
	}
	else {
		persist1 = 6000;
	}
	earMod.x = earMod.x + persist0;
	earMod.z = earMod.x >> 1;
	if (-1 < earMod.x) {
		earMod.z = 0;
	}
	earMod.y = earMod.y + persist1;
	
	/* generate ear matrices */
	r = (void*)0x803F8C68/*_DAT_80858ac8*/; // FIXME it should work without this line, but doesn't
	/* right */
	ear.x = r->y + 0x03E2; ear.y = r->z + 0x0D8E; ear.z = r->x + 0xCB76;
	z_matrix_translate_3f_800D1694(97.0f, -1203.0f, -240.0f, &ear);
	z_matrix_top_to_fixed(ear_mtx, 0, 0);
	/* left */
	ear.x = r->y + 0xFC1E; ear.y = 0xF242 - r->z; ear.z = r->x + 0xCB76;
	z_matrix_translate_3f_800D1694(97.0f, -1203.0f, 240.0f, &ear);
	z_matrix_top_to_fixed(ear_mtx + 1, 0, 0);
}
