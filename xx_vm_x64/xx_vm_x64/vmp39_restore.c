/* vmp39_restore.c - VMP 3.9.4 VM bytecode restore (static decode)
 *
 * Based on g0th1c54e4/vmp394-handlers sample analysis:
 *   bytecode = [operand(0-2B)] + [4B encrypted delta], length by handler (4/5/6/8/9/12/13)
 *   handler dispatch = relative delta (vbase += delta)
 *   vkey(r9) rolling key; VR slot = n*8 offset from VM entry rsp
 *
 * Uses handler dict + delta empirical table (extracted from sample trace).
 * Reads bytecode from debuggee memory, falls back to module file (static restore).
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "vmp39_restore.h"

extern const VMP39_HANDLER vmp39_handlers[];
extern const int vmp39_handler_num;
extern const VMP39_DELTA vmp39_deltas[];
extern const int vmp39_delta_num;

#ifdef __cplusplus
extern "C" {
#endif
int DbgMemRead(unsigned long long addr, void* dest, unsigned long long size);
int _plugin_logprintf(const char* format, ...);
int _plugin_logputs(const char* text);
#ifdef __cplusplus
}
#endif

static FILE *g_fout = NULL;

static const VMP39_HANDLER* find_handler(unsigned long long addr)
{
	for (int i = 0; i < vmp39_handler_num; i++)
	{
		if (vmp39_handlers[i].addr == addr)
			return &vmp39_handlers[i];
	}
	return NULL;
}

static int find_delta(unsigned long long handler, unsigned long long dv, unsigned long long *pnext)
{
	int best = -1;
	unsigned int bestmask = 0;
	unsigned int dv32 = (unsigned int)dv;
	for (int i = 0; i < vmp39_delta_num; i++)
	{
		if (vmp39_deltas[i].handler != handler)
			continue;
		{
			unsigned int tdv = (unsigned int)vmp39_deltas[i].dv;
			/* try exact, then high-24, high-16, high-8 bit matches */
			if (tdv == dv32)
			{
				*pnext = vmp39_deltas[i].next;
				return 1;
			}
			if ((tdv & 0xffffff00) == (dv32 & 0xffffff00) && bestmask < 24)
			{
				best = i; bestmask = 24;
			}
			else if ((tdv & 0xffff0000) == (dv32 & 0xffff0000) && bestmask < 16)
			{
				best = i; bestmask = 16;
			}
			else if ((tdv & 0xff000000) == (dv32 & 0xff000000) && bestmask < 8)
			{
				best = i; bestmask = 8;
			}
		}
	}
	if (best >= 0)
	{
		*pnext = vmp39_deltas[best].next;
		return 1;
	}
	return 0;
}

/* decode a single operand byte using the common vPopReg-style chain:
   xor vkey_lo; dec; not; sbb 0x37; rol 1; dec */
static unsigned char op_decode(unsigned char b, unsigned char vk_lo)
{
	unsigned int al = b ^ vk_lo;
	al = (al - 1) & 0xff;
	al = (~al) & 0xff;
	al = (al - 0x37) & 0xff;
	al = ((al << 1) | (al >> 7)) & 0xff;
	al = (al - 1) & 0xff;
	return (unsigned char)al;
}

static void out(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (g_fout) fprintf(g_fout, "%s\n", buf);
	_plugin_logputs(buf);
}

/* File-based fallback: map VA -> file offset using PE sections of the main module */
static char g_modpath[1024] = "";
static unsigned long long g_imgbase = 0;
static FILE *g_modfile = NULL;

