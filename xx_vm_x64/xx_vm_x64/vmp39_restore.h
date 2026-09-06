/* vmp39_restore.h - VMP 3.9.4 bytecode restore module */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _VMP39_HANDLER {
	unsigned long long addr;
	int len;
	int vk_xor;
	int vr_w;
	int vr_r;
	int ins_idx;
	const char *sem;
} VMP39_HANDLER;

typedef struct _VMP39_DELTA {
	unsigned long long handler;
	unsigned long long dv;
	unsigned long long next;
} VMP39_DELTA;

void vmp39_decode(unsigned long long vpc, unsigned long long vkey,
	unsigned long long vbase, unsigned long long bc_base, int bc_len);

void vmp39_restore_from_x64dbg(unsigned long long vpc, unsigned long long vkey,
	unsigned long long vbase, unsigned long long bc_base, int bc_len);

void vmp39_set_module(const char *path, unsigned long long imgbase);

#ifdef __cplusplus
}
#endif