void vmp39_set_module(const char *path, unsigned long long imgbase)
{
	unsigned char peh[0x400];
	unsigned int peoff;
	long fsz;
	out("vmp39_set_module: path=%s imgbase=%08llx", path, imgbase);
	strncpy(g_modpath, path, sizeof(g_modpath) - 1);
	g_imgbase = imgbase;
	if (g_modfile) fclose(g_modfile);
	g_modfile = fopen(path, "rb");
	if (g_modfile == NULL) out("  fopen FAILED");
	if (g_modfile)
	{
		fseek(g_modfile, 0, SEEK_END); fsz = ftell(g_modfile);
		fseek(g_modfile, 0, SEEK_SET);
		if (fsz > 0x400 && fread(peh, 1, 0x400, g_modfile) == 0x400)
		{
			peoff = peh[0x3c] | (peh[0x3d] << 8) | (peh[0x3e] << 16) | (peh[0x3f] << 24);
			if (peoff + 0x58 <= fsz && memcmp(peh + peoff, "PE\0\0", 4) == 0)
			{
				unsigned long long ib = 0;
				int i;
				/* PE: sig(4) + COFF(20) = 24; OptionalHeader ImageBase at +24 */
				for (i = 0; i < 8; i++)
					ib |= ((unsigned long long)peh[peoff + 24 + 24 + i]) << (8 * i);
				g_imgbase = ib;
			}
		}
	}
}

static int read_mem(unsigned long long addr, void *buf, int n)
{
	int r = 0;
	int nonzero = 0;
	int i;
	/* prefer file-based static read: does not require running process */
	if (g_modfile)
	{
		unsigned long long rva = addr - g_imgbase;
		unsigned char peh[0x400];
		unsigned int peoff, nsec;
		long fsz;
		int i;
		fseek(g_modfile, 0, SEEK_END); fsz = ftell(g_modfile);
		fseek(g_modfile, 0x3c, SEEK_SET);
		if (fread(peh, 1, 4, g_modfile) != 4) return 0;
		peoff = peh[0] | (peh[1] << 8) | (peh[2] << 16) | (peh[3] << 24);
		if (peoff + 0x18 + 0x200 > fsz) return 0;
		fseek(g_modfile, peoff, SEEK_SET);
		if (fread(peh, 1, 0x18, g_modfile) != 0x18) return 0;
		nsec = peh[6] | (peh[7] << 8);
		{
			unsigned int optsize = peh[20] | (peh[21] << 8);
			for (i = 0; i < nsec; i++)
			{
				unsigned char sh[40];
				unsigned int vsize, vaddr, rawptr, rawsize;
				long off = peoff + 0x18 + optsize + 40 * i;
				fseek(g_modfile, off, SEEK_SET);
				if (fread(sh, 1, 40, g_modfile) != 40) return 0;
				vsize = sh[8] | (sh[9] << 8) | (sh[10] << 16) | (sh[11] << 24);
				vaddr = sh[12] | (sh[13] << 8) | (sh[14] << 16) | (sh[15] << 24);
				rawsize = sh[16] | (sh[17] << 8) | (sh[18] << 16) | (sh[19] << 24);
				rawptr = sh[20] | (sh[21] << 8) | (sh[22] << 16) | (sh[23] << 24);
				if (vaddr <= rva && rva + n <= vaddr + vsize)
				{
					long foff = rawptr + (rva - vaddr);
					if (foff + n > fsz) return 0;
					fseek(g_modfile, foff, SEEK_SET);
					if (fread(buf, 1, n, g_modfile) == (size_t)n) return 1;
					return 0;
				}
			}
		}
	}
	/* fallback to live memory */
	r = DbgMemRead(addr, buf, n);
	if (r)
	{
		unsigned char *pb = (unsigned char *)buf;
		for (i = 0; i < n; i++)
			if (pb[i]) nonzero = 1;
		if (nonzero) return 1;
	}
	return 0;
}

void vmp39_decode(unsigned long long vpc, unsigned long long vkey,
	unsigned long long vbase, unsigned long long bc_base, int bc_len)
{
	unsigned char buf[16];
	unsigned long long dv;
	unsigned long long next_handler = 0;
	int step = 0;
	int unknown = 0;

	if (g_fout) fclose(g_fout);
	g_fout = fopen("vmp39_restore.log", "w");
	out("=== VMP 3.9.4 bytecode restore ===");
	out("init vpc=%08llx vkey=%08llx vbase=%08llx bc=[%08llx,%d]", vpc, vkey, vbase, bc_base, bc_len);
	out("handler dict %d entries, delta table %d entries", vmp39_handler_num, vmp39_delta_num);

	for (step = 0; step < 500; step++)
	{
		const VMP39_HANDLER *h = find_handler(vbase);
		int len;
		int i;

		if (h == NULL)
		{
			out("[%d] unknown handler %08llx", step, vbase);
			unknown++;
			break;
		}

		len = h->len;
		if (len <= 0 || len > 16)
		{
			out("[%d] handler %08llx bad len %d", step, vbase, len);
			break;
		}

		if (vpc < bc_base || vpc > bc_base + bc_len)
		{
			out("[%d] vpc %08llx out of bytecode range", step, vpc);
			break;
		}
		memset(buf, 0, sizeof(buf));
		read_mem(vpc - len, buf, len);

		dv = 0;
		for (i = 0; i < 4; i++)
			dv |= ((unsigned long long)buf[len - 4 + i]) << (8 * i);
		dv ^= (vkey & 0xffffffff);

		if (find_delta(vbase, dv, &next_handler) == 0)
		{
			/* vkey drift: fall back only when this handler has a unique next */
			unsigned long long uniq_next = 0;
			int uniq = 1, first = 1;
			for (int k = 0; k < vmp39_delta_num; k++)
			{
				if (vmp39_deltas[k].handler == vbase)
				{
					if (first) { uniq_next = vmp39_deltas[k].next; first = 0; }
					else if (vmp39_deltas[k].next != uniq_next) { uniq = 0; break; }
				}
			}
			if (uniq && !first)
			{
				next_handler = uniq_next;
				out("[%d] %s  (delta approx fallback)", step, h->sem);
			}
			else
			{
				out("[%d] %s  no delta mapping dv=%08llx, stop", step, h->sem, dv);
				unknown++;
				break;
			}
		}
		if (1)
		{
			char ops[64] = "";
			unsigned char vk_lo = (unsigned char)(vkey & 0xff);
			unsigned char roll = 0;
			int nops = len - 4;
			for (i = 0; i < nops && i < 16; i++)
			{
				unsigned char dec = op_decode(buf[i], vk_lo);
				sprintf(ops + strlen(ops), "%02x", buf[i]);
				if (h->vr_w + h->vr_r > 0)
					roll ^= dec;   /* operand decoded to VR slot offset */
				vk_lo ^= dec;      /* vkey rolls per decoded byte */
			}

			{
				/* decode operand bytes to VR slots (op_decode output = slot offset n*8) */
				char vrnote[128] = "";
				unsigned char vk2 = (unsigned char)(vkey & 0xff);
				int k = 0;
				for (i = 0; i < len - 4 && i < 8; i++)
				{
					unsigned char d = op_decode(buf[i], vk2);
					vk2 ^= d;
					if (d % 8 == 0 && d < 0x100)
					{
						char tmp[32];
						sprintf(tmp, " VR%d", d / 8);
						strcat(vrnote, tmp);
						k++;
					}
				}
				if (k) out("[%d] %s  len=%d ops=%s%s  vpc=%08llx -> %08llx",
					step, h->sem, len, ops, vrnote, vpc, next_handler);
				else out("[%d] %s  len=%d ops=%s  vpc=%08llx -> %08llx",
					step, h->sem, len, ops, vpc, next_handler);
			}

			vkey = (vkey & 0xffffffffffffff00ULL) | vk_lo;
			vpc -= len;
			vbase = next_handler;
		}
		else
		{
			out("[%d] %s  no delta mapping dv=%08llx, stop", step, h->sem, dv);
			unknown++;
			break;
		}
	}

	out("=== done: %d decoded, %d unknown ===", step, unknown);
}

void vmp39_restore_from_x64dbg(unsigned long long vpc, unsigned long long vkey,
	unsigned long long vbase, unsigned long long bc_base, int bc_len)
{
	vmp39_decode(vpc, vkey, vbase, bc_base, bc_len);
}
