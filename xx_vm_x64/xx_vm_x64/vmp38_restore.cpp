/* vmp38_restore.cpp - VMP 3.8 VM restore: dynamic vsp tracking + handler semantics
 *
 * Kanxue 280283 (VMP 3.8.1 handler features):
 *   - vm_start = pushfd, vm_exit = popfd
 *   - push: vsp-8 (imm32 obfuscated), pop: vsp+8
 *   - add/sub: push;push;not;mov;mov;add;not
 *   - Detect handlers by vsp change; get real operands via emulation
 *
 * Uses DbgDisasmAt for disassembly + lightweight manual vsp tracking
 * (independent mini-simulator, does NOT call xx_execute to avoid
 *  clashing with the 3.5 engine's global state).
 */

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unicorn/unicorn.h>
#include "bridgemain.h"
#include "_scriptapi_module.h"
#include "_scriptapi_memory.h"
#include "_scriptapi_misc.h"
#include "_scriptapi_debug.h"
#include "_scriptapi_register.h"
#include "vmp38_restore.h"
/* bounded string append - prevents RTC#2 stack corruption */
#define SCAT(dst, src) strncat((dst), (src), sizeof(dst) - strlen(dst) - 1)


/* _plugin_logputs/_plugin_logprintf declared in _plugins.h (via _scriptapi_module.h) */

int g_dyn_running = 0;
int g_dyn_stop = 0;
unsigned long long g_dyn_start = 0;
static FILE *g_fout = NULL;
static char g_fout_path[512] = "";

/* Log session management: every VMP38 restore action (static restore,
   dynamic trace, scan) opens its OWN log file under "<x64dbg.exe dir>\xxvm\"
   named to the millisecond so concurrent runs never collide:
     xxvm\xxvm_restore_20260805-112345-678.txt
     xxvm\xxvm_dyn_20260805-112345-678.txt
   x64dbg's GUI log buffer is fixed (~1MB) and silently DROPS the earliest
   lines on long traces, so the GUI export is incomplete; the file is the
   full record. */
static int vmp38_log_open(const char *prefix)
{
	if (g_fout) return 0;   /* already inside a session (nested call) */
	{
		char exedir[MAX_PATH] = "";
		GetModuleFileNameA(NULL, exedir, sizeof(exedir));
		/* strip the exe file name -> directory */
		{
			char *sl = strrchr(exedir, '\\');
			if (sl) *sl = 0;
		}
		/* ensure xxvm subdir exists */
		{
			char dir[MAX_PATH];
			_snprintf(dir, sizeof(dir), "%s\\xxvm", exedir);
			CreateDirectoryA(dir, NULL);
			_snprintf(g_fout_path, sizeof(g_fout_path), "%s\\xxvm\\xxvm_%s_", exedir, prefix);
		}
		/* millisecond timestamp */
		{
			SYSTEMTIME st;
			GetLocalTime(&st);
			_snprintf(g_fout_path + strlen(g_fout_path),
				sizeof(g_fout_path) - strlen(g_fout_path),
				"%04d%02d%02d-%02d%02d%02d-%03d.txt",
				st.wYear, st.wMonth, st.wDay,
				st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		}
		g_fout = fopen(g_fout_path, "w");
		if (g_fout)
			fprintf(g_fout, "=== xxvm vmp38 %s session ===\n", prefix);
		return 1;   /* this call created the session file */
	}
}

static void vmp38_log_close(void)
{
	if (g_fout)
	{
		fprintf(g_fout, "=== end ===\n");
		fclose(g_fout);
		g_fout = NULL;
	}
}

static void out_file(const char *buf)
{
	if (g_fout)
		fprintf(g_fout, "%s\n", buf);
}

/* GUI log lines shown before muting: x64dbg's GUI log buffer is ~1MB and
   drops the oldest lines once full, so huge handler dumps both stall the UI
   and come out incomplete. Show the first GUI_OUT_MAX lines, then keep
   writing to the session FILE only (with a periodic one-line summary). */
#define GUI_OUT_MAX 5000
static int g_out_gui_cnt = 0;
static void out(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	out_file(buf);
	if (g_out_gui_cnt < GUI_OUT_MAX)
		_plugin_logputs(buf);
	else if ((g_out_gui_cnt % 5000) == 0)
	{
		char s2[192];
		_snprintf(s2, sizeof(s2),
			"[xxvm] GUI log muted at %d lines - details in vmp38_restore.log (still running)",
			g_out_gui_cnt);
		_plugin_logputs(s2);
	}
	g_out_gui_cnt++;
}

/* ============ ValueCommand::Calc implementation ============ */
static unsigned long long vmp38_mask(int size_bits)
{
	return (size_bits >= 64) ? ~0ULL : (size_bits == 32) ? 0xffffffffULL :
		(size_bits == 16) ? 0xffffULL : 0xffULL;
}

static unsigned long long vmp38_rol(unsigned long long v, int cnt, int bits)
{
	unsigned long long mask = vmp38_mask(bits);
	v &= mask;
	cnt %= bits;
	return ((v << cnt) | (v >> (bits - cnt))) & mask;
}

static unsigned long long vmp38_ror(unsigned long long v, int cnt, int bits)
{
	unsigned long long mask = vmp38_mask(bits);
	v &= mask;
	cnt %= bits;
	return ((v >> cnt) | (v << (bits - cnt))) & mask;
}

static unsigned long long vmp38_bswap(unsigned long long v, int bits)
{
	if (bits == 16)
		return ((v & 0xff) << 8) | ((v >> 8) & 0xff);
	if (bits == 32)
		return ((v & 0xff) << 24) | ((v & 0xff00) << 8) |
			((v >> 8) & 0xff00) | ((v >> 24) & 0xff);
	return ((v & 0xff) << 56) | ((v & 0xff00) << 40) |
		((v & 0xff0000) << 24) | ((v & 0xff000000) << 8) |
		((v >> 8) & 0xff000000) | ((v >> 24) & 0xff0000) |
		((v >> 40) & 0xff00) | ((v >> 56) & 0xff);
}

static unsigned long long vmp38_calc_one(unsigned long long value,
	const VMP_CMD *cmd, int bits)
{
	switch (cmd->type)
	{
	case VMP_CC_ADD: case VMP_CC_INC:
		return (value + cmd->val) & vmp38_mask(bits);
	case VMP_CC_SUB: case VMP_CC_DEC:
		return (value - cmd->val) & vmp38_mask(bits);
	case VMP_CC_XOR:
		return value ^ cmd->val;
	case VMP_CC_NOT:
		return (~value) & vmp38_mask(bits);
	case VMP_CC_NEG:
		return (0 - value) & vmp38_mask(bits);
	case VMP_CC_BSWAP:
		return vmp38_bswap(value, bits);
	case VMP_CC_ROL:
		return vmp38_rol(value, (int)(cmd->val & 0x3f), bits);
	case VMP_CC_ROR:
		return vmp38_ror(value, (int)(cmd->val & 0x3f), bits);
	}
	return value;
}

/* Decrypt: iterate commands in reverse, swapping add<->sub/inc<->dec/rol<->ror */
unsigned long long vmp38_value_decrypt(unsigned long long value,
	const VMP_CMD *cmds, int n)
{
	int i;
	for (i = n - 1; i >= 0; i--)
	{
		VMP_CMD c = cmds[i];
		int bits = cmds[i].size_bits ? cmds[i].size_bits : 64;
		if (c.type == VMP_CC_ADD) c.type = VMP_CC_SUB;
		else if (c.type == VMP_CC_SUB) c.type = VMP_CC_ADD;
		else if (c.type == VMP_CC_INC) c.type = VMP_CC_DEC;
		else if (c.type == VMP_CC_DEC) c.type = VMP_CC_INC;
		else if (c.type == VMP_CC_ROL) c.type = VMP_CC_ROR;
		else if (c.type == VMP_CC_ROR) c.type = VMP_CC_ROL;
		value = vmp38_calc_one(value, &c, bits);
	}
	return value;
}

/* Encrypt: forward order, original commands */
unsigned long long vmp38_value_encrypt(unsigned long long value,
	const VMP_CMD *cmds, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		int bits = cmds[i].size_bits ? cmds[i].size_bits : 64;
		value = vmp38_calc_one(value, &cmds[i], bits);
	}
	return value;
}

/* map register name -> index (rax..r15, eax.., ax.., al.., r8d..) */
static int reg_index_from_name(const char *name)
{
	if (!name) return -1;
	if (strcmp(name, "rax") == 0 || strcmp(name, "eax") == 0 ||
		strcmp(name, "ax") == 0 || strcmp(name, "al") == 0 ||
		strcmp(name, "ah") == 0) return 0;
	if (strcmp(name, "rcx") == 0 || strcmp(name, "ecx") == 0 ||
		strcmp(name, "cx") == 0 || strcmp(name, "cl") == 0 ||
		strcmp(name, "ch") == 0) return 1;
	if (strcmp(name, "rdx") == 0 || strcmp(name, "edx") == 0 ||
		strcmp(name, "dx") == 0 || strcmp(name, "dl") == 0 ||
		strcmp(name, "dh") == 0) return 2;
	if (strcmp(name, "rbx") == 0 || strcmp(name, "ebx") == 0 ||
		strcmp(name, "bx") == 0 || strcmp(name, "bl") == 0 ||
		strcmp(name, "bh") == 0) return 3;
	if (strcmp(name, "rsp") == 0 || strcmp(name, "esp") == 0 ||
		strcmp(name, "sp") == 0) return 4;
	if (strcmp(name, "rbp") == 0 || strcmp(name, "ebp") == 0 ||
		strcmp(name, "bp") == 0 || strcmp(name, "bpl") == 0) return 5;
	if (strcmp(name, "rsi") == 0 || strcmp(name, "esi") == 0 ||
		strcmp(name, "si") == 0 || strcmp(name, "sil") == 0) return 6;
	if (strcmp(name, "rdi") == 0 || strcmp(name, "edi") == 0 ||
		strcmp(name, "di") == 0 || strcmp(name, "dil") == 0) return 7;
	if (strcmp(name, "r8") == 0 || strcmp(name, "r8d") == 0 ||
		strcmp(name, "r8w") == 0 || strcmp(name, "r8b") == 0) return 8;
	if (strcmp(name, "r9") == 0 || strcmp(name, "r9d") == 0) return 9;
	if (strcmp(name, "r10") == 0 || strcmp(name, "r10d") == 0 ||
		strcmp(name, "r10w") == 0 || strcmp(name, "r10b") == 0) return 10;
	if (strcmp(name, "r11") == 0 || strcmp(name, "r11d") == 0) return 11;
	if (strcmp(name, "r12") == 0 || strcmp(name, "r12d") == 0) return 12;
	if (strcmp(name, "r13") == 0 || strcmp(name, "r13d") == 0) return 13;
	if (strcmp(name, "r14") == 0 || strcmp(name, "r14d") == 0) return 14;
	if (strcmp(name, "r15") == 0 || strcmp(name, "r15d") == 0) return 15;
	return -1;
}

/* extract mnemonic of a disasm string */
static void get_mnemonic(const char *disasm, char *out, int outsize)
{
	int j = 0;
	const char *p = disasm;
	while (*p && *p != ' ' && *p != '	' && j < outsize - 1)
		out[j++] = *p++;
	out[j] = 0;
}

/* read a number from string (hex or dec), returns 0 if not a number */
static int parse_num(const char *s, unsigned long long *out)
{
	if (!s) return 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		return sscanf(s + 2, "%llx", out) == 1;
	/* hex digits? */
	{
		const char *p = s;
		int is_hex = 0;
		while (*p)
		{
			if (!((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
				(*p >= 'A' && *p <= 'F')))
			{
				is_hex = -1;
				break;
			}
			is_hex = 1;
			p++;
		}
		if (is_hex == 1)
			return sscanf(s, "%llx", out) == 1;
	}
	return sscanf(s, "%llu", out) == 1;
}

/* ============ core data structures ============ */
#define MAX_STEP 4096

/* mini memory model: vsp-relative slots (offset -> value) */
#define MINI_MEM_SLOTS 256
typedef struct {
	int count;
	unsigned long long off[MINI_MEM_SLOTS];
	unsigned long long val[MINI_MEM_SLOTS];
	int size[MINI_MEM_SLOTS];
} MINI_MEM;

/* register file for the mini-simulator (16 GP regs) */
typedef struct {
	unsigned long long r[16];
	unsigned long long eflags;
	int flags_valid;
	int t;
	MINI_MEM mem;
	unsigned long long vsp;
} MINI_REG;

/* per-instruction record */
typedef struct {
	unsigned long long ip;
	char disasm[128];
	int instr_len;
	int push;
	int pop;
	int is_alu;
	int is_jmp;
	unsigned long long vsp_delta;
	unsigned long long real_val;
	int has_real;
	int reg_w;
	unsigned long long reg_w_val;
} STEP_REC;

/* extract the last numeric token from a disasm string (the immediate operand) */
static int parse_imm_from_disasm(const char *disasm, unsigned long long *out)
{
	const char *p = disasm;
	const char *last_num = NULL;
	int have = 0;
	while (*p)
	{
		if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
			(*p >= 'A' && *p <= 'F') || *p == 'x')
		{
			const char *s = p;
			while (s > disasm && ((s[-1] >= '0' && s[-1] <= '9') ||
				(s[-1] >= 'a' && s[-1] <= 'f') || (s[-1] >= 'A' && s[-1] <= 'F') ||
				s[-1] == 'x'))
				s--;
			last_num = s;
			have = 1;
			while (*p && ((*p >= '0' && *p <= '9') ||
				(*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F') || *p == 'x'))
				p++;
			continue;
		}
		p++;
	}
	if (have && last_num && parse_num(last_num, out))
		return 1;
	return 0;
}

/* Auto-extract a ValueCommand::Calc decrypt chain from a step sequence. */
/* check if a register name belongs to the same physical reg as carrier idx
   (e.g. r8d/r8w/r8b all belong to r8 = idx 8) */
static int same_reg_family(const char *name, int carrier_idx)
{
	int ri = reg_index_from_name(name);
	return (ri == carrier_idx);
}

/* Does this instruction's operands involve the carrier register? */
static int uses_carrier(const char *disasm, int carrier_idx)
{
	const char *p = disasm;
	/* skip mnemonic */
	while (*p && *p != ' ') p++;
	while (*p == ' ') p++;
	/* scan operands for a register in the carrier family */
	while (*p)
	{
		/* find a plausible reg token: 2-3 alpha chars or rNx */
		if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z'))
		{
			char tok[16] = "";
			int k = 0;
			while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
				(*p >= '0' && *p <= '9'))
			{
				if (k < 15) tok[k++] = *p;
				p++;
			}
			tok[k] = 0;
			if (same_reg_family(tok, carrier_idx))
				return 1;
			continue;
		}
		p++;
	}
	return 0;
}

/* Multi-carrier ValueCommand::Calc chain extraction.
   VMP 3.8 rebuilds constants across scattered registers (r11->r8->eax...),
   so we track a SET of carrier registers instead of a single one:
     - mov reg2, reg1 (reg1 in set)  -> add reg2 to set
     - ALU with any source in set    -> add to chain, add dest to set
     - ALU with dest in set          -> add to chain
     - control flow (jmp/call/ret)   -> stop
   Returns command count. */
#define MAX_CARRIERS 8
static int carrier_has(const int *carriers, int ncar, int ridx)
{
	int i;
	for (i = 0; i < ncar; i++)
		if (carriers[i] == ridx)
			return 1;
	return 0;
}
static void carrier_add(int *carriers, int *ncar, int ridx)
{
	if (ridx < 0) return;
	if (!carrier_has(carriers, *ncar, ridx) && *ncar < MAX_CARRIERS)
		carriers[(*ncar)++] = ridx;
}
static int extract_calc_chain_precise(STEP_REC *recs, int n, int start_idx,
	int carrier_idx, VMP_CMD *cmds, int maxcmds)
{
	int ncmd = 0;
	int i;
	int carriers[MAX_CARRIERS];
	int ncar = 0;
#define CARRIER_HAS(ridx)  carrier_has(carriers, ncar, (ridx))
#define CARRIER_ADD(ridx)  carrier_add(carriers, &ncar, (ridx))


	/* seed carrier set */
	if (carrier_idx >= 0 && ncar < MAX_CARRIERS)
		carriers[ncar++] = carrier_idx;

	for (i = start_idx; i < n && ncmd < maxcmds; i++)
	{
		char m[32];
		char op1[64] = "", op2[64] = "";
		unsigned long long imm = 0;
		const char *d = recs[i].disasm;
		int dst = -1, src = -1;
		int k;

		get_mnemonic(d, m, sizeof(m));

		/* parse operands */
		{
			const char *p = d;
			while (*p && *p != ' ') p++;
			while (*p == ' ') p++;
			k = 0;
			while (*p && *p != ',' && k < 63) op1[k++] = *p++;
			op1[k] = 0;
			if (*p == ',') { p++; while (*p == ' ') p++; k = 0; while (*p && k < 63) op2[k++] = *p++; op2[k] = 0; }
		}
		if (op1[0] && op1[0] != '[')
			dst = reg_index_from_name(op1);
		if (op2[0] && op2[0] != '[')
			src = reg_index_from_name(op2);


		if (strcmp(m, "mov") == 0 && !strchr(d, '['))
		{
			/* mov reg2, reg1 : propagate carrier */
			if (src >= 0 && CARRIER_HAS(src))
				CARRIER_ADD(dst);
			continue;
		}

		if (strcmp(m, "add") == 0 || strcmp(m, "sub") == 0 ||
			strcmp(m, "xor") == 0 || strcmp(m, "rol") == 0 ||
			strcmp(m, "ror") == 0)
		{
			int on_carrier = (dst >= 0 && CARRIER_HAS(dst)) || (src >= 0 && CARRIER_HAS(src));
			if (!on_carrier)
				continue;
			{
				int type = -1;
				if (strcmp(m, "add") == 0) type = VMP_CC_ADD;
				else if (strcmp(m, "sub") == 0) type = VMP_CC_SUB;
				else if (strcmp(m, "xor") == 0) type = VMP_CC_XOR;
				else if (strcmp(m, "rol") == 0) type = VMP_CC_ROL;
				else if (strcmp(m, "ror") == 0) type = VMP_CC_ROR;
				cmds[ncmd].type = type;
				cmds[ncmd].size_bits = 64;
				cmds[ncmd].val = parse_imm_from_disasm(d, &imm) ? imm : 0;
				ncmd++;
				/* result flows into dst */
				if (dst >= 0)
					CARRIER_ADD(dst);
			}
			continue;
		}

		if (strcmp(m, "lea") == 0)
		{
			/* lea rd, [rs...] : constant rebuild often uses lea (rs*scale+disp) */
			int on_carrier = 0;
			{
				const char *scan = op2;
				int ii;
				for (ii = 0; ii < ncar; ii++)
				{
					char nm[16];
					/* check if op2 references carrier reg by scanning names */
					{
						const char *pp = scan;
						while (*pp)
						{
							if ((*pp >= 'a' && *pp <= 'z') || (*pp >= 'A' && *pp <= 'Z'))
							{
								char tk[16] = "";
								int kk = 0;
								while ((*pp >= 'a' && *pp <= 'z') || (*pp >= 'A' && *pp <= 'Z') ||
									(*pp >= '0' && *pp <= '9'))
								{
									if (kk < 15) tk[kk++] = *pp;
									pp++;
								}
								tk[kk] = 0;
								if (reg_index_from_name(tk) == carriers[ii])
								{
									on_carrier = 1;
									break;
								}
								continue;
							}
							pp++;
						}
					}
					(void)nm;
					if (on_carrier) break;
				}
			}
			if (on_carrier)
			{
				/* lea reg,[rs*scale+disp] : rebuild = rs*scale + disp.
				   Model as: ADD disp (the scale multiply on the carrier is
				   approximated by preceding carrier value 鈥? exact scale
				   requires the carrier's prior value, which we don't track
				   here; record the disp as ADD and note scale if parseable). */
				unsigned long long disp = 0;
				int scale = 1;
				const char *b = strchr(op2, '[');
				const char *e = b ? strchr(b, ']') : NULL;
				if (b && e)
				{
					const char *s = b;
					while (s < e)
					{
						if (*s == '*' && s + 1 < e && s[1] >= '0' && s[1] <= '9')
						{
							scale = s[1] - '0';
							break;
						}
						s++;
					}
				}
				(void)scale;
				if (parse_imm_from_disasm(d, &disp))
				{
					/* lea dst,[carrier*N+disp] : treat as ADD disp (scale
					   folded into an implicit multiply we can't express
					   as a single VMP command; emit ADD for the disp part) */
					cmds[ncmd].type = VMP_CC_ADD;
					cmds[ncmd].size_bits = 64;
					cmds[ncmd].val = disp;
					ncmd++;
				}
				if (dst >= 0)
					CARRIER_ADD(dst);
			}
			continue;
		}

		if (strcmp(m, "not") == 0 || strcmp(m, "neg") == 0 ||
			strcmp(m, "bswap") == 0 || strcmp(m, "inc") == 0 ||
			strcmp(m, "dec") == 0)
		{
			if (!(dst >= 0 && CARRIER_HAS(dst)))
				continue;
			{
				int type = -1;
				if (strcmp(m, "not") == 0) type = VMP_CC_NOT;
				else if (strcmp(m, "neg") == 0) type = VMP_CC_NEG;
				else if (strcmp(m, "bswap") == 0) type = VMP_CC_BSWAP;
				else if (strcmp(m, "inc") == 0) type = VMP_CC_INC;
				else if (strcmp(m, "dec") == 0) type = VMP_CC_DEC;
				cmds[ncmd].type = type;
				cmds[ncmd].size_bits = 64;
				cmds[ncmd].val = (strcmp(m, "inc") == 0 || strcmp(m, "dec") == 0) ? 1 : 0;
				ncmd++;
			}
			continue;
		}

		if (ncmd > 0)
		{
			if (strcmp(m, "jmp") == 0 || strcmp(m, "call") == 0 ||
				strcmp(m, "ret") == 0)
				break;
			continue;
		}
	}
	#undef CARRIER_HAS
	#undef CARRIER_ADD
	return ncmd;
}

/* original extract_calc_chain kept for fallback (non-precise) */
static int extract_calc_chain(STEP_REC *recs, int n, int start_idx, VMP_CMD *cmds, int maxcmds)
{
	return extract_calc_chain_precise(recs, n, start_idx, -1, cmds, maxcmds);
}

/* Decrypt an encrypted immediate using a precise command chain.
   carrier_idx = register holding the encrypted value (-1 = no tracking). */
static int try_decrypt_enc_imm(unsigned long long enc_val, STEP_REC *recs, int n,
	int start_idx, int carrier_idx, unsigned long long *out)
{
	VMP_CMD cmds[32];
	int ncmd = extract_calc_chain_precise(recs, n, start_idx, carrier_idx, cmds, 32);
	if (ncmd > 0)
	{
		*out = vmp38_value_decrypt(enc_val, cmds, ncmd);
		return 1;
	}
	return 0;
}

/* mask for width */
static unsigned long long width_mask(int width)
{
	return (width == 64) ? ~0ULL : (width == 32) ? 0xffffffffULL :
		(width == 16) ? 0xffffULL : 0xffULL;
}

/* write a value to vsp-relative memory slot */
static void mini_mem_write(MINI_REG *mreg, unsigned long long vsp_off, unsigned long long val, int size)
{
	MINI_MEM *mem = &mreg->mem;
	int i;
	/* merge overlapping slot: keep existing if same offset+size */
	for (i = 0; i < mem->count; i++)
	{
		if (mem->off[i] == vsp_off && mem->size[i] == size)
		{
			mem->val[i] = val;
			return;
		}
	}
	if (mem->count < MINI_MEM_SLOTS)
	{
		mem->off[mem->count] = vsp_off;
		mem->val[mem->count] = val;
		mem->size[mem->count] = size;
		mem->count++;
	}
}

/* read a value from vsp-relative memory slot; supports overlap (different size).
   Strategy: find the slot whose range [off, off+size) best covers [vsp_off, vsp_off+size).
   For byte/halfword reads inside a larger stored slot, extract the sub-field. */
static int mini_mem_read(MINI_REG *mreg, unsigned long long vsp_off, int size, unsigned long long *out)
{
	MINI_MEM *mem = &mreg->mem;
	int i;
	int best = -1;
	unsigned long long best_cover = 0;
	/* exact match first */
	for (i = 0; i < mem->count; i++)
	{
		if (mem->off[i] == vsp_off && mem->size[i] == size)
		{
			*out = mem->val[i];
			return 1;
		}
	}
	/* containment: target fully inside a bigger slot */
	for (i = 0; i < mem->count; i++)
	{
		if (mem->off[i] <= vsp_off && vsp_off + size <= mem->off[i] + mem->size[i])
		{
			unsigned long long shift = (vsp_off - mem->off[i]) * 8;
			unsigned long long v = (mem->val[i] >> shift) & width_mask(size);
			*out = v;
			return 1;
		}
	}
	/* partial overlap: take highest coverage slot (best effort) */
	for (i = 0; i < mem->count; i++)
	{
		unsigned long long a0 = mem->off[i], a1 = mem->off[i] + mem->size[i];
		unsigned long long b0 = vsp_off, b1 = vsp_off + size;
		unsigned long long ov = 0;
		if (a0 < b1 && b0 < a1)  /* overlap */
		{
			unsigned long long s = (a0 > b0) ? a0 : b0;
			unsigned long long e = (a1 < b1) ? a1 : b1;
			ov = e - s;
		}
		if (ov > best_cover)
		{
			best_cover = ov;
			best = i;
		}
	}
	if (best >= 0)
	{
		unsigned long long shift = (vsp_off > mem->off[best])
			? (vsp_off - mem->off[best]) * 8 : 0;
		unsigned long long v = (mem->val[best] >> shift) & width_mask(size);
		*out = v;
		return 1;
	}
	return 0;
}

/* compute absolute address from an addressing expression [base + index*scale 卤 disp].
   Supports arbitrary base/index registers (rsp/r8/rbx/rdi/rax...).
   Returns 1 on success, 0 if expression has unknown parts. */
static int compute_abs_addr_ex(MINI_REG *mreg, const char *expr, unsigned long long *addr,
	unsigned long long ip, unsigned long long instr_len)
{
	const char *b = strchr(expr, '[');
	const char *e;
	char inside[160];
	if (!b) return 0;
	/* segment prefix: fs:/gs: 鈥? resolve base via x64dbg expressions.
	   fs: in user mode = TEB (thread environment block), gs: = 0 (or kernel).
	   We use teb()/peb() built-in expression functions when available. */
	{
		const char *colon = NULL;
		unsigned long long segbase = 0;
		if (strstr(expr, "fs:"))
			colon = strstr(expr, "fs:");
		else if (strstr(expr, "gs:"))
			colon = strstr(expr, "gs:");
		if (colon)
		{
			/* resolve segment base via expression (teb() for fs) */
			if (Script::Misc::ParseExpression("teb()", &segbase))
			{
				/* fs:offset -> teb + offset; extract the numeric offset */
				unsigned long long off = 0;
				const char *b2 = strchr(expr, '[');
				if (b2)
				{
					if (parse_imm_from_disasm(expr, &off))
					{
						*addr = segbase + off;
						return 1;
					}
				}
			}
		}
	}
	/* RIP-relative addressing: [rip+disp] -> ip + instr_len + disp */
	if (strncmp(b, "[rip", 4) == 0 || strncmp(b, "[RIP", 4) == 0)
	{
		unsigned long long disp = 0;
		if (parse_imm_from_disasm(expr, &disp))
		{
			*addr = ip + instr_len + disp;
			return 1;
		}
		return 0;
	}
	e = strchr(b, ']');
	if (!e) return 0;
	{
		size_t len = (size_t)(e - b - 1);
		if (len == 0 || len >= sizeof(inside)) return 0;
		memcpy(inside, b + 1, len);
		inside[len] = 0;
	}

	{
		const char *t = inside;
		char base[16] = "";
		int j = 0;
		int have_base = 0;
		unsigned long long val = 0;
		int unknown = 0;

		/* first token = base reg (skip leading whitespace) */
		while (*t == ' ') t++;
		while (*t && *t != '+' && *t != '-' && *t != '*' && j < 15)
			base[j++] = *t++;
		base[j] = 0;

		/* base may be a register, or the expr may start with index reg */
		{
			int ri = reg_index_from_name(base);
			if (ri >= 0)
			{
				val = mreg->r[ri];
				have_base = 1;
			}
			else
			{
				/* not a reg: could be pure disp like [0x1234] or [rip+..] */
				unsigned long long d;
				if (parse_num(base, &d))
				{
					val = d;
					have_base = 1;
				}
			}
		}

		/* parse remaining terms: 卤 reg[*scale] or 卤 imm */
		while (*t && !unknown)
		{
			if (*t == '+' || *t == '-')
			{
				char sign = *t;
				unsigned long long d = 0;
				const char *n = t + 1;
				while (*n == ' ') n++;
				if (parse_num(n, &d))
				{
					val += (sign == '-') ? (0 - d) : d;
					/* consume imm and rest (disp usually last) */
					break;
				}
				/* reg[ * scale] */
				{
					char rn[16] = "";
					int k = 0;
					unsigned long long scale = 1;
					while (*n && *n != '*' && *n != '+' && *n != '-' && k < 15)
						rn[k++] = *n++;
					rn[k] = 0;
					if (*n == '*')
					{
						n++;
						if (*n >= '0' && *n <= '9')
						{
							scale = *n - '0';
							n++;
						}
					}
					{
						int ii = reg_index_from_name(rn);
						if (ii >= 0)
						{
							unsigned long long add = mreg->r[ii] * scale;
							val += (sign == '-') ? (0 - add) : add;
						}
						else
							unknown = 1;
					}
				}
				t = n;
				continue;
			}
			else if (*t == '*')
			{
				/* scale applied to following reg (rare form) */
				unsigned long long scale = 1;
				t++;
				if (*t >= '0' && *t <= '9')
				{
					scale = *t - '0';
					t++;
				}
				{
					char rn[16] = "";
					int k = 0;
					while (*t && *t != '+' && *t != '-' && k < 15)
						rn[k++] = *t++;
					rn[k] = 0;
					{
						int ii = reg_index_from_name(rn);
						if (ii >= 0)
						{
							val += mreg->r[ii] * scale;
							have_base = 1;
						}
						else
							unknown = 1;
					}
				}
				continue;
			}
			t++;
		}

		if (have_base && !unknown)
		{
			*addr = val;
			return 1;
		}
	}
	return 0;
}

/* legacy wrapper (no rip support) */
static int compute_abs_addr(MINI_REG *mreg, const char *expr, unsigned long long *addr)
{
	return compute_abs_addr_ex(mreg, expr, addr, 0, 0);
}

/* extract vsp-relative offset from an addressing expression like [rsp+r10*1-0x9B02] or [r8+rdx*8-0x308]
   returns 1 if it's a simple vsp-relative form, 0 otherwise */
static int parse_vsp_offset(MINI_REG *mreg, const char *expr, unsigned long long *vsp_off)
{
	/* expr like: [rsp+imm] [r8+imm] [rsp+r10*1+imm] */
	const char *b = strchr(expr, '[');
	const char *e;
	char inside[128];
	if (!b) return 0;
	e = strchr(b, ']');
	if (!e) return 0;
	{
		size_t len = (size_t)(e - b - 1);
		if (len == 0 || len >= sizeof(inside)) return 0;
		memcpy(inside, b + 1, len);
		inside[len] = 0;
	}
	/* base must be rsp or r8 (vsp) */
	{
		const char *t = inside;
		char base[16] = "";
		int j = 0;
		while (*t && *t != '+' && *t != '-' && j < 15)
			base[j++] = *t++;
		base[j] = 0;
		if (strcmp(base, "rsp") != 0 && strcmp(base, "r8") != 0)
			return 0;
		{
			unsigned long long off = 0;
			/* remaining: [卤 reg*scale] 卤 imm */
			const char *s = t;
			while (*s)
			{
				if (*s == '+' || *s == '-')
				{
					char sign = *s;
					unsigned long long d = 0;
					const char *n = s + 1;
					while (*n == ' ') n++;
					/* try imm first */
					if (parse_num(n, &d))
					{
						off += (sign == '-') ? (0 - d) : d;
						break;
					}
					/* reg*scale */
					{
						char rn[16] = "";
						int k = 0;
						while (*n && *n != '*' && *n != '+' && *n != '-' && k < 15)
							rn[k++] = *n++;
						rn[k] = 0;
						{
							int ri = reg_index_from_name(rn);
							unsigned long long scale = 1;
							if (*n == '*')
							{
								n++;
								if (*n >= '0' && *n <= '9')
								{
									scale = *n - '0';
									n++;
								}
							}
							if (ri >= 0)
							{
								unsigned long long add = mreg->r[ri] * scale;
								off += (sign == '-') ? (0 - add) : add;
							}
						}
					}
					s = n;
					continue;
				}
				s++;
			}
			/* note: offset is relative to the *current* vsp value; normalize */
			*vsp_off = off;
			return 1;
		}
	}
}

/* get operand width from register name (rax->64, eax->32, ax->16, al->8) */
static int op_width(const char *rn)
{
	int len = (int)strlen(rn);
	/* 8-bit regs: al bl cl dl ah bh ch dh sil dil bpl spl r8b..r15b */
	if (len == 2 && rn[1] == 'l') return 8;                 /* al bl cl dl */
	if (len == 2 && rn[1] == 'h') return 8;                 /* ah bh ch dh */
	if (len == 3 && rn[0] == 's' && rn[1] == 'i' && rn[2] == 'l') return 8;
	if (len == 3 && rn[0] == 'd' && rn[1] == 'i' && rn[2] == 'l') return 8;
	if (len == 3 && rn[0] == 'b' && rn[1] == 'p' && rn[2] == 'l') return 8;
	if (len == 3 && rn[0] == 's' && rn[1] == 'p' && rn[2] == 'l') return 8;
	if (len >= 3 && rn[0] == 'r' && rn[1] >= '0' && rn[1] <= '9' && rn[len-1] == 'b') return 8;
	/* 16-bit */
	if (len == 2 && (rn[1] == 'x' || rn[1] == 'i' || rn[1] == 'p' || rn[1] == 'w')) return 16;
	if (len == 2 && rn[0] == 'b' && rn[1] == 'p') return 16;
	if (len >= 3 && rn[0] == 'r' && rn[1] >= '0' && rn[1] <= '9' && rn[len-1] == 'w') return 16;
	if (len == 2 && rn[0] == 's' && rn[1] == 'p') return 16;
	if (len == 2 && rn[0] == 'd' && rn[1] == 'i') return 16;
	if (len == 2 && rn[0] == 's' && rn[1] == 'i') return 16;
	/* 32-bit: eax ebx ecx edx esi edi ebp esp r8d..r15d */
	if (len == 3 && rn[0] == 'e') return 32;
	if (len >= 4 && rn[0] == 'r' && rn[1] >= '0' && rn[1] <= '9' && rn[len-1] == 'd') return 32;
	/* 64-bit: rax rbx rcx rdx rsi rdi rbp rsp r8..r15 */
	if (len == 3 && rn[0] == 'r' && rn[1] != '1' && rn[2] != '8') return 64;
	if (len == 3 && rn[0] == 'r' && rn[1] == '1' && rn[2] >= '0' && rn[2] <= '5') return 64;
	if (len == 2 && rn[0] == 'r' && rn[1] >= '8' && rn[1] <= '9') return 64;
	return 64;
}

/* mask for width */

/* truncate reg to operand width (zero-extend low bits) */
static unsigned long long trunc_reg(unsigned long long val, int width)
{
	return val & width_mask(width);
}

/* disassemble one instruction at ip and classify it (push/pop/alu/jmp) */
static int step_one(DISASM_INSTR *out_ins, STEP_REC *rec, unsigned long long ip)
{
	char mnem[32] = "";
	memset(rec, 0, sizeof(*rec));
	memset(out_ins, 0, sizeof(*out_ins));

	DbgDisasmAt(ip, out_ins);
	if (out_ins->instr_size < 1 || out_ins->instruction[0] == 0)
		return 0;

	rec->ip = ip;
	rec->instr_len = out_ins->instr_size;
	strncpy(rec->disasm, out_ins->instruction, sizeof(rec->disasm) - 1);

	/* classify by mnemonic */
	{
		const char *p = out_ins->instruction;
		int j = 0;
		memset(mnem, 0, sizeof(mnem));
		while (*p && *p != ' ' && *p != '	' && j < 31)
			mnem[j++] = *p++;
		mnem[j] = 0;

		if (strncmp(mnem, "push", 4) == 0)
			rec->push = 1;
		else if (strncmp(mnem, "pop", 3) == 0)
			rec->pop = 1;
		else if (strncmp(mnem, "jmp", 3) == 0 || strncmp(mnem, "call", 4) == 0 ||
			strcmp(mnem, "ret") == 0)
			rec->is_jmp = 1;
		else if (strcmp(mnem, "add") == 0 || strcmp(mnem, "sub") == 0 ||
			strcmp(mnem, "xor") == 0 || strcmp(mnem, "and") == 0 ||
			strcmp(mnem, "or") == 0 || strcmp(mnem, "not") == 0 ||
			strcmp(mnem, "neg") == 0 || strcmp(mnem, "imul") == 0 ||
			strcmp(mnem, "mul") == 0 || strcmp(mnem, "shl") == 0 ||
			strcmp(mnem, "shr") == 0 || strcmp(mnem, "sar") == 0 ||
			strcmp(mnem, "shrd") == 0 || strcmp(mnem, "shld") == 0 ||
			strcmp(mnem, "div") == 0 || strcmp(mnem, "idiv") == 0 ||
			strcmp(mnem, "rol") == 0 || strcmp(mnem, "ror") == 0 ||
			strcmp(mnem, "lea") == 0 || strcmp(mnem, "inc") == 0 ||
			strcmp(mnem, "dec") == 0 || strcmp(mnem, "xadd") == 0 ||
			strcmp(mnem, "xchg") == 0)
			rec->is_alu = 1;
	}

	/* vsp delta: push = 8, pop = 8 */
	if (rec->push)
		rec->vsp_delta = 8;
	else if (rec->pop)
		rec->vsp_delta = 8;
	else if (strcmp(mnem, "mov") == 0)
	{
		const char *d = out_ins->instruction;
		const char *br = strchr(d, '[');
		const char *comm = strchr(d, ',');
		int mem_first = 0;
		if (br && comm)
			mem_first = (br < comm) ? 1 : 0;
		if (strstr(d, "rsp") || strstr(d, "r8"))
		{
			if (mem_first)
				rec->push = 1;
			else
				rec->pop = 1;
			rec->is_alu = 1;
		}
	}
	else if (strcmp(mnem, "lea") == 0)
	{
		const char *d = out_ins->instruction;
		if (strstr(d, "rsp") || strstr(d, "r8"))
			rec->is_alu = 1;
	}

	return 1;
}

/* mini-exec: emulate common ALU/mov to track obfuscated imm values.
   Returns 1 if it wrote a register with a known value. */
static int mini_exec(const char *disasm, MINI_REG *mreg, STEP_REC *rec)
{
	char m[32];
	char op1[64] = "", op2[64] = "";
	const char *p;
	int reg_dst;
	unsigned long long imm;
	int is_imm2;

	get_mnemonic(disasm, m, sizeof(m));
	p = disasm;
	while (*p && *p != ' ') p++;
	if (*p) p++;
	/* parse op1 [, op2] */
	{
		int i = 0;
		while (*p && *p != ',' && i < 63) op1[i++] = *p++;
		op1[i] = 0;
		if (*p == ',') { p++; while (*p == ' ') p++; i = 0; while (*p && i < 63) op2[i++] = *p++; op2[i] = 0; }
	}

	rec->reg_w = -1;
	rec->has_real = 0;

	/* mov reg, imm */
	if (strcmp(m, "mov") == 0 && !strchr(op1, '[') &&
		(reg_dst = reg_index_from_name(op1)) >= 0 && parse_num(op2, &imm))
	{
		int w = op_width(op1);
		/* mov r32/r16/r8 zero-extends: clear high bits */
		mreg->r[reg_dst] = trunc_reg(imm, w);
		rec->reg_w = reg_dst;
		rec->reg_w_val = mreg->r[reg_dst];
		rec->has_real = 1;
		return 1;
	}

	/* mov reg, reg */
	if (strcmp(m, "mov") == 0 && !strchr(op1, '[') &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		int src = reg_index_from_name(op2);
		if (src >= 0)
		{
			int w = op_width(op1);
			mreg->r[reg_dst] = trunc_reg(mreg->r[src], w);
			rec->reg_w = reg_dst;
			rec->reg_w_val = mreg->r[reg_dst];
			rec->has_real = 1;
			return 1;
		}
		/* mov reg, [vsp卤imm] 鈥? read from mini memory */
		if (strchr(op2, '['))
		{
			unsigned long long vsp_off, v;
			int w = op_width(op1);
			if (parse_vsp_offset(mreg, op2, &vsp_off) && mini_mem_read(mreg, vsp_off, w, &v))
			{
				mreg->r[reg_dst] = trunc_reg(v, w);
				rec->reg_w = reg_dst;
				rec->reg_w_val = mreg->r[reg_dst];
				rec->has_real = 1;
				return 1;
			}
		}
		/* mov reg, [abs_mem] 鈥? read from real process memory */
		if (strchr(op2, '['))
		{
			unsigned long long addr = 0;
			int w = op_width(op1);
			if (compute_abs_addr_ex(mreg, op2, &addr, rec->ip, rec->instr_len))
			{
				unsigned long long v = 0;
				duint szrd = 0;
				if (Script::Memory::Read(addr, &v, w / 8, &szrd) && szrd == w / 8)
				{
					v = trunc_reg(v, w);
					mreg->r[reg_dst] = v;
					rec->reg_w = reg_dst;
					rec->reg_w_val = v;
					rec->has_real = 1;
					return 1;
				}
			}
		}
	}

	/* movzx/movsx reg, reg|mem */
	if ((strcmp(m, "movzx") == 0 || strcmp(m, "movsx") == 0) &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		int src = reg_index_from_name(op2);
		int dw = op_width(op1);
		unsigned long long v = 0;
		int have = 0;
		if (src >= 0)
		{
			/* source width from src name: r8b=8, r8w=16, r8d=32 */
			int sw = op_width(op2);
			v = trunc_reg(mreg->r[src], sw);
			have = 1;
		}
		else if (strchr(op2, '['))
		{
			/* memory source: RIP-relative + segment-aware addressing */
			unsigned long long addr = 0;
			int sw = (strstr(op2, "byte") ? 8 : strstr(op2, "word") ? 16 : 32);
			if (compute_abs_addr_ex(mreg, op2, &addr, rec->ip, rec->instr_len))
			{
				unsigned long long mv = 0;
				duint szrd2 = 0;
				if (Script::Memory::Read(addr, &mv, sw / 8, &szrd2) && szrd2 == sw / 8)
				{
					v = trunc_reg(mv, sw);
					have = 1;
				}
			}
		}
		if (have)
		{
			int sw = op_width(op2);
			if (strcmp(m, "movsx") == 0)
			{
				/* sign extend from sw */
				if (sw == 8) v = (unsigned long long)(signed char)(v & 0xff);
				else if (sw == 16) v = (unsigned long long)(short)(v & 0xffff);
				else if (sw == 32) v = (unsigned long long)(int)(v & 0xffffffff);
			}
			mreg->r[reg_dst] = trunc_reg(v, dw);
			rec->reg_w = reg_dst;
			rec->reg_w_val = mreg->r[reg_dst];
			rec->has_real = 1;
			return 1;
		}
	}

	/* push-like mov: mov [vsp卤imm], reg  -> record value to mini memory */
	if (strcmp(m, "mov") == 0 && strchr(op1, '['))
	{
		unsigned long long vsp_off;
		int src = reg_index_from_name(op2);
		int w = 64;
		if (strstr(op1, "byte")) w = 8;
		else if (strstr(op1, "word") && !strstr(op1, "qword")) w = 16;
		else if (strstr(op1, "dword")) w = 32;
		int srcw = op_width(op2);
		(void)srcw;
		if (parse_vsp_offset(mreg, op1, &vsp_off))
		{
			unsigned long long v;
			if (src >= 0)
			{
				v = trunc_reg(mreg->r[src], w);
				mini_mem_write(mreg, vsp_off, v, w);
			}
			else if (parse_num(op2, &imm))
			{
				v = trunc_reg(imm, w);
				/* note: this imm is the VMP-encrypted constant; the real value
				   is rebuilt at runtime via the command chain. ValueCommand::Calc
				   decryption (vmp38_value_decrypt) can algebraically recover it
				   once the chain is known. We record the encrypted imm here. */
				out("  const[enc] %s = 0x%llx (VMP-encrypted imm, %d bits)",
					disasm, imm, w);
				mini_mem_write(mreg, vsp_off, v, w);
			}
			else
				return 0;
			rec->real_val = v;
			rec->has_real = 1;
			return 1;
		}
	}

	/* xadd [mem], reg : exchange and add (old mem -> reg, mem += reg) */
	if (strcmp(m, "xadd") == 0 && strchr(op1, '['))
	{
		unsigned long long vsp_off, v, oldv;
		int src = reg_index_from_name(op2);
		int w = 64;
		if (strstr(op1, "byte")) w = 8;
		else if (strstr(op1, "word") && !strstr(op1, "qword")) w = 16;
		else if (strstr(op1, "dword")) w = 32;
		if (src >= 0 && parse_vsp_offset(mreg, op1, &vsp_off))
		{
			if (mini_mem_read(mreg, vsp_off, w, &oldv))
			{
				unsigned long long mask = width_mask(w);
				unsigned long long rv = mreg->r[src] & mask;
				/* reg gets old mem value; mem gets old+reg */
				mreg->r[src] = trunc_reg(oldv, w);
				v = (oldv + rv) & mask;
				mini_mem_write(mreg, vsp_off, v, w);
				rec->reg_w = src;
				rec->reg_w_val = mreg->r[src];
				rec->has_real = 1;
				return 1;
			}
		}
	}

	/* xchg [mem], reg : exchange */
	if (strcmp(m, "xchg") == 0 && strchr(op1, '['))
	{
		unsigned long long vsp_off, oldv;
		int src = reg_index_from_name(op2);
		int w = 64;
		if (strstr(op1, "byte")) w = 8;
		else if (strstr(op1, "word") && !strstr(op1, "qword")) w = 16;
		else if (strstr(op1, "dword")) w = 32;
		if (src >= 0 && parse_vsp_offset(mreg, op1, &vsp_off))
		{
			if (mini_mem_read(mreg, vsp_off, w, &oldv))
			{
				unsigned long long rv = mreg->r[src];
				mreg->r[src] = trunc_reg(oldv, w);
				mini_mem_write(mreg, vsp_off, rv, w);
				rec->reg_w = src;
				rec->reg_w_val = mreg->r[src];
				rec->has_real = 1;
				return 1;
			}
		}
	}

	/* cmpxchg [mem], reg : if (reg == al/ax/eax/rax) mem=reg else reg=mem
	   (approximate: swap like xchg when not tracking flags precisely) */
	if (strcmp(m, "cmpxchg") == 0 && strchr(op1, '['))
	{
		unsigned long long vsp_off, oldv;
		int src = reg_index_from_name(op2);
		int w = 64;
		if (strstr(op1, "byte")) w = 8;
		else if (strstr(op1, "word") && !strstr(op1, "qword")) w = 16;
		else if (strstr(op1, "dword")) w = 32;
		if (src >= 0 && parse_vsp_offset(mreg, op1, &vsp_off))
		{
			if (mini_mem_read(mreg, vsp_off, w, &oldv))
			{
				/* assume compare fails (common): reg = mem, mem unchanged */
				mreg->r[src] = trunc_reg(oldv, w);
				rec->reg_w = src;
				rec->reg_w_val = mreg->r[src];
				rec->has_real = 1;
				return 1;
			}
		}
	}

	/* add/sub/xor/and/or reg, imm|reg */
	if ((strcmp(m, "add") == 0 || strcmp(m, "sub") == 0 ||
		strcmp(m, "xor") == 0 || strcmp(m, "and") == 0 || strcmp(m, "or") == 0) &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		unsigned long long v;
		if (parse_num(op2, &v))
		{
			int w = op_width(op1);
			unsigned long long mask = width_mask(w);
			v = trunc_reg(v, w);
			{
				unsigned long long nv = mreg->r[reg_dst];
				if (strcmp(m, "add") == 0) nv = (nv + v) & mask;
				else if (strcmp(m, "sub") == 0) nv = (nv - v) & mask;
				else if (strcmp(m, "xor") == 0) nv = (nv ^ v) & mask;
				else if (strcmp(m, "and") == 0) nv = nv & v;
				else if (strcmp(m, "or") == 0) nv = nv | v;
				mreg->r[reg_dst] = (mreg->r[reg_dst] & ~mask) | nv;
			}
			rec->reg_w = reg_dst;
			rec->reg_w_val = mreg->r[reg_dst];
			rec->has_real = 1;
			return 1;
		}
		else
		{
			int src = reg_index_from_name(op2);
			if (src >= 0)
			{
				if (strcmp(m, "add") == 0) mreg->r[reg_dst] += mreg->r[src];
				else if (strcmp(m, "sub") == 0) mreg->r[reg_dst] -= mreg->r[src];
				else if (strcmp(m, "xor") == 0) mreg->r[reg_dst] ^= mreg->r[src];
				else if (strcmp(m, "and") == 0) mreg->r[reg_dst] &= mreg->r[src];
				else if (strcmp(m, "or") == 0) mreg->r[reg_dst] |= mreg->r[src];
				rec->reg_w = reg_dst;
				rec->reg_w_val = mreg->r[reg_dst];
				rec->has_real = 1;
				return 1;
			}
		}
	}

	/* not/neg reg */
	if ((strcmp(m, "not") == 0 || strcmp(m, "neg") == 0) &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		if (strcmp(m, "not") == 0) mreg->r[reg_dst] = ~mreg->r[reg_dst];
		else mreg->r[reg_dst] = 0 - mreg->r[reg_dst];
		rec->reg_w = reg_dst;
		rec->reg_w_val = mreg->r[reg_dst];
		rec->has_real = 1;
		return 1;
	}

	/* inc/dec reg */
	if ((strcmp(m, "inc") == 0 || strcmp(m, "dec") == 0) &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		if (strcmp(m, "inc") == 0) mreg->r[reg_dst]++;
		else mreg->r[reg_dst]--;
		rec->reg_w = reg_dst;
		rec->reg_w_val = mreg->r[reg_dst];
		rec->has_real = 1;
		return 1;
	}

	/* adc/sbb reg, imm|reg  (carry approximated as 0/0; treat like add/sub) */
	if ((strcmp(m, "adc") == 0 || strcmp(m, "sbb") == 0) &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		unsigned long long v;
		if (parse_num(op2, &v))
		{
			if (strcmp(m, "adc") == 0) mreg->r[reg_dst] += v;
			else mreg->r[reg_dst] -= v;
			rec->reg_w = reg_dst;
			rec->reg_w_val = mreg->r[reg_dst];
			rec->has_real = 1;
			return 1;
		}
		else
		{
			int src = reg_index_from_name(op2);
			if (src >= 0)
			{
				if (strcmp(m, "adc") == 0) mreg->r[reg_dst] += mreg->r[src];
				else mreg->r[reg_dst] -= mreg->r[src];
				rec->reg_w = reg_dst;
				rec->reg_w_val = mreg->r[reg_dst];
				rec->has_real = 1;
				return 1;
			}
		}
	}

	/* imul/mul reg, imm|reg */
	if ((strcmp(m, "imul") == 0 || strcmp(m, "mul") == 0) &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		unsigned long long v;
		if (parse_num(op2, &v))
		{
			mreg->r[reg_dst] *= v;
			rec->reg_w = reg_dst;
			rec->reg_w_val = mreg->r[reg_dst];
			rec->has_real = 1;
			return 1;
		}
		else
		{
			int src = reg_index_from_name(op2);
			if (src >= 0)
			{
				mreg->r[reg_dst] *= mreg->r[src];
				rec->reg_w = reg_dst;
				rec->reg_w_val = mreg->r[reg_dst];
				rec->has_real = 1;
				return 1;
			}
		}
	}

	/* div/idiv reg : RDX:RAX / reg -> RAX = quotient, RDX = remainder
	   (VMP 3.5 0x0a00 div d[sp+4],d[sp+8]; 32/64-bit forms). div is
	   unsigned, idiv signed. Approximate with 64-bit division; width
	   follows the register operand (op1). */
	if ((strcmp(m, "div") == 0 || strcmp(m, "idiv") == 0) &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		int width = 64;
		const char *rn = op1;
		if (strstr(rn, "d") && !strstr(rn, "dx") && strstr(rn, "d") == (rn + strlen(rn) - 1))
			width = 32;
		else if (strstr(rn, "w") && (strstr(rn, "w") == (rn + strlen(rn) - 1)))
			width = 16;
		else if (strstr(rn, "b") && (strstr(rn, "b") == (rn + strlen(rn) - 1)))
			width = 8;

		/* dividend = rdx:rax concatenated (upper in rdx, lower in rax) */
		unsigned long long divisor = mreg->r[reg_dst] & width_mask(width);
		unsigned long long dividend;
		if (width == 64)
			dividend = mreg->r[2];   /* rdx upper; rax lower already full 64 */
		else
			dividend = (mreg->r[2] << 32) | (mreg->r[0] & 0xffffffffULL);
		/* for 32-bit: only the low 32 of rdx participates */
		if (width == 32)
			dividend = ((mreg->r[2] & 0xffffffffULL) << 32) | (mreg->r[0] & 0xffffffffULL);

		if (divisor != 0)
		{
			if (strcmp(m, "idiv") == 0)
			{
				/* signed division: take signed operands */
				long long sd = (long long)(divisor & width_mask(width));
				long long sdd = (long long)dividend;
				mreg->r[0] = (unsigned long long)(sdd / sd);
				mreg->r[2] = (unsigned long long)(sdd % sd);
			}
			else
			{
				mreg->r[0] = dividend / divisor;
				mreg->r[2] = dividend % divisor;
			}
			rec->reg_w = 0;               /* rax written */
			rec->reg_w_val = mreg->r[0];
			rec->has_real = 1;
			return 1;
		}
		/* divisor == 0: leave regs untouched (would #DE) */
	}

	/* rol/ror/shl/shr/sar reg, imm|cl */
	if ((strcmp(m, "rol") == 0 || strcmp(m, "ror") == 0 ||
		strcmp(m, "shl") == 0 || strcmp(m, "shr") == 0 || strcmp(m, "sar") == 0) &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		unsigned long long cnt = 0;
		if (parse_num(op2, &cnt))
		{
			cnt &= 0x3f;
			/* width-aware rotate/shift */
			{
				int width = 64;
				const char *rn = op1;
				if (strstr(rn, "d") && !strstr(rn, "dx") && strstr(rn, "d") == (rn + strlen(rn) - 1))
					width = 32;
				else if (strstr(rn, "w") && (strstr(rn, "w") == (rn + strlen(rn) - 1)))
					width = 16;
				else if (strstr(rn, "b") && (strstr(rn, "b") == (rn + strlen(rn) - 1)))
					width = 8;

				unsigned long long mask = width_mask(width);
				unsigned long long v = mreg->r[reg_dst] & mask;
				unsigned long long nv;
				if (strcmp(m, "rol") == 0 || strcmp(m, "ror") == 0)
				{
					cnt %= width;
					if (strcmp(m, "rol") == 0)
						nv = ((v << cnt) | (v >> (width - cnt))) & mask;
					else
						nv = ((v >> cnt) | (v << (width - cnt))) & mask;
				}
				else if (strcmp(m, "shl") == 0)
					nv = (v << cnt) & mask;
				else if (strcmp(m, "shr") == 0)
					nv = v >> cnt;
				else /* sar */
				{
					/* arithmetic shift on signed width */
					if (width == 64)
						nv = (unsigned long long)((long long)v >> cnt);
					else if (width == 32)
						nv = (unsigned long long)((int)(v & 0xffffffff) >> cnt) & mask;
					else if (width == 16)
						nv = (unsigned long long)((short)(v & 0xffff) >> cnt) & mask;
					else
						nv = (unsigned long long)((signed char)(v & 0xff) >> cnt) & mask;
				}
				/* keep high bits unchanged for sub-64 ops */
				mreg->r[reg_dst] = (mreg->r[reg_dst] & ~mask) | (nv & mask);
				rec->reg_w = reg_dst;
				rec->reg_w_val = mreg->r[reg_dst];
				rec->has_real = 1;
				return 1;
			}
		}
	}

	/* shrd/shld dst, src, cnt : double-precision shift (VMP 3.5 0x0b10/0x0e10)
	   shrd: dst >>= cnt, top bits filled from src low bits
	   shld: dst <<= cnt, low bits filled from src high bits
	   3-operand form (dst, src, imm|cl). */
	if ((strcmp(m, "shrd") == 0 || strcmp(m, "shld") == 0) &&
		(reg_dst = reg_index_from_name(op1)) >= 0)
	{
		int src = reg_index_from_name(op2);
		unsigned long long cnt = 0;
		if (src >= 0)
		{
			/* count in op3 (disasm: shrd rax, rbx, imm) or cl (shrd rax, rbx, cl) */
			const char *p3 = disasm;
			const char *comma = NULL;
			int ncom = 0;
			for (p3 = disasm; *p3; p3++)
			{
				if (*p3 == ',') { ncom++; comma = p3; }
			}
			if (ncom >= 2 && comma)
			{
				const char *q = comma + 1;
				while (*q == ' ') q++;
				if (strncmp(q, "cl", 2) == 0 && (q[2] == 0 || q[2] == ' '))
					cnt = mreg->r[1] & 0x3f;   /* cl register */
				else
					parse_num(q, &cnt);
			}
			else
				cnt = 1;   /* shrd dst, cl - count from cl (approx 1) */
			if (strcmp(op2, "cl") == 0)
				cnt = mreg->r[1] & 0x3f;
			cnt &= 0x3f;
			{
				int width = 64;
				const char *rn = op1;
				if (strstr(rn, "d") && !strstr(rn, "dx") && strstr(rn, "d") == (rn + strlen(rn) - 1))
					width = 32;
				else if (strstr(rn, "w") && (strstr(rn, "w") == (rn + strlen(rn) - 1)))
					width = 16;
				else if (strstr(rn, "b") && (strstr(rn, "b") == (rn + strlen(rn) - 1)))
					width = 8;
				unsigned long long mask = width_mask(width);
				unsigned long long dst = mreg->r[reg_dst] & mask;
				unsigned long long sval = mreg->r[src] & mask;
				unsigned long long nv;
				cnt %= width;
				if (cnt == 0)
					nv = dst;
				else if (strcmp(m, "shrd") == 0)
					nv = ((dst >> cnt) | (sval << (width - cnt))) & mask;
				else
					nv = ((dst << cnt) | (sval >> (width - cnt))) & mask;
				mreg->r[reg_dst] = (mreg->r[reg_dst] & ~mask) | (nv & mask);
				rec->reg_w = reg_dst;
				rec->reg_w_val = mreg->r[reg_dst];
				rec->has_real = 1;
				return 1;
			}
		}
	}

	/* lea reg, [reg*scale 卤 imm]  鈥? key for constant rebuild (e.g. r8 = 3*r11 - const) */
	if (strcmp(m, "lea") == 0 && (reg_dst = reg_index_from_name(op1)) >= 0)
	{
		/* op2 like: [r11+r11*2-0x4CC2FDD4] or [rsp+0x28] */
		const char *b = strchr(op2, '[');
		const char *e;
		char inside[96];
		if (b)
		{
			e = strchr(b, ']');
			if (e)
			{
				size_t len = (size_t)(e - b - 1);
				if (len > 0 && len < sizeof(inside))
				{
					memcpy(inside, b + 1, len);
					inside[len] = 0;
					/* parse: base[卤 index*scale] 卤 disp, scale-aware */
					{
						char regname[32] = "";
						char scale_reg[32] = "";
						int base_scale = 1;     /* scale applied to base itself (r11*2) */
						int index_scale = 1;    /* scale of a second index reg */
						int have_base = 0;
						int have_index = 0;
						unsigned long long disp = 0;
						unsigned long long val = 0;
						const char *t = inside;
						int j = 0;

						/* first token: base reg (or index reg if it has *scale) */
						while (*t && *t != '+' && *t != '-' && *t != '*' && j < 31)
							regname[j++] = *t++;
						regname[j] = 0;

						/* if base reg directly followed by *N, it's index*scale */
						if (*t == '*' && t[1] >= '0' && t[1] <= '9')
						{
							base_scale = t[1] - '0';
							have_index = 1;
							t += 2;
							/* consume the scale reg name (same as regname usually) */
							j = 0;
							while (*t && *t != '+' && *t != '-' && j < 31)
								scale_reg[j++] = *t++;
							scale_reg[j] = 0;
						}
						else
						{
							/* plain base reg */
							have_base = 1;
						}

						/* scan rest for [+ index*scale] 卤 disp */
						while (*t)
						{
							if (*t == '+' || *t == '-')
							{
								char sign = *t;
								unsigned long long d = 0;
								const char *n = t + 1;
								while (*n == ' ') n++;
								if (parse_num(n, &d))
								{
									disp = (sign == '-') ? (0 - d) : d;
									break;
								}
								/* index reg [*scale] */
								{
									char rn[16] = "";
									int k = 0;
									int sc = 1;
									while (*n && *n != '*' && *n != '+' && *n != '-' && k < 15)
										rn[k++] = *n++;
									rn[k] = 0;
									if (*n == '*')
									{
										n++;
										if (*n >= '0' && *n <= '9')
										{
											sc = *n - '0';
											n++;
										}
									}
									{
										int ii = reg_index_from_name(rn);
										if (ii >= 0)
										{
											unsigned long long add = mreg->r[ii] * sc;
											val += (sign == '-') ? (0 - add) : add;
											have_index = 1;
										}
									}
								}
								t = n;
								continue;
							}
							t++;
						}

						/* compute: base*base_scale + index terms + disp */
						{
							int ri = reg_index_from_name(regname);
							if (ri >= 0)
							{
								unsigned long long bv = mreg->r[ri];
								if (have_index && base_scale > 1)
								{
									/* regname was actually an index*scale base */
									val += bv * (unsigned long long)base_scale;
									/* if a separate scale_reg was parsed, add it too */
									if (scale_reg[0])
									{
										int sr = reg_index_from_name(scale_reg);
										if (sr >= 0 && sr != ri)
											val += mreg->r[sr];
									}
								}
								else
								{
									val += bv;
								}
								val += disp;
								mreg->r[reg_dst] = val;
								rec->reg_w = reg_dst;
								rec->reg_w_val = val;
								rec->has_real = 1;
								return 1;
							}
						}
					}
				}
			}
		}
	}

	/* push reg -> record real value */
	if (strncmp(m, "push", 4) == 0)
	{
		int src = reg_index_from_name(op1);
		if (src >= 0)
		{
			rec->real_val = mreg->r[src];
			rec->has_real = 1;
		}
		else if (parse_num(op1, &imm))
		{
			rec->real_val = imm;
			rec->has_real = 1;
		}
		return 1;
	}

	/* string ops: movsb/movsw/movsd (with rep prefix). VMP vInit entry
	   handler does "cld; rep movsb" to copy the initial VM context onto
	   the VM stack; without executing it the simulated stack differs from
	   the real snapshot and the next dispatch pop gets garbage. */
	if (strcmp(m, "movsb") == 0 || strcmp(m, "movsw") == 0 ||
		strcmp(m, "movsd") == 0)
	{
		int esz = strcmp(m, "movsb") == 0 ? 1 : strcmp(m, "movsw") == 0 ? 2 : 4;
		unsigned long long cnt = 1;
		if (mreg->r[1] > 0 && mreg->r[1] < 0x1000)
			cnt = mreg->r[1];
		{
			int df = (mreg->eflags & 0x400) ? -1 : 1;
			unsigned long long i;
			unsigned long long esi0 = mreg->r[6];
			unsigned long long edi0 = mreg->r[7];
			int ok = 1;
			for (i = 0; i < cnt && ok; i++)
			{
				unsigned long long src = esi0 + (df > 0 ? i : cnt - 1 - i) * esz;
				unsigned long long dst = edi0 + (df > 0 ? i : cnt - 1 - i) * esz;
				unsigned char buf[8] = { 0 };
				unsigned long long szr = 0;
				if (Script::Memory::Read(src, buf, esz, &szr) && szr == (unsigned long)esz)
				{
					unsigned long long szw = 0;
					Script::Memory::Write(dst, buf, esz, &szw);
				}
				else
					ok = 0;
			}
			if (ok && cnt > 0)
			{
				mreg->r[6] = esi0 + (unsigned long long)(df > 0 ? cnt : 0 - (long long)cnt) * esz;
				mreg->r[7] = edi0 + (unsigned long long)(df > 0 ? cnt : 0 - (long long)cnt) * esz;
				mreg->r[1] = 0;
				rec->reg_w = 6;
				rec->reg_w_val = mreg->r[6];
				rec->has_real = 1;
			}
		}
		return 1;
	}

	(void)is_imm2;
	return 0;
}

/* kanxue 280283 add/sub pattern (loose variant for deep mutation):
   push;push; ... (add|sub) ... not
   In VMP 3.8, add/sub handlers push 2 operands (push or mov [vsp],reg),
   then perform ALU (add/sub/imul/xor) and finish with not/neg.
   Returns: 0=no match, 1=add, 2=sub */
static int match_kanxue_addsub(STEP_REC *recs, int n)
{
	int i;
	int push_cnt = 0, alu_add = 0, alu_sub = 0, has_not = 0, has_mul = 0;
	char alu_kind[32] = "";

	for (i = 0; i < n; i++)
	{
		char m[32];
		get_mnemonic(recs[i].disasm, m, sizeof(m));

		if (recs[i].push)
			push_cnt++;
		else if (strncmp(m, "push", 4) == 0)
			push_cnt++;

		if (strcmp(m, "add") == 0)
		{
			alu_add++;
			strcpy(alu_kind, "add");
		}
		else if (strcmp(m, "sub") == 0)
		{
			alu_sub++;
			strcpy(alu_kind, "sub");
		}
		else if (strcmp(m, "imul") == 0 || strcmp(m, "mul") == 0)
			has_mul = 1;
		else if (strcmp(m, "not") == 0 || strcmp(m, "neg") == 0)
			has_not = 1;
	}

	out("match_kanxue: push=%d add=%d sub=%d not=%d mul=%d",
		push_cnt, alu_add, alu_sub, has_not, has_mul);

	/* loose pattern: >=1 push + (add|sub) + not (mutation-tolerant) */
	if (push_cnt >= 1 && (alu_add > 0 || alu_sub > 0) && has_not)
	{
		if (alu_add > 0)
			return 1;
		if (alu_sub > 0)
			return 2;
	}
	(void)has_mul; (void)alu_kind;
	return 0;
}

/* identify handler semantics from step sequence */
static void identify_handler(STEP_REC *recs, int n, int handler_idx,
	unsigned long long vsp_in, unsigned long long vsp_out)
{
	/* Auto-decrypt VMP-encrypted immediates:
	   1. mov reg, imm(enc) 鈥? carrier = reg, precise chain
	   2. mov [mem], imm(enc) 鈥? try with -1 carrier (less precise) */
	{
		int i;
		for (i = 0; i < n; i++)
		{
			char m[32];
			unsigned long long enc = 0;
			get_mnemonic(recs[i].disasm, m, sizeof(m));
			if (strcmp(m, "mov") == 0 && !strchr(recs[i].disasm, '[') &&
				strchr(recs[i].disasm, ',') &&
				parse_imm_from_disasm(recs[i].disasm, &enc))
			{
				char op1[64] = "";
				const char *p = recs[i].disasm;
				while (*p && *p != ' ') p++;
				while (*p == ' ') p++;
				{
					int k = 0;
					while (*p && *p != ',' && k < 63) op1[k++] = *p++;
					op1[k] = 0;
				}
				/* candidate: 32-bit random-looking (0xACB33EAB) or 64-bit neg */
				{
					int is_cand = (enc > 0x40000000ULL && enc <= 0xffffffffULL) ||
						(enc > 0xffffffffULL);
					if (is_cand)
					{
						int carrier = reg_index_from_name(op1);
						unsigned long long dec = 0;
						if (carrier >= 0 && try_decrypt_enc_imm(enc, recs, n, i + 1, carrier, &dec))
						{
							out("  [decrypt] enc=0x%llx -> real=0x%llx  %s",
								enc, dec, recs[i].disasm);
						}
					}
				}
			}
			else if (strcmp(m, "mov") == 0 && strchr(recs[i].disasm, '[') &&
				strchr(recs[i].disasm, ',') &&
				parse_imm_from_disasm(recs[i].disasm, &enc))
			{
				unsigned long long dec = 0;
				if ((enc > 0x10000000ULL || enc > 0x7fffffffULL) &&
					try_decrypt_enc_imm(enc, recs, n, i + 1, -1, &dec))
				{
					out("  [decrypt] enc=0x%llx -> real=0x%llx  %s",
						enc, dec, recs[i].disasm);
				}
			}
		}
	}

	int pushes = 0, pops = 0, alu = 0, jmps = 0, i;
	char tag[64] = "";
	char note[128] = "";
	char ops[128] = "";

	for (i = 0; i < n; i++)
	{
		if (recs[i].push) pushes++;
		if (recs[i].pop)  pops++;
		if (recs[i].is_alu) alu++;
		if (recs[i].is_jmp) jmps++;
	}

	/* collect non-trivial mnemonics */
	{
		int used = 0;
		for (i = 0; i < n && used < 8; i++)
		{
			char mnem[32] = "";
			const char *p = recs[i].disasm;
			int j = 0;
			while (*p && *p != ' ' && *p != '\t' && j < 31)
				mnem[j++] = *p++;
			mnem[j] = 0;
			if (strncmp(mnem, "jmp", 3) && strncmp(mnem, "call", 4) && strcmp(mnem, "ret")
				&& strncmp(mnem, "push", 4) && strncmp(mnem, "pop", 3))
			{
				if (used)
					SCAT(ops, ",");
				SCAT(ops, mnem);
				used++;
			}
		}
	}

	/* kanxue 280283 add/sub precise pattern */
	{
		int k = match_kanxue_addsub(recs, n);
		if (k == 1)
		{
			out("handler[%d] ADD (kanxue pattern) ops=[%s] vsp %08llx->%08llx",
				handler_idx, ops, vsp_in, vsp_out);
			/* dump sequence */
			for (i = 0; i < n && i < 30; i++)
			{
				out("    [%d] @%08llx %s %s", i, recs[i].ip,
					recs[i].push ? "PUSH" : recs[i].pop ? "POP " :
					recs[i].is_alu ? "ALU " : recs[i].is_jmp ? "JMP " : "    ",
					recs[i].disasm);
			}
			return;
		}
		else if (k == 2)
		{
			out("handler[%d] SUB (kanxue pattern) ops=[%s] vsp %08llx->%08llx",
				handler_idx, ops, vsp_in, vsp_out);
			for (i = 0; i < n && i < 30; i++)
			{
				out("    [%d] @%08llx %s %s", i, recs[i].ip,
					recs[i].push ? "PUSH" : recs[i].pop ? "POP " :
					recs[i].is_alu ? "ALU " : recs[i].is_jmp ? "JMP " : "    ",
					recs[i].disasm);
			}
			return;
		}
	}

	/* ===== vCrc: table-driven checksum (VMP 3.5 0x0800 crc) =====
	   movzx reg,byte[reg]; xor; and reg,0xFF; mov reg,[reg*4+bigconst];
	   xor; inc; xor eax,key; dec; jnz = byte-loop + table lookup
	   ([reg*scale+bigconst]) + xor chain, NO stack-relative VM ops
	   (unlike real ALU handlers). */
	{
		int n_tab = 0, n_byte = 0, n_xor = 0, n_stk = 0;
		for (i = 0; i < n; i++)
		{
			char m[32];
			get_mnemonic(recs[i].disasm, m, sizeof(m));
			if (strstr(recs[i].disasm, "*4") || strstr(recs[i].disasm, "*8"))
			{
				const char *br = strchr(recs[i].disasm, '[');
				if (br && (strstr(br, "0x") || strstr(br, "0X")))
					n_tab++;
			}
			if (strstr(recs[i].disasm, "byte ptr") || strcmp(m, "movzx") == 0 ||
				strncmp(m, "set", 3) == 0)
				n_byte++;
			if (strcmp(m, "xor") == 0) n_xor++;
			if (strstr(recs[i].disasm, "[rsp") || strstr(recs[i].disasm, "[rbp") ||
				strstr(recs[i].disasm, "[esp") || strstr(recs[i].disasm, "[ebp"))
				n_stk++;
		}
		if (n_tab >= 1 && n_byte >= 1 && n_xor >= 1 && n_stk <= 1)
		{
			out("handler[%d] vCrc (tab=%d byte=%d xor=%d stk=%d) ops=[%s] vsp %08llx->%08llx",
				handler_idx, n_tab, n_byte, n_xor, n_stk, ops, vsp_in, vsp_out);
			for (i = 0; i < n && i < 30; i++)
				out("    [%d] @%08llx %s %s", i, recs[i].ip,
					recs[i].push ? "PUSH" : recs[i].pop ? "POP " :
					recs[i].is_alu ? "ALU " : recs[i].is_jmp ? "JMP " : "    ",
					recs[i].disasm);
			return;
		}
	}

	/* semantic classification (kanxue 280283) */
	if (pushes >= 2 && pops == 0 && alu == 0)
	{
		strcpy(tag, "push");
		/* show pushed operands */
		{
			char tmp[256] = "";
			int p = 0;
			for (i = 0; i < n && p < 6; i++)
			{
				if (recs[i].push)
				{
					const char *op = strchr(recs[i].disasm, ' ');
					if (op)
					{
						char ov[64];
						strncpy(ov, op + 1, sizeof(ov) - 1);
						ov[sizeof(ov) - 1] = 0;
						/* strip trailing spaces */
						{
							int l = (int)strlen(ov);
							while (l > 0 && (ov[l-1] == ' ' || ov[l-1] == '	')) ov[--l] = 0;
						}
						if (p) SCAT(tmp, ",");
						SCAT(tmp, ov);
						p++;
					}
				}
			}
			sprintf(note, " (push x%d: %s)", pushes, tmp);
		}
	}
	else if (pushes == 0 && pops >= 2)
	{
		strcpy(tag, "pop");
		sprintf(note, " (pop x%d)", pops);
	}
	else if (pushes >= 1 && alu >= 1)
	{
		strcpy(tag, "alu");
		sprintf(note, " (alu=%d push=%d pop=%d)", alu, pushes, pops);
	}
	else if (pushes == 0 && pops == 0 && alu >= 1)
	{
		strcpy(tag, "alu-only");
		sprintf(note, " (alu=%d)", alu);
	}
	else if (pushes == 0 && pops == 0 && alu == 0 && jmps == 0)
	{
		strcpy(tag, "mov/other");
	}
	else
	{
		strcpy(tag, "mixed");
		sprintf(note, " (push=%d pop=%d alu=%d jmp=%d)", pushes, pops, alu, jmps);
	}

	out("handler[%d] %s%s ops=[%s] vsp %08llx->%08llx",
		handler_idx, tag, note, ops, vsp_in, vsp_out);

	/* dump raw sequence */
	for (i = 0; i < n && i < 30; i++)
	{
		out("    [%d] %s %s%s", i,
			recs[i].push ? "PUSH" : recs[i].pop ? "POP " :
			recs[i].is_alu ? "ALU " : recs[i].is_jmp ? "JMP " : "    ",
			recs[i].disasm, recs[i].has_real ? "  <real>" : "");
	}
}

/* Auto-locate the VMP 3.8 VM entry:
   1. find the largest non-.text section (the VM section)
   2. scan .text for call/jmp instructions whose target lands in the VM section
   returns the first call target (VM entry trampoline), or 0. */
/* VM section (runtime) - set by vmp38_auto_locate, used by restore to
   reject bogus .text dispatch targets (JUNK stub loop guard) */
static unsigned long long g_vm_start = 0, g_vm_end = 0;

unsigned long long vmp38_auto_locate()
{
	Script::Module::ModuleInfo mi;
	unsigned long long vmsect_begin = 0, vmsect_end = 0;
	unsigned long long text_begin = 0, text_end = 0;
	int i;

	if (!Script::Module::GetMainModuleInfo(&mi))
		return 0;

	/* find sections: VM section = largest non-.text section */
	{
		int n = Script::Module::SectionCountFromAddr(mi.base);
		unsigned long long max_size = 0;
		for (i = 0; i < n; i++)
		{
			Script::Module::ModuleSectionInfo si;
			if (Script::Module::SectionFromAddr(mi.base, i, &si))
			{
				const char *nm = si.name;
				if (strcmp(nm, ".text") == 0)
				{
					text_begin = si.addr;
					text_end = si.addr + si.size;
				}
				else if (si.size > max_size)
				{
					max_size = si.size;
					vmsect_begin = si.addr;
					vmsect_end = si.addr + si.size;
				}
			}
		}
	}

	if (!vmsect_begin || !text_begin)
		return 0;

	g_vm_start = vmsect_begin;
	g_vm_end = vmsect_end;
	out("vmp38_auto_locate: text=[%08llx-%08llx] vmsect=[%08llx-%08llx]",
		text_begin, text_end, vmsect_begin, vmsect_end);

	/* scan .text for call/jmp into VM section */
	{
		unsigned long long a;
		for (a = text_begin; a + 5 < text_end; a++)
		{
			DISASM_INSTR ins;
			unsigned long long tgt = 0;
			memset(&ins, 0, sizeof(ins));
			DbgDisasmAt(a, &ins);
			if (ins.instr_size < 1 || ins.instruction[0] == 0)
			{
				a += 4;
				continue;
			}
			if ((strncmp(ins.instruction, "call", 4) == 0 ||
				strncmp(ins.instruction, "jmp", 3) == 0) &&
				ins.argcount > 0)
			{
				/* arg[0].constant = target for direct call/jmp */
				tgt = ins.arg[0].constant;
				if (tgt >= vmsect_begin && tgt < vmsect_end)
				{
					out("vmp38_auto_locate: found %s at %08llx -> VM %08llx",
						ins.instruction, a, tgt);
					return tgt;
				}
			}
			a += ins.instr_size - 1;
		}
	}

	/* no direct call found (VMP 3.8 uses indirect jumps): scan the VM
	   section for the first executable code block (skip junk/data) */
	{
		unsigned long long a;
		int good_run = 0;
		unsigned long long good_start = 0;
		for (a = vmsect_begin; a < vmsect_begin + 0x20000 && a < vmsect_end; )
		{
			DISASM_INSTR ins;
			char g[32];
			memset(&ins, 0, sizeof(ins));
			DbgDisasmAt(a, &ins);
			if (ins.instr_size < 1 || ins.instruction[0] == 0)
			{
				good_run = 0;
				a += 4;
				continue;
			}
			get_mnemonic(ins.instruction, g, sizeof(g));
			if (strcmp(g, "outsb") == 0 || strcmp(g, "outsd") == 0 ||
				strcmp(g, "insb") == 0 || strcmp(g, "insd") == 0 ||
				strcmp(g, "in") == 0 || strcmp(g, "out") == 0 ||
				strcmp(g, "hlt") == 0 || strcmp(g, "int3") == 0 ||
				strcmp(g, "int") == 0 || strcmp(g, "???") == 0)
			{
				good_run = 0;
				a += ins.instr_size;
				continue;
			}
			if (good_run == 0)
				good_start = a;
			good_run++;
			if (good_run >= 20)
			{
				out("vmp38_auto_locate: VM entry @ %08llx (first code block, %d insns)",
					good_start, good_run);
				return good_start;
			}
			a += ins.instr_size;
		}
	}
	out("vmp38_auto_locate: no code block found, using vmsect_begin=%08llx", vmsect_begin);
	return vmsect_begin;
	return 0;
}

/* dynamic trace freeze counter (file-scope; see vmp38_dynamic_trace) */

/* Run to VM entry and capture real registers there */
int vmp38_capture_vm_regs(unsigned long long vm_entry, VMP38_CTX *ctx)
{
	if (!ctx || !vm_entry)
		return 0;
	/* if already at VM entry, just read registers without running */
	{
		unsigned long long cur_ip = Script::Register::Get(Script::Register::RegisterEnum::RIP);
		if (cur_ip == vm_entry)
		{
			ctx->r[0] = Script::Register::Get(Script::Register::RegisterEnum::RAX);
			ctx->r[1] = Script::Register::Get(Script::Register::RegisterEnum::RCX);
			ctx->r[2] = Script::Register::Get(Script::Register::RegisterEnum::RDX);
			ctx->r[3] = Script::Register::Get(Script::Register::RegisterEnum::RBX);
			ctx->r[4] = Script::Register::Get(Script::Register::RegisterEnum::RSP);
			ctx->r[5] = Script::Register::Get(Script::Register::RegisterEnum::RBP);
			ctx->r[6] = Script::Register::Get(Script::Register::RegisterEnum::RSI);
			ctx->r[7] = Script::Register::Get(Script::Register::RegisterEnum::RDI);
			ctx->r[8] = Script::Register::Get(Script::Register::RegisterEnum::R8);
			ctx->r[9] = Script::Register::Get(Script::Register::RegisterEnum::R9);
			ctx->r[10] = Script::Register::Get(Script::Register::RegisterEnum::R10);
			ctx->r[11] = Script::Register::Get(Script::Register::RegisterEnum::R11);
			ctx->r[12] = Script::Register::Get(Script::Register::RegisterEnum::R12);
			ctx->r[13] = Script::Register::Get(Script::Register::RegisterEnum::R13);
			ctx->r[14] = Script::Register::Get(Script::Register::RegisterEnum::R14);
			ctx->r[15] = Script::Register::Get(Script::Register::RegisterEnum::R15);
			ctx->ip = cur_ip;
			out("vmp38_capture(fast): ip=%08llx rsp=%08llx rbp=%08llx r11=%08llx",
				ctx->ip, ctx->r[4], ctx->r[5], ctx->r[11]);
			return 1;
		}
	}
	out("vmp38_capture: bp at %08llx, running...", vm_entry);

	/* set temp breakpoint at VM entry */
	if (!Script::Debug::SetBreakpoint(vm_entry))
	{
		out("vmp38_capture: setbp failed");
		return 0;
	}

	/* continue execution until the breakpoint hits */
	Script::Debug::Run();
	Script::Debug::Wait();

	/* read registers (x64dbg uses x64 register names) */
	{
		/* RegisterEnum values: REG_RAX=0..REG_R15=15 (x64) */
		ctx->r[0] = Script::Register::Get(Script::Register::RegisterEnum::RAX);
		ctx->r[1] = Script::Register::Get(Script::Register::RegisterEnum::RCX);
		ctx->r[2] = Script::Register::Get(Script::Register::RegisterEnum::RDX);
		ctx->r[3] = Script::Register::Get(Script::Register::RegisterEnum::RBX);
		ctx->r[4] = Script::Register::Get(Script::Register::RegisterEnum::RSP);
		ctx->r[5] = Script::Register::Get(Script::Register::RegisterEnum::RBP);
		ctx->r[6] = Script::Register::Get(Script::Register::RegisterEnum::RSI);
		ctx->r[7] = Script::Register::Get(Script::Register::RegisterEnum::RDI);
		ctx->r[8] = Script::Register::Get(Script::Register::RegisterEnum::R8);
		ctx->r[9] = Script::Register::Get(Script::Register::RegisterEnum::R9);
		ctx->r[10] = Script::Register::Get(Script::Register::RegisterEnum::R10);
		ctx->r[11] = Script::Register::Get(Script::Register::RegisterEnum::R11);
		ctx->r[12] = Script::Register::Get(Script::Register::RegisterEnum::R12);
		ctx->r[13] = Script::Register::Get(Script::Register::RegisterEnum::R13);
		ctx->r[14] = Script::Register::Get(Script::Register::RegisterEnum::R14);
		ctx->r[15] = Script::Register::Get(Script::Register::RegisterEnum::R15);
		ctx->ip = Script::Register::Get(Script::Register::RegisterEnum::RIP);
	}
	out("vmp38_capture: ip=%08llx rsp=%08llx rbx=%08llx",
		ctx->ip, ctx->r[4], ctx->r[3]);

	/* remove temp breakpoint */
	Script::Debug::DeleteBreakpoint(vm_entry);
	return 1;
}

int vmp38_dynamic_trace(unsigned long long start_ip)
{
	int log_owned = vmp38_log_open("dyn");
	int step;
	int handler_idx = 0;
	int rec_n = 0;
	STEP_REC recs[MAX_STEP];
	unsigned long long h_start = 0, h_vsp = 0;
	/* VM-exit marker: a handler that unwinds pushad (>=2 pop regs) and
	   ends with popfd/popad is the VM exit sequence (vm_start=pushfd,
	   vm_exit=popfd, kanxue 280283). The FIRST ret after that marker is
	   the real vmret - the VM-ized code returns into .text restored code.
	   This is what whole-main VMization never detects with the
	   module-escape-only rule, and it works regardless of whether the
	   ret lands in .text (restored code) or leaves the module. */
	int vm_exit_pending = 0;
	/* vm-exit 后 ret 落到 .text 明文 stub（返回值计算）时不能当函数返回；
	   phase=1 表示已进入 .text stub 追返回值，下一个 ret 才是 vmret。 */
	int vm_exit_text_phase = 0;

	/* Termination relies on real signals only: user stop (VMP38-Stop menu),
	   EIP escaping the module (antidebug/exception), vm-exit sequence
	   (popfd+popregs then ret = vmret), ret out of module, or process end.
	   Snapshot-based dead-loop heuristics and the max_steps cap were both
	   removed (2026-08-05): VMP handlers are side-effect-free units that
	   legitimately repeat the same register state, so state-repetition
	   tests false-positive and dropped valuable later restore work; real
	   hangs are caught by module escape + user stop. */

	out("=== VMP 3.8 dynamic trace (start=%08llx) ===", start_ip);

	/* No step limit: trace until VM return, module escape, or user stop. */
	g_dyn_running = 1;
	for (step = 0; ; step++)
	{
		if (g_dyn_stop)
		{
			out("[dyn] stop requested by user - trace exits at step %d", step);
			g_dyn_running = 0;
			if (log_owned) vmp38_log_close();
			return step;
		}
		unsigned long long eip, esp, eax, ebx, ecx, edx, esi, edi, ebp;
		DISASM_INSTR ins;
		unsigned long long next;
		int is_boundary = 0;

		eip = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RIP);
		eax = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RAX);
		ecx = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RCX);
		edx = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RDX);
		ebx = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RBX);
		esp = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RSP);
		ebp = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RBP);
		esi = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RSI);
		edi = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RDI);

		/* module escape: EIP landing outside the main module means VMP's
		   exception handler detected the single-step (anti-debug) and
		   jumped into system DLLs / its handler. Stop immediately. */
		{
			Script::Module::ModuleInfo mi;
			unsigned long long modbase = 0;
			if (Script::Module::GetMainModuleInfo(&mi))
				modbase = mi.base;
			if (modbase && (eip < modbase || eip >= modbase + 0x20000000ULL))
			{
				out("  dynamic trace: EIP escaped module (%08llx) - antidebug/exception; stop", eip);
				if (log_owned) vmp38_log_close();
				return 0;
			}
		}

		memset(&ins, 0, sizeof(ins));
		DbgDisasmAt(eip, &ins);
		if (ins.instr_size < 1)
		{
			out("  [%d] disasm fail at %08llx", step, eip);
			break;
		}

		/* VALUE CAPTURE: mul/imul/div/idiv - log operand regs so magic
		   constants can be recovered black-box from the dynamic trace. */
		{
			char vm_[32];
			get_mnemonic(ins.instruction, vm_, sizeof(vm_));
			if (strcmp(vm_, "mul") == 0 || strcmp(vm_, "imul") == 0 ||
				strcmp(vm_, "div") == 0 || strcmp(vm_, "idiv") == 0)
			{
				out("[val] %s @%08llx: eax=%08x edx=%08x ecx=%08x ebx=%08x esi=%08x edi=%08x  %s",
					vm_, eip, (unsigned)eax, (unsigned)edx, (unsigned)ecx,
					(unsigned)ebx, (unsigned)esi, (unsigned)edi, ins.instruction);
			}
		}

		/* handler boundary: EIP jumped (not sequential) or jmp/ret/call */
		/* handler boundary: EIP jumped (not sequential) 鈥? detected via the
		   previous instruction being a jmp/call/ret (control transfer) */
		(void)next;
		if (rec_n > 0)
		{
			char pm[32];
			get_mnemonic(recs[rec_n-1].disasm, pm, sizeof(pm));
			if (strncmp(pm, "jmp", 3) == 0 || strncmp(pm, "call", 4) == 0 ||
				strcmp(pm, "ret") == 0)
				is_boundary = 1;
		}
		(void)next;

		if (is_boundary && rec_n > 0)
		{
			/* identify the finished handler with real register state */
			identify_handler(recs, rec_n, handler_idx, h_vsp, esp);
			/* VM-exit detection: a handler that restores the full
			   register set (popad unwound as >=2 pop regs) and pops
			   eflags (popfd/popad) is the VM exit sequence. The ret
			   that follows it is the vmret. Handler-internal
			   pushfd/popfd pairs (eflags save/restore) have no pop
			   regs and are ignored. */
			{
				int i, n_popreg = 0, n_popfd = 0;
				for (i = 0; i < rec_n; i++)
				{
					char pm[32];
					get_mnemonic(recs[i].disasm, pm, sizeof(pm));
					if (strcmp(pm, "pop") == 0 && strchr(recs[i].disasm, '[') == 0)
						n_popreg++;            /* pop reg (popad unwound) */
					else if (strcmp(pm, "popfd") == 0 || strcmp(pm, "popf") == 0 ||
						strcmp(pm, "popad") == 0)
						n_popfd++;
				}
				if (n_popfd > 0 && n_popreg >= 2)
				{
					vm_exit_pending = 1;
					out("  [vm-exit] popfd+%d popregs (handler %d) - next ret = vmret", n_popreg, handler_idx);
				}
			}
			handler_idx++;
			rec_n = 0;
		}
		if (rec_n == 0)
		{
			h_start = eip;
			h_vsp = esp;
		}

		/* record step */
		if (rec_n < MAX_STEP)
		{
			STEP_REC *r = &recs[rec_n++];
			memset(r, 0, sizeof(*r));
			r->ip = eip;
			r->instr_len = ins.instr_size;
			strncpy(r->disasm, ins.instruction, sizeof(r->disasm) - 1);
			{
				char m[32];
				get_mnemonic(r->disasm, m, sizeof(m));
				if (strncmp(m, "push", 4) == 0) r->push = 1;
				else if (strncmp(m, "pop", 3) == 0) r->pop = 1;
				else if (strncmp(m, "jmp", 3) == 0 || strncmp(m, "call", 4) == 0 ||
					strcmp(m, "ret") == 0) r->is_jmp = 1;
			}
			out("  [%d] eip=%08llx esp=%08llx ebp=%08llx %s", step, eip, esp, ebp, r->disasm);
		}

		/* ret: VM function end ONLY if next eip leaves the module.
		   Internal ret = handler dispatch (continue tracing). */
		{
			char m[32];
			get_mnemonic(recs[rec_n-1].disasm, m, sizeof(m));
			if (strcmp(m, "ret") == 0)
			{
				unsigned long long modbase = 0;
				{
					Script::Module::ModuleInfo mi;
					if (Script::Module::GetMainModuleInfo(&mi))
						modbase = mi.base;
				}
				/* peek the return address from the real stack */
				unsigned long long retaddr = 0;
				{
					unsigned long esp_now = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::RSP);
					duint szrd3 = 0;
					Script::Memory::Read(esp_now, &retaddr, 8, &szrd3);
				}
				if (vm_exit_pending)
				{
					/* VM exit sequence (popfd/popad unwind) was seen. ret
					   target inside the module = .text restored stub (return
					   value computation) -> chase it; outside = real return.
					   vm_exit_pending is one-shot - clear it so the next
					   ret is treated as internal dispatch again. */
					vm_exit_pending = 0;
					if (modbase && retaddr >= modbase &&
						retaddr < modbase + 0x20000000ULL)
					{
						vm_exit_text_phase = 1;
						if (rec_n > 0)
							identify_handler(recs, rec_n, handler_idx, h_vsp, esp);
						handler_idx++;
						out("  [ret] vm-exit -> .text restored retaddr=%08llx (chase return value)", retaddr);
					}
					else
					{
						if (rec_n > 0)
							identify_handler(recs, rec_n, handler_idx, h_vsp, esp);
						handler_idx++;
						out("  [ret] VM function return, retaddr=%08llx (vm-exit)", retaddr);
						out("=== dynamic trace done: %d handlers (VM return) ret=%08llx ===", handler_idx, retaddr);
						if (log_owned) vmp38_log_close();
						return 1;
					}
				}
				else if (vm_exit_text_phase)
				{
					/* .text restored stub's ret = real vmret */
					if (rec_n > 0)
						identify_handler(recs, rec_n, handler_idx, h_vsp, esp);
					handler_idx++;
					out("  [ret] VM function return, retaddr=%08llx (.text restored)", retaddr);
					out("=== dynamic trace done: %d handlers (VM return) ret=%08llx ===", handler_idx, retaddr);
					if (log_owned) vmp38_log_close();
					return 1;
				}
				if (retaddr == 0 || modbase == 0 ||
					retaddr < modbase || retaddr >= modbase + 0x20000000ULL)
				{
					/* leave the module -> real function return */
					if (rec_n > 0)
						identify_handler(recs, rec_n, handler_idx, h_vsp, esp);
					handler_idx++;
					out("=== dynamic trace done: %d handlers (VM return) ret=%08llx ===", handler_idx, retaddr);
					if (log_owned) vmp38_log_close();
					return 1;
				}
				/* internal ret: dispatch within module, keep tracing */
				out("  [ret] internal dispatch retaddr=%08llx (continue)", retaddr);
			}
		}

		/* Queued single-step: DbgCmdExec (queued to the command thread) is
		   the EXACT same path as GUI F7 - the command thread fully handles
		   the single-step exception between steps (F7 held down does not
		   crash, while the old DbgCmdExecDirect("sti") from the plugin
		   callback did). Headless has no command thread, so fall back to
		   DbgCmdExecDirect when the queued step never lands. */
		{
			unsigned long long eip0 = eip;
			unsigned long long cur = eip0;
			int w = 0;
			DbgCmdExec("sti");
			while (w < 500)
			{
				Sleep(20);
				w += 20;
				cur = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RIP);
				if (cur != eip0)
					break;
			}
			if (cur == eip0)
			{
				/* queued sti never landed (headless) -> direct fallback */
				DbgCmdExecDirect("sti");
				Sleep(20);
			}
		}
	}
	if (rec_n > 0)
		identify_handler(recs, rec_n, handler_idx, h_vsp,
			(unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::RSP));
	out("=== dynamic trace done: %d handlers (%d steps) ===", handler_idx, step);
	if (log_owned) vmp38_log_close();
	return 1;
}
void vmp38_restore_from_x64dbg(VMP38_CTX *in_ctx, unsigned long long bc_base, int bc_len)
{
	int log_owned = vmp38_log_open("restore");
	unsigned long long ip;
	unsigned long long vsp_in;
	int step = 0;
	int handler_idx = 0;
	int rec_n = 0;
	STEP_REC recs[MAX_STEP];

	out("=== VMP 3.8 restore (dynamic vsp tracking) ===");
	if (!in_ctx)
	{
		out("vmp38: null ctx");
		if (log_owned) vmp38_log_close();
		return;
	}
	MINI_REG mreg;
	int i;

	ip = in_ctx->ip;
	vsp_in = in_ctx->r[4];
	for (i = 0; i < 16; i++)
		mreg.r[i] = in_ctx->r[i];
	mreg.eflags = in_ctx->eflag;

	out("vmp38: ip=%08llx rsp=%08llx bc=[%08llx,%d]", ip, vsp_in, bc_base, bc_len);

	unsigned long long prev_ip = 0;   /* JUNK-loop guard */
	int same_ip_cnt = 0;
	for (step = 0; step < 50000 && ip != 0; step++)
	{
		DISASM_INSTR ins;
		STEP_REC rec;
		unsigned long long next_ip;

		/* Final guard: any same-ip loop (JUNK ret-continue) must terminate
		   quickly - a real VM never executes 50+ iterations at the same ip. */
		if (ip == prev_ip)
		{
			if (++same_ip_cnt > 50)
			{
				out("[chain-break] ip=%08llx JUNK loop detected (%d iters at same ip)",
					ip, same_ip_cnt);
				next_ip = 0;
				break;
			}
		}
		else
			same_ip_cnt = 0;
		prev_ip = ip;

		if (!step_one(&ins, &rec, ip))
		{
			out("[%d] disasm failed at ip=%08llx", step, ip);
			break;
		}
		next_ip = ip + ins.instr_size;

		/* mini-exec: track obfuscated imm real values */
		mini_exec(rec.disasm, &mreg, &rec);
			out("  [%d] r%d -> 0x%llx  %s", step, rec.reg_w == 5 ? 5 : 2,
				rec.reg_w == 5 ? mreg.r[5] : mreg.r[2], rec.disasm);
		if (rec.has_real && rec.push)
		{
			out("  [%d] push real=0x%llx  %s", step, rec.real_val, rec.disasm);
		}

		if (rec_n < MAX_STEP)
			recs[rec_n++] = rec;

		/* handler boundary: jmp/call/ret */
		if (rec.is_jmp)
		{
			if (rec_n > 0)
			{
					identify_handler(recs, rec_n, handler_idx, vsp_in, ip);
				handler_idx++;
				rec_n = 0;
				vsp_in = ip;
			}
			/* indirect dispatch: next ip unknown statically; stop after ret */
			if (strncmp(rec.disasm, "ret", 3) == 0)
				break;
			/* direct jmp: follow target if parseable */
			{
				const char *op = ins.instruction;
				const char *sp = strchr(op, ' ');
				if (sp && strncmp(rec.disasm, "jmp", 3) == 0)
				{
					unsigned long long tgt = 0;
					int tgtlen = 0;
					/* register names like "rdx" are partially valid hex
					   (sscanf("%llx") parses "ed" -> 0xed). Only accept a
					   FULL hex token (nothing left after the number). */
					if (sscanf(sp + 1, "%llx%n", &tgt, &tgtlen) == 1 &&
						sp[1 + tgtlen] == 0)
					{
						next_ip = tgt;
					}
					else
					{
						/* indirect jmp (reg/mem): resolve from mini-exec state;
						   fall back to the live register under GUI (long
						   virtualized functions: simulator reg may drift, the
						   live reg is the true dispatch target). */
						char rn[16] = "";
						int k = 0;
						const char *rp = sp + 1;
						while (*rp == ' ') rp++;
						while (*rp && ((*rp >= 'a' && *rp <= 'z') ||
							(*rp >= 'A' && *rp <= 'Z') || *rp == '0') && k < 15)
							rn[k++] = *rp++;
						rn[k] = 0;
						{
							int ri = reg_index_from_name(rn);
							unsigned long long rv = 0;
							if (ri >= 0)
								rv = mreg.r[ri];
							if (ri >= 0 && rv >= 0x100000ULL && rv < 0x80000000ULL)
							{
								next_ip = rv;
								out("  [exec] jmp %s -> %08llx (simulated reg)", rn, next_ip);
							}
							else
							{
								/* REAL-REGISTER FALLBACK (GUI): live register is
								   the true dispatch target (headless returns 0). */
								unsigned long long realv = 0;
								int re = -1;
								if (strcmp(rn, "rax") == 0 || strcmp(rn, "eax") == 0) re = 0;
								else if (strcmp(rn, "rcx") == 0 || strcmp(rn, "ecx") == 0) re = 1;
								else if (strcmp(rn, "rdx") == 0 || strcmp(rn, "edx") == 0) re = 2;
								else if (strcmp(rn, "rbx") == 0 || strcmp(rn, "ebx") == 0) re = 3;
								else if (strcmp(rn, "rsp") == 0 || strcmp(rn, "esp") == 0) re = 4;
								else if (strcmp(rn, "rbp") == 0 || strcmp(rn, "ebp") == 0) re = 5;
								else if (strcmp(rn, "rsi") == 0 || strcmp(rn, "esi") == 0) re = 6;
								else if (strcmp(rn, "rdi") == 0 || strcmp(rn, "edi") == 0) re = 7;
								else if (strncmp(rn, "r8", 2) == 0) re = 8;
								else if (strncmp(rn, "r9", 2) == 0) re = 9;
								else if (strncmp(rn, "r10", 3) == 0) re = 10;
								else if (strncmp(rn, "r11", 3) == 0) re = 11;
								else if (strncmp(rn, "r12", 3) == 0) re = 12;
								else if (strncmp(rn, "r13", 3) == 0) re = 13;
								else if (strncmp(rn, "r14", 3) == 0) re = 14;
								else if (strncmp(rn, "r15", 3) == 0) re = 15;
								if (re >= 0)
								{
									static const Script::Register::RegisterEnum renum[16] = {
										Script::Register::RegisterEnum::RAX,
										Script::Register::RegisterEnum::RCX,
										Script::Register::RegisterEnum::RDX,
										Script::Register::RegisterEnum::RBX,
										Script::Register::RegisterEnum::RSP,
										Script::Register::RegisterEnum::RBP,
										Script::Register::RegisterEnum::RSI,
										Script::Register::RegisterEnum::RDI,
										Script::Register::RegisterEnum::R8,
										Script::Register::RegisterEnum::R9,
										Script::Register::RegisterEnum::R10,
										Script::Register::RegisterEnum::R11,
										Script::Register::RegisterEnum::R12,
										Script::Register::RegisterEnum::R13,
										Script::Register::RegisterEnum::R14,
										Script::Register::RegisterEnum::R15 };
									realv = (unsigned long long)Script::Register::Get(renum[re]);
								}
								if (realv >= 0x100000ULL && realv < 0x80000000ULL)
								{
									next_ip = realv;
									out("  [exec] jmp %s -> %08llx (REAL register)", rn, next_ip);
								}
								else
								{
									/* indirect jmp (reg/mem): cannot resolve statically.
									   Terminate the loop - an unset next_ip would wander
									   into .text stubs (JUNK loop for 50000 steps). */
									next_ip = 0;
									break;
								}
							}
						}
					}
				}
			}
		}

		/* VMP handler dispatch targets must be inside the VM section.
		   A simulated target in .text is a bogus value that lands in a
		   JUNK stub loop - terminate instead of looping 50000 steps. */
		if (next_ip && g_vm_start && g_vm_end &&
			!(next_ip >= g_vm_start && next_ip < g_vm_end))
		{
			out("[chain-break] ip=%08llx next=%08llx outside VM section", ip, next_ip);
			next_ip = 0;
		}

		ip = next_ip;
		if (step % 5000 == 0)
			out("... step %d ip=%08llx", step, ip);
	}

	if (rec_n > 0)
		identify_handler(recs, rec_n, handler_idx, vsp_in, ip);

	out("=== done: %d steps, %d handlers ===", step, handler_idx);
	if (log_owned) vmp38_log_close();
}

void vmp38_set_module(const char *path, unsigned long long imgbase)
{
	out("vmp38_set_module: %s base=%08llx", path, imgbase);
}

/* ============================================================================
   VMP38 inline analyzer (C++ port of analyzer.py v3)
   Reads xxvm_restore_<ts>.data.txt -> emits pseudo C++ with vslot
   unification + constant folding. Fully embedded: no external python.
   ========================================================================== */
typedef struct
{
	char ip[32];
	char disasm[160];
	int alu, core;
	char wreg[16];
	char wval[32];
	char mem[64];
	char r0[16], r1[16], r2[16], r3[16], r4[16], r5[16], r6[16], r7[16];
	char ef[16];
	int cond;
	unsigned long long ctgt;
} AN_INSN;

typedef struct
{
	int idx;
	char type[48];
	char vsp_in[16], vsp_out[16];
	AN_INSN *insns;
	int n, cap;
	int d_n;
	char d_ip[64][16];
	unsigned long d_real[64];
} AN_HANDLER;

#define AN_M32 0xFFFFFFFFULL
#define AN_MAX_H 65536

static unsigned long an_ror32(unsigned long x, unsigned long sh)
{
	sh &= 31;
	return sh ? ((x >> sh) | (x << (32 - sh))) & AN_M32 : x;
}
static unsigned long an_rol32(unsigned long x, unsigned long sh)
{
	sh &= 31;
	return sh ? ((x << sh) | (x >> (32 - sh))) & AN_M32 : x;
}
static long long an_eval_op(const char *op, unsigned long a, unsigned long b)
{
	if (!strcmp(op, "add")) return (long long)((a + b) & AN_M32);
	if (!strcmp(op, "sub")) return (long long)((a - b) & AN_M32);
	if (!strcmp(op, "xor")) return (long long)((a ^ b) & AN_M32);
	if (!strcmp(op, "and")) return (long long)((a & b) & AN_M32);
	if (!strcmp(op, "or"))  return (long long)((a | b) & AN_M32);
	if (!strcmp(op, "shl")) return (long long)((a << (b & 31)) & AN_M32);
	if (!strcmp(op, "shr")) return (long long)((a >> (b & 31)) & AN_M32);
	if (!strcmp(op, "sar")) return (long long)(((a & AN_M32) >> (b & 31)) & AN_M32);
	if (!strcmp(op, "rol")) return (long long)an_rol32(a, b);
	if (!strcmp(op, "ror")) return (long long)an_ror32(a, b);
	if (!strcmp(op, "neg")) return (long long)((-a) & AN_M32);
	if (!strcmp(op, "not")) return (long long)((~a) & AN_M32);
	if (!strcmp(op, "inc")) return (long long)((a + 1) & AN_M32);
	if (!strcmp(op, "dec")) return (long long)((a - 1) & AN_M32);
	if (!strcmp(op, "mul") || !strcmp(op, "imul")) return (long long)((a * b) & AN_M32);
	return -1;   /* unknown: caller treats <0 as no-fold */
}

/* ---------------- VSolver (clean: reg/val maps + const table) ---------------- */
#define AN_MAX_VAR 16384
typedef struct
{
	char reg_name[64][16];        /* register key (eax,ecx,...) */
	char reg_var[64][16];         /* var name bound to that register */
	int reg_n;
	char val_hex[4096][16];       /* hex value key */
	char val_var[4096][16];       /* var name bound to that value */
	int val_n;
	char var_names[AN_MAX_VAR][16];
	char last_cmp_op1[64], last_cmp_op2[64];
	int var_inc[AN_MAX_VAR];   /* inc/dec count per var (for-loop counter) */
	unsigned long var_const[AN_MAX_VAR];   /* 0xFFFFFFFF = unknown */
	int var_n;
	int stack[AN_MAX_VAR], stack_n;
	int n;
} AN_VS;

static const char *an_new_var(AN_VS *vs)
{
	_snprintf(vs->var_names[vs->var_n], 16, "v%d", vs->n++);
	return vs->var_names[vs->var_n];
}

static int an_var_id(AN_VS *vs, const char *name)
{
	for (int i = 0; i < vs->var_n; i++)
		if (!strcmp(vs->var_names[i], name))
			return i;
	return -1;
}

/* register key -> bound var (mint if unbound) */
static const char *an_reg_canon(const char *tok);
/* mov reg2, reg1: make reg2 share reg1's variable (VMP slot carrier).
   This keeps the algorithm chain coherent (mov param -> calc reg). */
static const char *an_get_reg(AN_VS *vs, const char *r);
static const char *an_new_var(AN_VS *vs);
static const char *an_alias_reg(AN_VS *vs, const char *dst, const char *src)
{
	const char *sc = an_reg_canon(src);
	const char *dc = an_reg_canon(dst);
	const char *src_var = an_get_reg(vs, sc);
	const char *dst_var = NULL;
	for (int i = 0; i < vs->reg_n; i++)
		if (!strcmp(vs->reg_name[i], dc)) { dst_var = vs->reg_var[i]; break; }
	if (!dst_var)
	{
		/* dst not tracked yet - create and alias */
		dst_var = an_new_var(vs);
		_snprintf(vs->reg_name[vs->reg_n], 16, "%s", dc);
		_snprintf(vs->reg_var[vs->reg_n], 16, "%s", src_var);
		vs->reg_n++;
	}
	else
	{
		for (int i = 0; i < vs->reg_n; i++)
			if (!strcmp(vs->reg_name[i], dc)) { _snprintf(vs->reg_var[i], 16, "%s", src_var); break; }
	}
	return src_var;
}

static const char *an_get_reg(AN_VS *vs, const char *r)
{
	const char *rc = an_reg_canon(r);
	for (int i = 0; i < vs->reg_n; i++)
		if (!strcmp(vs->reg_name[i], rc))
			return vs->reg_var[i];
	{
		const char *v = an_new_var(vs);
		_snprintf(vs->reg_name[vs->reg_n], 16, "%s", an_reg_canon(r));
		_snprintf(vs->reg_var[vs->reg_n], 16, "%s", v);
		vs->reg_n++;
		return v;
	}
}

/* bind register to the var aliasing wval_hex (value unification), else new var */
static void an_set_const(AN_VS *vs, const char *var, unsigned long val);
static const char *an_set_reg(AN_VS *vs, const char *r, const char *wval_hex)
{
	if (wval_hex && wval_hex[0] && strcmp(wval_hex, "00000000") && strcmp(wval_hex, "0")) {
		unsigned long wv = strtoul(wval_hex, NULL, 16);
		for (int i = 0; i < vs->val_n; i++)
			if (!strcmp(vs->val_hex[i], wval_hex))
				return vs->val_var[i];
		{
			const char *v = an_new_var(vs);
			_snprintf(vs->val_hex[vs->val_n], 16, "%s", wval_hex);
			_snprintf(vs->val_var[vs->val_n], 16, "%s", v);
			vs->val_n++;
			/* value snapshot: the trace recorded the real register value
			   after this instruction - bind it as a constant so later
			   ALU ops const-fold instead of emitting '?'. */
			an_set_const(vs, v, wv);
			for (int i = 0; i < vs->reg_n; i++)
				if (!strcmp(vs->reg_name[i], r))
					_snprintf(vs->reg_var[i], 16, "%s", v);
			return v;
		}
	}
	return an_get_reg(vs, r);
}

static void an_set_const(AN_VS *vs, const char *var, unsigned long val)
{
	int id = an_var_id(vs, var);
	if (id >= 0) vs->var_const[id] = val;
}
static long an_get_const(AN_VS *vs, const char *var)
{
	int id = an_var_id(vs, var);
	if (id >= 0 && vs->var_const[id] != 0xFFFFFFFFUL)
		return (long)vs->var_const[id];
	return -1;
}
static void an_clear_const(AN_VS *vs, const char *var)
{
	int id = an_var_id(vs, var);
	if (id >= 0) vs->var_const[id] = 0xFFFFFFFFUL;
}
static void an_push_var(AN_VS *vs, int var)
{
	if (vs->stack_n < AN_MAX_VAR) vs->stack[vs->stack_n++] = var;
}
static int an_pop_var(AN_VS *vs)
{
	return vs->stack_n ? vs->stack[--vs->stack_n] : -1;
}

/* ---------------- helpers ---------------- */
static const char *AN_REGS32[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
/* canonicalize a register token to its 64-bit name so eax/rax/ax/al share
   one variable (VMP mixes operand sizes in one handler). */
static const char *an_reg_canon(const char *tok)
{
	static char buf[16];
	static const char *map32[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
	static const char *map64[8] = { "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi" };
	static const char *map16[8] = { "ax","cx","dx","bx","sp","bp","si","di" };
	static const char *map8[8]  = { "al","cl","dl","bl","ah","ch","dh","bh" };
	int i;
	_snprintf(buf, sizeof(buf), "%s", tok);
	_strlwr(buf);
	/* r8d/r8w/r8b -> r8 .. r15 */
	for (i = 8; i <= 15; i++) {
		char base[8];
		_snprintf(base, sizeof(base), "r%d", i);
		if (!strncmp(buf, base, strlen(base)))
			return base;
	}
	for (i = 0; i < 8; i++) {
		if (!strcmp(buf, map32[i]) || !strcmp(buf, map16[i]) || !strcmp(buf, map8[i]))
			return map64[i];
	}
	if (!strcmp(buf, "rsp")) return "rsp";
	if (!strcmp(buf, "rbp")) return "rbp";
	if (!strcmp(buf, "rip")) return "rip";
	return buf;
}
static const char *AN_REGS64[8] = { "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi" };
static const char *AN_REGS16[8] = { "ax","cx","dx","bx","sp","bp","si","di" };
static const char *AN_REGS8[8] = { "al","cl","dl","bl","ah","ch","dh","bh" };

static int an_is_reg(const char *tok)
{
	char t[64];
	int i;
	_snprintf(t, sizeof(t), "%s", tok);
	for (i = (int)strlen(t) - 1; i >= 0 && t[i] == ','; i--) t[i] = 0;
	_strlwr(t);
	for (i = 0; i < 8; i++)
		if (!strcmp(t, AN_REGS32[i]) || !strcmp(t, AN_REGS16[i]) || !strcmp(t, AN_REGS8[i]) ||
			!strcmp(t, AN_REGS64[i]))
			return 1;
	/* canonicalize to 64-bit name (r8d -> r8 etc.) - any recognized reg */
	if (an_reg_canon(t) != t || !strcmp(t, "r8") || !strcmp(t, "r9") ||
		!strcmp(t, "r10") || !strcmp(t, "r11") || !strcmp(t, "r12") ||
		!strcmp(t, "r13") || !strcmp(t, "r14") || !strcmp(t, "r15") ||
		!strcmp(t, "rsp") || !strcmp(t, "rbp"))
		return 1;
	return 0;
}

static int an_parse_imm(const char *tok, unsigned long *out)
{
	char t[64];
	unsigned long v;
	char *endp = NULL;
	_snprintf(t, sizeof(t), "%s", tok);
	for (int i = (int)strlen(t) - 1; i >= 0 && t[i] == ','; i--) t[i] = 0;
	/* strip leading whitespace (strtok leaves " 0x.." from ", 0x..") */
	{
		char *sp = t;
		while (*sp == ' ' || *sp == '\t') sp++;
		if (sp != t) memmove(t, sp, strlen(sp) + 1);
	}
	_strlwr(t);
	if (!strncmp(t, "0x", 2)) {
		v = strtoul(t + 2, &endp, 16);
		if (endp && *endp == 0) { *out = v; return 1; }
	}
	/* decimal */
	for (int i = 0; t[i]; i++)
		if (!isdigit((unsigned char)t[i])) return 0;
	v = strtoul(t, &endp, 10);
	if (endp && *endp == 0) { *out = v; return 1; }
	return 0;
}

static const char *an_reg_snap(AN_INSN *insn)
{
	static char buf[512];
	int i, n = 0;
	buf[0] = 0;
	const char *regs[8] = { insn->r0, insn->r1, insn->r2, insn->r3, insn->r4, insn->r5, insn->r6, insn->r7 };
	const char *(*RNS)[8] = &AN_REGS64;
	for (i = 0; i < 8; i++) {
		if (regs[i][0] && strcmp(regs[i], "00000000")) {
			_snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s=%s%s", (*RNS)[i], regs[i], n ? ", " : "");
			n++;
		}
	}
	if (insn->ef[0] && strcmp(insn->ef, "00000000"))
		_snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%sef=%s", n ? ", " : "", insn->ef);
	if (!buf[0]) return "";
	return buf;
}

/* emit_write: cur op rhs -> var with const folding. buf holds the line. */
static const char *an_op_sym(const char *op)
{
	if (!strcmp(op, "add")) return "+";
	if (!strcmp(op, "sub")) return "-";
	if (!strcmp(op, "xor")) return "^";
	if (!strcmp(op, "and")) return "&";
	if (!strcmp(op, "or")) return "|";
	if (!strcmp(op, "shl")) return "<<";
	if (!strcmp(op, "shr")) return ">>";
	if (!strcmp(op, "sar")) return ">>";
	if (!strcmp(op, "rol")) return "rol";
	if (!strcmp(op, "ror")) return "ror";
	if (!strcmp(op, "imul") || !strcmp(op, "mul")) return "*";
	if (!strcmp(op, "inc")) return "+";
	if (!strcmp(op, "dec")) return "-";
	return op;
}

static const char *an_jcc_sym(const char *mn)
{
	if (!strcmp(mn, "jb") || !strcmp(mn, "jnae") || !strcmp(mn, "jc")) return "<";
	if (!strcmp(mn, "jbe") || !strcmp(mn, "jna")) return "<=";
	if (!strcmp(mn, "ja") || !strcmp(mn, "jnbe")) return ">";
	if (!strcmp(mn, "jae") || !strcmp(mn, "jnb") || !strcmp(mn, "jnc")) return ">=";
	if (!strcmp(mn, "je") || !strcmp(mn, "jz")) return "==";
	if (!strcmp(mn, "jne") || !strcmp(mn, "jnz")) return "!=";
	if (!strcmp(mn, "jg") || !strcmp(mn, "jnle")) return ">";
	if (!strcmp(mn, "jge") || !strcmp(mn, "jnl")) return ">=";
	if (!strcmp(mn, "jl") || !strcmp(mn, "jnge")) return "<";
	if (!strcmp(mn, "jle") || !strcmp(mn, "jng")) return "<=";
	if (!strcmp(mn, "js")) return "< 0";
	if (!strcmp(mn, "jns")) return ">= 0";
	if (!strcmp(mn, "jo")) return "overflow";
	if (!strcmp(mn, "jno")) return "no-overflow";
	return mn;
}

static void an_emit_write(AN_VS *vs, const char *op, unsigned long rhs_const, const char *rhs_var,
	const char *r, const char *wval_hex, char *buf, size_t bufsz)
{
	const char *cur_var = an_get_reg(vs, r);
	long cur_val = -1, rhs_val = -1;
	unsigned long res;
	const char *var;
	if (rhs_const != 0xFFFFFFFFUL) rhs_val = (long)rhs_const;
	else if (rhs_var) rhs_val = an_get_const(vs, rhs_var);
	cur_val = an_get_const(vs, cur_var);
	/* self-op on the same variable (VMP junk: or r,r / rol r,r) - no info */
	if (rhs_var && cur_var && !strcmp(rhs_var, cur_var) &&
		(!strcmp(op, "or") || !strcmp(op, "and") || !strcmp(op, "xor") ||
		 !strcmp(op, "rol") || !strcmp(op, "ror") || !strcmp(op, "shl") ||
		 !strcmp(op, "shr") || !strcmp(op, "sar")))
	{
		buf[0] = 0;
		return;
	}
	var = an_set_reg(vs, r, wval_hex);
	if (!strcmp(op, "inc") || !strcmp(op, "dec")) {
		int vn_ = an_var_id(vs, var);
		if (vn_ >= 0) vs->var_inc[vn_]++;
	}
	if (cur_val >= 0 && rhs_val >= 0) {
		long long rr = an_eval_op(op, (unsigned long)cur_val, (unsigned long)rhs_val);
		if (rr >= 0) {
			res = (unsigned long)rr;
			an_set_const(vs, var, res);
			if (rhs_const != 0xFFFFFFFFUL)
				_snprintf(buf, bufsz, "    %s = 0x%lX;  // = 0x%lX %s 0x%lX (const-folded)", var, res, (unsigned long)cur_val, an_op_sym(op), rhs_const);
			else
				_snprintf(buf, bufsz, "    %s = 0x%lX;  // = 0x%lX %s %s (const-folded)", var, res, (unsigned long)cur_val, op, rhs_var ? rhs_var : "?");
			return;
		}
	}
	an_clear_const(vs, var);
	if (rhs_const != 0xFFFFFFFFUL)
		_snprintf(buf, bufsz, "    %s = %s %s 0x%lX;", var, cur_var, an_op_sym(op), rhs_const);
	else if (rhs_var && rhs_var[0] && strcmp(rhs_var, "?"))
	{
		long rv = an_get_const(vs, rhs_var);
		if (rv >= 0)
			_snprintf(buf, bufsz, "    %s = %s %s 0x%lX;", var, cur_var, an_op_sym(op), (unsigned long)rv);
		else
			_snprintf(buf, bufsz, "    %s = %s %s %s;", var, cur_var, an_op_sym(op), rhs_var);
	}
	else if (!strcmp(op, "not"))
		_snprintf(buf, bufsz, "    %s = ~%s;", var, cur_var);
	else if (!strcmp(op, "neg"))
		_snprintf(buf, bufsz, "    %s = -%s;", var, cur_var);
	else
		buf[0] = 0;   /* un-resolvable - emit nothing */

}

static int g_an_indent = 0;  /* loop-body indent */
/* loop structure: emit "while (loop @ctgt) {" at the loop head and
   "} // while (cond)" at the back-edge. Cond uses the last cmp operands
   (translate has run by the time the back-edge is emitted). */
typedef struct { unsigned long long ctgt, jcc_ip; char mn[16]; } AN_LOOP;

static int g_an_skip = 0;  /* loop-iteration skip depth */
static int g_an_loop_iter[64];  /* iterations seen per loop head */

static void an_insn_loop_braces(FILE *fp, AN_INSN *ii,
	AN_LOOP *loops, int loop_n, AN_VS *vs)
{
	unsigned long long iip;
	int li;
	if (!ii || !ii->ip[0]) return;
	iip = strtoull(ii->ip, NULL, 16);
	/* leave loop first (nested: ip can be outer jcc and inner ctgt) */
	for (li = 0; li < loop_n; li++)
	{
		if (iip == loops[li].jcc_ip && g_an_loop_iter[li] > 0)
		{
			int was_skip = (g_an_skip > 0);
			if (was_skip) g_an_skip--;
			if (g_an_indent > 0) g_an_indent--;
			{
				char m2[16] = "";
				_snprintf(m2, sizeof(m2), "%s", loops[li].mn);
				if (!was_skip) {
					if (vs && vs->last_cmp_op1[0])
						fprintf(fp, "%*s} // while (%s %s %s)\n", g_an_indent * 4, "",
							vs->last_cmp_op1, an_jcc_sym(m2), vs->last_cmp_op2);
					else
						fprintf(fp, "%*s} // while (%s)\n", g_an_indent * 4, "", an_jcc_sym(m2));
				} else {
					fprintf(fp, "%*s} // while (%s) (iter)\n", g_an_indent * 4, "", an_jcc_sym(m2));
				}
			}
		}
	}
	/* enter loop */
	for (li = 0; li < loop_n; li++)
	{
		if (iip == loops[li].ctgt)
		{
			if (g_an_skip > 0)
			{
				/* outer iteration being skipped: nested loop is skipped too */
				g_an_skip++;
				continue;
			}
			if (g_an_loop_iter[li] == 0)
			{
				fprintf(fp, "%*swhile (loop @0x%llx) {\n", g_an_indent * 4, "", loops[li].ctgt);
			}
			else
			{
				/* later iteration: skip the body, note it */
				g_an_skip++;
				fprintf(fp, "%*s// (loop iteration @0x%llx - body skipped)\n",
					g_an_indent * 4, "", loops[li].ctgt);
			}
			g_an_loop_iter[li]++;
			g_an_indent++;
		}
	}
}


static int an_insn_translate(AN_INSN *insn, AN_VS *vs, char *out, size_t outsz, AN_HANDLER *h, FILE *fp)
{
	char d[200], mn[32], rest[180];
	int i;
	if (!insn->disasm[0]) return 0;
	/* bind trace register snapshots as constants: the data records r0-r7
	   (real register values after this instruction). Giving the variables
	   concrete values lets ALU ops const-fold instead of emitting '?'. */
	{
		/* First-touch binding: only bind a register's trace snapshot when the
		   variable does not exist yet (VM-entry value). Later insns keep the
		   value propagated by mov-imm / ALU const-folding; per-insn overwrite
		   would use the *post*-instruction value for a pre-instruction read. */
		const char *snap_regs[8] = { insn->r0, insn->r1, insn->r2, insn->r3,
			insn->r4, insn->r5, insn->r6, insn->r7 };
		for (int ri = 0; ri < 8; ri++) {
			if (snap_regs[ri][0] && strcmp(snap_regs[ri], "00000000") &&
				strcmp(snap_regs[ri], "ffffffff")) {
				int exists = 0;
				for (int vi = 0; vi < vs->reg_n; vi++)
					if (!strcmp(vs->reg_name[vi], AN_REGS64[ri])) { exists = 1; break; }
				if (!exists) {
					unsigned long sv = strtoul(snap_regs[ri], NULL, 16);
					an_set_const(vs, an_get_reg(vs, AN_REGS64[ri]), sv);
				}
			}
		}
	}
	_snprintf(d, sizeof(d), "%s", insn->disasm);
	/* split mnemonic */
	i = 0;
	while (d[i] && !isspace((unsigned char)d[i]) && i < 31) { mn[i] = d[i]; i++; }
	mn[i] = 0;
	while (d[i] == ' ' || d[i] == '\t') i++;
	_snprintf(rest, sizeof(rest), "%s", d + i);
	_strlwr(mn);
	/* junk/filter mnemonics */
	{
		static const char *junk[] = { "call","ret","retn","retf","int3","das","nop",
			"pushf","popf","pushfd","popfd","pushad","popad","movss","movsd","movsb","movsw",
			"stos","lods","scas","leave","enter","jecxz","loop","rep","repne", NULL };
		for (i = 0; junk[i]; i++)
			if (!strcmp(mn, junk[i])) return 0;
		if (mn[0] == 'j') {
						/* control flow: conditional jump -> if-branch; jmp back -> loop */
			if (insn->ctgt) {
				unsigned long long cur = strtoull(insn->ip, NULL, 16);
				const char *csym = an_jcc_sym(mn);
				if (insn->cond && insn->ctgt < cur)
				{
					/* back-edge: braces emitted by an_insn_loop_braces */
					_snprintf(out, outsz, "    // (back-edge @0x%llx)", insn->ctgt);
				}
				else if (insn->cond)
				{
					if (vs->last_cmp_op1[0])
						_snprintf(out, outsz, "    // if (%s %s %s) goto 0x%llx", vs->last_cmp_op1, csym, vs->last_cmp_op2, insn->ctgt);
					else
						_snprintf(out, outsz, "    // if (%s) goto 0x%llx", csym, insn->ctgt);
				}
				else if (insn->ctgt < cur)
					_snprintf(out, outsz, "    // LOOP: goto 0x%llx (back-edge)", insn->ctgt);
				else
					_snprintf(out, outsz, "    // goto 0x%llx", insn->ctgt);
			} else {
				if (insn->cond)
					_snprintf(out, outsz, "    // if (%s) <branch>", an_jcc_sym(mn));
				else
					_snprintf(out, outsz, "    // goto <dispatch>");
			}
			fprintf(fp, "%*s%s\n", g_an_indent, "", out);
			return 1;
		}
		if (strchr(rest, '[')) return 0;
	}
	/* operands */
	{
		char ops[8][64];
		int on = 0;
		char *p = rest, *tok;
		while (on < 8 && (tok = strtok(p, ",")) != NULL) {
			p = NULL;
			_snprintf(ops[on++], 64, "%s", tok);
		}
		if (on == 0) return 0;
		if (!an_is_reg(ops[0])) return 0;
		{
			char r[32];
			const char *var;
			const char *wval_hex = insn->wval;
			_snprintf(r, sizeof(r), "%s", ops[0]);
			_strlwr(r);
			if (!strcmp(mn, "mov") && on >= 2) {
				unsigned long imm;
				if (an_parse_imm(ops[1], &imm)) {
					/* decrypted const lookup by ip */
					unsigned long real = 0xFFFFFFFFUL;
					for (int k = 0; k < h->d_n; k++)
						if (!strcmp(h->d_ip[k], insn->ip)) { real = h->d_real[k]; break; }
					var = an_set_reg(vs, r, wval_hex);
					if (real != 0xFFFFFFFFUL) {
						an_set_const(vs, var, real);
						_snprintf(out, outsz, "    %s = 0x%lX;  // decrypted from 0x%lX", var, real, imm);
						fprintf(fp, "%*s%s\n", g_an_indent, "", out);
						return 1;
					}
					an_set_const(vs, var, imm);
					_snprintf(out, outsz, "    %s = 0x%lX;", var, imm);
					fprintf(fp, "%*s%s\n", g_an_indent, "", out);
					return 1;
				}
				if (an_is_reg(ops[1])) {
					const char *src = an_get_reg(vs, ops[1]);
					long sc = an_get_const(vs, src);
					var = an_alias_reg(vs, r, ops[1]);
					if (sc >= 0) {
						an_set_const(vs, var, (unsigned long)sc);
						_snprintf(out, outsz, "    %s = 0x%lX;  // mov %s (const %lx)", var, (unsigned long)sc, src, (unsigned long)sc);
						fprintf(fp, "%*s%s\n", g_an_indent, "", out);
						return 1;
					}
					/* slot alias: same variable propagates the value chain */
					return 1;
				}
				return 0;
			}
			if (!strcmp(mn, "push") && on >= 1) {
				if (an_is_reg(ops[0])) {
					const char *src = an_get_reg(vs, ops[0]);
					int id = an_var_id(vs, src);
					an_push_var(vs, id);
					/* value flow: only show pushes with a known value */
					if (an_get_const(vs, src) < 0)
						return 1;
					_snprintf(out, outsz, "    push(%s);", src);
					fprintf(fp, "%*s%s\n", g_an_indent, "", out);
					return 1;
				}
				unsigned long imm;
				if (an_parse_imm(ops[0], &imm)) {
					_snprintf(out, outsz, "    push(0x%lX);", imm);
					fprintf(fp, "%*s%s\n", g_an_indent, "", out);
					return 1;
				}
				return 0;
			}
			if (!strcmp(mn, "pop") && on >= 1 && an_is_reg(ops[0])) {
				int slotv = an_pop_var(vs);
				var = an_set_reg(vs, r, wval_hex);
				an_clear_const(vs, var);
				if (slotv >= 0 && slotv < vs->var_n) {
					long sc = an_get_const(vs, vs->var_names[slotv]);
					if (sc >= 0) {
						an_set_const(vs, var, (unsigned long)sc);
						_snprintf(out, outsz, "    %s = 0x%lX;  // pop (slot const %lx)", var, (unsigned long)sc, (unsigned long)sc);
						fprintf(fp, "%*s%s\n", g_an_indent, "", out);
						return 1;
					}
				}
				/* value flow: slot unknown - skip display (state kept) */
				return 1;
			}
			if ((!strcmp(mn, "inc") || !strcmp(mn, "dec") || !strcmp(mn, "neg") || !strcmp(mn, "not")) && on >= 1 && an_is_reg(ops[0])) {
				if (!strcmp(r, "esp") || !strcmp(r, "sp")) return 0;
				if (!strcmp(mn, "inc")) an_emit_write(vs, "inc", 1, NULL, r, wval_hex, out, outsz);
				else if (!strcmp(mn, "dec")) an_emit_write(vs, "dec", 1, NULL, r, wval_hex, out, outsz);
				else if (!strcmp(mn, "not")) an_emit_write(vs, "not", 0xFFFFFFFFUL, NULL, r, wval_hex, out, outsz);
				else an_emit_write(vs, "neg", 0xFFFFFFFFUL, NULL, r, wval_hex, out, outsz);
				if (!out[0]) return 0;
				fprintf(fp, "%*s%s\n", g_an_indent, "", out);
				return 1;
			}
			{
				static const char *binops[] = { "add","sub","xor","and","or","shl","shr","sar","rol","ror","imul","mul", NULL };
				for (i = 0; binops[i]; i++) {
					if (!strcmp(mn, binops[i]) && on >= 2) {
						if (an_is_reg(ops[1])) {
							if (!strcmp(r, "esp") || !strcmp(r, "sp")) return 0;
							{
								const char *src = an_get_reg(vs, ops[1]);
								const char *cur = an_get_reg(vs, r);
								if (!strcmp(src, "esp") || !strcmp(src, "sp")) return 0;
								(void)cur;
								an_emit_write(vs, mn, 0xFFFFFFFFUL, src, r, wval_hex, out, outsz);
								if (!out[0]) return 0;
								fprintf(fp, "%*s%s\n", g_an_indent, "", out);
								return 1;
							}
						}
						unsigned long imm;
						if (an_parse_imm(ops[1], &imm)) {
							if (imm == 0 && (!strcmp(mn, "or") || !strcmp(mn, "and") || !strcmp(mn, "add") ||
								!strcmp(mn, "xor") || !strcmp(mn, "sub") || !strcmp(mn, "shl") ||
								!strcmp(mn, "shr") || !strcmp(mn, "sar")))
								return 0;
							an_emit_write(vs, mn, imm, NULL, r, wval_hex, out, outsz);
							if (!out[0]) return 0;
							fprintf(fp, "%*s%s\n", g_an_indent, "", out);
							return 1;
						}
						return 0;
					}
				}
			}
			if (!strcmp(mn, "cmp") && on >= 2) {
				if (an_is_reg(ops[0])) {
					const char *src = an_get_reg(vs, ops[0]);
					_snprintf(out, outsz, "    // cmp %s, %s (eflags)", src, ops[1]);
					fprintf(fp, "%*s%s\n", g_an_indent, "", out);
					_snprintf(vs->last_cmp_op1, sizeof(vs->last_cmp_op1), "%s", src);
					_snprintf(vs->last_cmp_op2, sizeof(vs->last_cmp_op2), "%s", ops[1]);
					return 1;
				}
				_snprintf(out, outsz, "    // cmp %s, %s (eflags)", ops[0], ops[1]);
				fprintf(fp, "%*s%s\n", g_an_indent, "", out);
				_snprintf(vs->last_cmp_op1, sizeof(vs->last_cmp_op1), "%s", ops[0]);
				_snprintf(vs->last_cmp_op2, sizeof(vs->last_cmp_op2), "%s", ops[1]);
				return 1;
			}
		}
	}
	return 0;
}

/* ---------------- main entry ---------------- */
void vmp38_analyze_inline(const char *data_path, const char *out_path)
{
	FILE *fin = fopen(data_path, "r");
	FILE *fout = fopen(out_path, "w");
	AN_HANDLER *handlers = NULL;
	int hn = 0, hcap = 0;
	AN_HANDLER *cur = NULL;
	char line[2048];
	AN_VS vs;
	FILE *fp = fout;
	/* param/result tracking */
	char *all_lines = NULL;
	size_t all_len = 0, all_cap = 0;

	if (!fin || !fout) {
		if (fin) fclose(fin);
		if (fout) fclose(fout);
		return;
	}
	memset(&vs, 0, sizeof(vs));
	for (int i = 0; i < 8192; i++) vs.var_const[i] = 0xFFFFFFFFUL;
	for (int i = 0; i < AN_MAX_VAR; i++) vs.var_inc[i] = 0;

	/* ---- parse ---- */
	while (fgets(line, sizeof(line), fin)) {
		if (line[0] == 'H') {
			int idx, n;
			char v1[16], v2[16];
			if (sscanf(line, "H idx=%d n=%d vsp_in=%15s vsp_out=%15s", &idx, &n, v1, v2) == 4) {
				if (hn >= hcap) {
					hcap = hcap ? hcap * 2 : 256;
					handlers = (AN_HANDLER*)realloc(handlers, hcap * sizeof(AN_HANDLER));
				}
				memset(&handlers[hn], 0, sizeof(AN_HANDLER));
				handlers[hn].idx = idx;
				_snprintf(handlers[hn].vsp_in, sizeof(handlers[hn].vsp_in), "%s", v1);
				_snprintf(handlers[hn].vsp_out, sizeof(handlers[hn].vsp_out), "%s", v2);
				cur = &handlers[hn];
				hn++;
			}
		}
		else if (line[0] == 'T' && cur) {
			char tp[48];
			if (sscanf(line, "T type=%47s", tp) == 1)
				_snprintf(cur->type, sizeof(cur->type), "%s", tp);
		}
		else if (line[0] == 'I' && cur) {
			AN_INSN *ii;
			char *di = strstr(line, "disasm=\"");
			char *fields_end = di ? di : line + strlen(line);
			/* parse key=val fields up to disasm= */
			if (cur->n >= cur->cap) {
				cur->cap = cur->cap ? cur->cap * 2 : 64;
				cur->insns = (AN_INSN*)realloc(cur->insns, cur->cap * sizeof(AN_INSN));
			}
			ii = &cur->insns[cur->n];
			memset(ii, 0, sizeof(*ii));
			{
				/* manual token parse (sscanf %31s=%63s loop proved unreliable
				   for the 0x-hex ctgt field on some toolchains) */
				const char *tp = line;
				while (tp && *tp) {
					const char *sp = strchr(tp, ' ');
					size_t tl = sp ? (size_t)(sp - tp) : strlen(tp);
					if (tl > 1) {
						char tok[96];
						char *eq;
						if (tl >= sizeof(tok)) tl = sizeof(tok) - 1;
						memcpy(tok, tp, tl);
						tok[tl] = 0;
						eq = strchr(tok, '=');
						if (eq) {
							*eq = 0;
							const char *k = tok;
							const char *v = eq + 1;
							if (!strcmp(k, "ip")) _snprintf(ii->ip, sizeof(ii->ip), "%s", v);
							else if (!strcmp(k, "alu")) ii->alu = (v[0] == '1');
							else if (!strcmp(k, "core")) ii->core = (v[0] == '1');
							else if (!strcmp(k, "wreg")) _snprintf(ii->wreg, sizeof(ii->wreg), "%s", v);
							else if (!strcmp(k, "wval")) _snprintf(ii->wval, sizeof(ii->wval), "%s", v);
							else if (!strcmp(k, "mem")) _snprintf(ii->mem, sizeof(ii->mem), "%s", v);
							else if (!strcmp(k, "r0")) _snprintf(ii->r0, sizeof(ii->r0), "%s", v);
							else if (!strcmp(k, "r1")) _snprintf(ii->r1, sizeof(ii->r1), "%s", v);
							else if (!strcmp(k, "r2")) _snprintf(ii->r2, sizeof(ii->r2), "%s", v);
							else if (!strcmp(k, "r3")) _snprintf(ii->r3, sizeof(ii->r3), "%s", v);
							else if (!strcmp(k, "r4")) _snprintf(ii->r4, sizeof(ii->r4), "%s", v);
							else if (!strcmp(k, "r5")) _snprintf(ii->r5, sizeof(ii->r5), "%s", v);
							else if (!strcmp(k, "r6")) _snprintf(ii->r6, sizeof(ii->r6), "%s", v);
							else if (!strcmp(k, "r7")) _snprintf(ii->r7, sizeof(ii->r7), "%s", v);
							else if (!strcmp(k, "ef")) _snprintf(ii->ef, sizeof(ii->ef), "%s", v);
							else if (!strcmp(k, "cond")) ii->cond = (v[0] == '1');
							else if (!strcmp(k, "ctgt")) ii->ctgt = strtoull(v, NULL, 16);
						}
					}
					if (!sp) break;
					tp = sp + 1;
				}
			}
			if (di) {
				char *q = di + 8;
				char *end = strchr(q, '"');
				if (end) {
					size_t l = (size_t)(end - q);
					if (l > sizeof(ii->disasm) - 1) l = sizeof(ii->disasm) - 1;
					memcpy(ii->disasm, q, l);
					ii->disasm[l] = 0;
				}
			}
			cur->n++;
		}
		else if (line[0] == 'D' && cur) {
			char dip[16];
			unsigned long denc, dreal;
			if (sscanf(line, "D ip=%15s enc=%lx real=%lx", dip, &denc, &dreal) == 3 && cur->d_n < 64) {
				_snprintf(cur->d_ip[cur->d_n], 16, "%s", dip);
				cur->d_real[cur->d_n] = dreal;
				cur->d_n++;
			}
		}
	}
	fclose(fin);

	/* ---- emit ---- */
	fprintf(fp, "// VMP 3.8 restore - pseudo C++ v3 (vslot + constant folding)\n");
	fprintf(fp, "// vN = virtual regs; // (const-folded) shows recovered constants\n\n");

	/* dec_consts: ip -> real */
	{
		int nhandlers = hn;
		/* param->chain->result tracking */
		char param_line_ok = 0;
		char param_reg[16] = "";
		char param_mem[64] = "";
		char param_ip[16] = "";
		for (int hh = 0; hh < nhandlers && !param_line_ok; hh++) {
			for (int k = 0; k < handlers[hh].n; k++) {
				AN_INSN *ii = &handlers[hh].insns[k];
				if (strstr(ii->disasm, "[esp+0x18]") || strstr(ii->disasm, "[esp+0x1C]")) {
					if (strstr(ii->disasm, "mov ") && ii->mem[0]) {
						_snprintf(param_reg, sizeof(param_reg), "%s", ii->disasm + 4);
						param_reg[3] = 0;   /* eax/ecx/... */
						_snprintf(param_mem, sizeof(param_mem), "%s", ii->mem);
						_snprintf(param_ip, sizeof(param_ip), "%s", ii->ip);
						param_line_ok = 1;
						break;
					}
				}
			}
		}
		if (param_line_ok) {
			char memval[32] = "?";
			char memaddr[32] = "?";
			char *at = strchr(param_mem, '@');
			if (at) {
				*at = 0;
				_snprintf(memval, sizeof(memval), "%s", param_mem);
				_snprintf(memaddr, sizeof(memaddr), "%s", at + 1);
			}
			fprintf(fp, "// PARAM: %s = 0x%s  (from [esp+0x18] @0x%s)\n", param_reg, memval, param_ip);
			fprintf(fp, "//         mem addr 0x%s\n", memaddr);
			/* RESULT: last non-zero r0 across all insns */
			{
				char last_r0[16] = "";
				for (int hh = nhandlers - 1; hh >= 0 && !last_r0[0]; hh--)
					for (int k = handlers[hh].n - 1; k >= 0; k--)
						if (handlers[hh].insns[k].r0[0] && strcmp(handlers[hh].insns[k].r0, "00000000")) {
							_snprintf(last_r0, sizeof(last_r0), "%s", handlers[hh].insns[k].r0);
							break;
						}
				if (last_r0[0])
					fprintf(fp, "// RESULT: eax = 0x%s  (last non-zero snapshot)\n", last_r0);
			}
			fprintf(fp, "\n");
		}
		/* handlers */
		/* loop pre-scan: collect back-edge (cond && ctgt < current ip) */
		AN_LOOP loops[64];
		memset(loops, 0, sizeof(loops));
		int loop_n = 0;
		for (int hh = 0; hh < nhandlers && loop_n < 64; hh++) {
			AN_HANDLER *h0 = &handlers[hh];
			for (int k0 = 0; k0 < h0->n; k0++) {
				AN_INSN *ii = &h0->insns[k0];
				if (ii->cond && ii->ctgt && ii->ip[0]) {
					unsigned long long iip = strtoull(ii->ip, NULL, 16);
					if (ii->ctgt < iip) {
						int dup = 0;
						for (int dl = 0; dl < loop_n; dl++)
							if (loops[dl].ctgt == ii->ctgt) { dup = 1; break; }
						if (!dup) {
							char m0[32] = "";
							{ const char *dp = ii->disasm; int kk = 0; while (dp[kk] && !isspace((unsigned char)dp[kk]) && kk < 31) { m0[kk] = dp[kk]; kk++; } m0[kk] = 0; }
							_snprintf(loops[loop_n].mn, sizeof(loops[loop_n].mn), "%s", m0);
							loops[loop_n].ctgt = ii->ctgt;
							loops[loop_n].jcc_ip = iip;
							loop_n++;
						}
					}
				}
			}
		}
		g_an_indent = 0;
		g_an_skip = 0;
		for (int li = 0; li < 64; li++) g_an_loop_iter[li] = 0;
		int prev_cmp = -1;
		for (int hh = 0; hh < nhandlers; hh++) {
			AN_HANDLER *h = &handlers[hh];
			const char *t = h->type[0] ? h->type : "?";
			/* fold types: vDispatch/vInit/vJmp/vRet/vCall */
			if (!strcmp(t, "vDispatch") || !strcmp(t, "vInit") || !strcmp(t, "vJmp") || !strcmp(t, "vRet") || !strcmp(t, "vCall")) {
				prev_cmp = -1;
				continue;
			}
			if (!h->n) continue;
			if (!strcmp(t, "vCmp")) {
				prev_cmp = h->idx;
				fprintf(fp, "// handler[%d] vCmp (condition set)\n", h->idx);
				for (int k = 0; k < h->n; k++) {
					an_insn_loop_braces(fp, &h->insns[k], loops, loop_n, &vs);
					if (g_an_skip > 0) continue;
					an_insn_translate(&h->insns[k], &vs, line, sizeof(line), h, fp);
				}
				fprintf(fp, "\n");
				continue;
			}
			prev_cmp = -1;
			fprintf(fp, "// handler[%d] %s\n", h->idx, t);
			int n = 0;
			for (int k = 0; k < h->n; k++) {
				an_insn_loop_braces(fp, &h->insns[k], loops, loop_n, &vs);
				if (g_an_skip > 0) continue;   /* iteration body - skip */
				if (an_insn_translate(&h->insns[k], &vs, line, sizeof(line), h, fp))
					n++;
			}
			if (n == 0) {
				/* mark engine internals: go back to previous line? simpler: note in next line */
				fprintf(fp, "    // (engine internals only)\n");
			}
			fprintf(fp, "\n");
		}
	}

	fclose(fp);
	/* free */
	for (int hh = 0; hh < hn; hh++)
		if (handlers[hh].insns) free(handlers[hh].insns);
	if (handlers) free(handlers);
	(void)all_lines; (void)all_len; (void)all_cap;
	_plugin_logprintf("[VMP38-Analyze] inline done: %d handlers -> %s", hn, out_path);
}


#define IMM_FREQ_MAX 256
extern int (*vmp38_mem_hook_read)(unsigned int addr, char *pdata, unsigned int size);
static MINI_REG *g_xx_mreg = 0;   /* active mini-reg during xx_execute */
static unsigned long long g_imm_freq_enc[IMM_FREQ_MAX];
static int g_imm_freq_cnt[IMM_FREQ_MAX];
static int g_imm_freq_n = 0;
static unsigned char *g_snap = NULL;
static unsigned long long g_snap_begin = 0, g_snap_end = 0;
static int vmp38_fmem_read_hook(unsigned int addr, char *pdata, unsigned int size)
{
	if (g_snap && addr >= g_snap_begin && (unsigned long long)addr + size <= g_snap_end)
	{
		memcpy(pdata, g_snap + (addr - g_snap_begin), size);
		return 1;
	}
	{
		duint szread = 0;
		if (Script::Memory::Read(addr, pdata, size, &szread) && szread == size)
			return 1;
	}
	if (g_xx_mreg)
	{
		/* addr is absolute; convert to vsp-relative slot */
		unsigned long long vsp_off = (unsigned long long)addr - g_xx_mreg->vsp;
		unsigned long long v = 0;
		int sym = -1;
		if (vsp_off < 0x1000000ULL &&
			mini_mem_read(g_xx_mreg, vsp_off, size * 8, &v))
		{
			memcpy(pdata, &v, size);
			return 1;
		}
	}
	return 0;
}

/* ============================================================================
   x64 Unicorn full-trace — REWRITTEN with official Unicorn API (no x32 deps)
   Independent implementation: UC_MODE_64, RAX..R15 + RIP + EFLAGS snapshots.
   Mirrors the x32 pipeline behaviour (map -> dump -> inject -> trace -> restore)
   but does not reuse any x32-only engine code.
   ========================================================================== */
#define FT64_MAX 2000000
static unsigned long long g_ft64_eip[FT64_MAX];
static unsigned long long g_ft64_regs[FT64_MAX][16];   /* rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi,r8..r15 */
static unsigned long long g_ft64_ef[FT64_MAX];
static unsigned char g_ft64_cond[FT64_MAX];
static unsigned long long g_ft64_ctgt[FT64_MAX];
static int g_ft64_n = 0;
static uc_engine *g_uc64 = 0;
static unsigned long long g_uc64_lo = 0, g_uc64_hi = 0;
static int g_ft64_stop = 0;
static int g_ft64_vmexit_phase = 0;  /* popfq seen: chasing .text restored stub's
                                        return value; next ret (0xC3) = vmret */
static unsigned long long g_module_base = 0;
static unsigned long long g_module_size = 0;

/* per-instruction record for the restore pass */
typedef struct
{
	unsigned long long ip;
	char disasm[160];
	int instr_len;
} FT64_REC;

/* one "handler" = a run of instructions between dispatch targets (simplified:
   we group by vm-section boundaries and report every executed instruction;
   the semantic classification is added by the standalone analyzer). */
typedef struct
{
	int idx;
	unsigned long long vsp_in, vsp_out;
	FT64_REC *insns;
	int n, cap;
} FT64_HDL;

/* ---------------- memory helpers (x64 bridge) ---------------- */
static int ft64_read_mem(unsigned long long addr, void *buf, size_t size)
{
	duint szread = 0;
	if (Script::Memory::Read((duint)addr, buf, size, &szread) && szread == size)
		return 1;
	return 0;
}


/* ---------------- live-memory fallback hooks (x32 parity) ---------------- */
static bool uc_hook_mem_unmapped(uc_engine *uc, uc_mem_type type,
	uint64_t address, int size, int64_t value, void *user_data)
{
	(void)value; (void)user_data;
	if (size > 0 && size <= 256 &&
		(type == UC_MEM_READ_UNMAPPED || type == UC_MEM_WRITE_UNMAPPED ||
		 type == UC_MEM_FETCH_UNMAPPED))
	{
		/* pull the whole page from the live process, map + write it */
		unsigned long long pg = address & ~0xFFFULL;
		unsigned char page[0x1000];
		duint szr = 0;
		size_t n = 0;
		if (Script::Memory::Read((duint)pg, page, sizeof(page), &szr) && szr > 0)
			n = (size_t)((szr < 0x1000) ? szr : 0x1000);
		/* map the page regardless (zero-fill if the live read failed) so the
		   trace never breaks on an unmapped access */
		if (uc_mem_map(uc, pg, 0x1000, UC_PROT_ALL) != UC_ERR_OK)
			;   /* already mapped - fine */
		if (n)
			uc_mem_write(uc, pg, page, n);
		else
		{
			unsigned char zero[0x1000];
			memset(zero, 0, sizeof(zero));
			uc_mem_write(uc, pg, zero, sizeof(zero));
		}
		return true;
	}
	return false;
}

/* VMP uses int3 (0xCC) as anti-debug/junk in handlers - skip it (RIP+1) */
static void uc_hook_intr(uc_engine *uc, uint32_t intno, void *user_data)
{
	(void)intno; (void)user_data;
	{
		uint64_t ip = 0;
		if (uc_reg_read(uc, UC_X86_REG_RIP, &ip) == UC_ERR_OK)
		{
			ip += 1;
			uc_reg_write(uc, UC_X86_REG_RIP, &ip);
		}
	}
}

/* ============================================================================
   x64 normal-function head/tail boundary recognition (x32 vmp38_is_norm_funchead
   / is_norm_func_tail / is_func_boundary parity, kanxue 289859 skeleton).
   Pure opcode-byte matching - registers-independent, same bytes for x86/x64 in
   the supported forms (test reg,reg / push reg+mov reg / call;ret / SEH tail).
   ========================================================================== */
static int g_vm_continue64 = 0;   /* x32 g_vm_continue parity: 0=auto-off */

static int vmp38_is_norm_funchead64(uc_engine *uc, uint64_t addr)
{
	unsigned char b[16];
	if (!uc || !addr)
		return 0;
	int ok = 0;
	__try {
		ok = (uc_mem_read(uc, addr, b, sizeof(b)) == UC_ERR_OK) ? 1 : 0;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		ok = 0;
	}
	if (!ok)
		return 0;
	int i = 0;
	int n_push = 0;
	while (i < 4 && (b[i] == 0x53 || b[i] == 0x55 || b[i] == 0x56 || b[i] == 0x57))
	{
		n_push++;
		i++;
	}
	if (n_push == 0 && i == 0 && b[0] == 0x8B && b[1] == 0xFF)
	{
		i = 2;
		while (i < 6 && (b[i] == 0x53 || b[i] == 0x55 || b[i] == 0x56 || b[i] == 0x57))
		{
			n_push++;
			i++;
		}
	}
	if (i < (int)sizeof(b) - 1)
	{
		if (b[i] == 0x8B && b[i + 1] == 0xEC)
			return 1;
		if (b[i] == 0x83 && (b[i + 1] == 0xC4 || b[i + 1] == 0xEC))
			return 1;
	}
	if (n_push >= 3)
	{
		int j = i;
		int n_pimm = 0;
		while (j + 1 < (int)sizeof(b) && n_pimm < 4)
		{
			if (b[j] == 0x6A) { j += 2; n_pimm++; continue; }
			if (b[j] == 0x68 && j + 4 < (int)sizeof(b)) { j += 5; n_pimm++; continue; }
			break;
		}
		int n_movreg = 0;
		while (j + 1 < (int)sizeof(b))
		{
			if ((b[j] == 0x8B || b[j] == 0x89) && (b[j + 1] >> 6) == 3)
			{
				n_movreg++;
				j += 2;
			}
			else
				break;
		}
		if (n_movreg >= 2)
			return 1;
	}
	/* FPO small-function head (no push prologue, test reg,reg entry) */
	{
		int head_ok = 0;
		for (int n = 0; n + 1 < (int)sizeof(b); n++)
		{
			int is_test = 0;
			if (b[n] == 0x85 && (b[n + 1] >> 6) == 3)
				is_test = 1;
			if (b[n] == 0x84 && (b[n + 1] >> 6) == 3)
				is_test = 1;
			if (is_test && n + 2 < (int)sizeof(b))
			{
				unsigned char jcc = b[n + 2];
				int jlen = 0;
				if ((jcc & 0xF0) == 0x70)
					jlen = 3;
				else if (jcc == 0x0F && n + 3 < (int)sizeof(b) &&
				         (b[n + 3] & 0xF0) == 0x80)
					jlen = 5;
				if (jlen)
				{
					int body = n + jlen;
					int found_tail = 0;
					for (int t = body; t + 1 < (int)sizeof(b) && t <= body + 8; t++)
					{
						if (b[t] == 0xFF && ((b[t + 1] >> 3) & 7) == 2)
							found_tail = 1;
						if (b[t] == 0xE8 && t + 5 < (int)sizeof(b) && b[t + 5] == 0xC3)
							found_tail = 1;
						if (b[t] == 0x64 && t + 7 < (int)sizeof(b) &&
						    b[t + 1] == 0x8F && b[t + 2] == 0x05)
							found_tail = 1;
					}
					if (!found_tail)
					{
						for (int t = body; t + 1 < (int)sizeof(b) && t <= body + 14; t++)
						{
							if (b[t] == 0xE8 && t + 4 < (int)sizeof(b))
								found_tail = 1;
							if (b[t] == 0xFF && ((b[t + 1] >> 3) & 7) == 2)
								found_tail = 1;
						}
					}
					if (found_tail)
						head_ok = 1;
				}
			}
		}
		if (head_ok)
			return 1;
	}
	return 0;
}

static int vmp38_is_norm_func_tail64(uc_engine *uc, uint64_t addr)
{
	unsigned char b[12];
	if (!uc || !addr)
		return 0;
	int ok = 0;
	__try {
		ok = (uc_mem_read(uc, addr, b, sizeof(b)) == UC_ERR_OK) ? 1 : 0;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		ok = 0;
	}
	if (!ok)
		return 0;
	if (b[0] == 0x8B && b[1] == 0xE5)
	{
		if (b[2] == 0x5D && b[3] == 0xC3) return 1;
		if (b[2] == 0x5D && b[3] == 0xE9) return 1;
		if ((b[2] == 0x5B || b[2] == 0x5E || b[2] == 0x5F) &&
			(b[3] == 0xC3 || (b[3] == 0x5D && b[4] == 0xC3)))
			return 1;
	}
	if (b[0] == 0x83 && b[1] == 0xC4 && b[3] == 0x5D && b[4] == 0xC3)
		return 1;
	if (b[0] == 0x5D && b[1] == 0xC3)
		return 1;
	if (b[0] == 0xE8 && b[5] == 0xC3)
		return 1;
	if (b[0] == 0xFF)
	{
		unsigned char modrm = b[1];
		int group = (modrm >> 3) & 7;
		if (group == 2)
		{
			int len = 2, mod = modrm >> 6, rm = modrm & 7;
			if (mod != 3)
			{
				if (rm == 4) len++;
				if (mod == 0 && rm == 5) len += 4;
				else if (mod == 1) len += 1;
				else if (mod == 2) len += 4;
			}
			if (len < (int)sizeof(b) && b[len] == 0xC3)
				return 1;
		}
	}
	return 0;
}

static int vmp38_is_func_boundary64(uc_engine *uc, uint64_t target)
{
	if (vmp38_is_norm_func_tail64(uc, target))
		return 1;
	if (!g_vm_continue64)
		return vmp38_is_norm_funchead64(uc, target);
	return 0;
}

/* ---------------- Unicorn code hook ---------------- */
static void ft64_hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
	(void)uc; (void)size; (void)user_data;
	/* rep-string iteration suppression: same eip repeatedly = engine loop */
	if (g_ft64_n > 0 && g_ft64_eip[g_ft64_n - 1] == address)
	{
		unsigned char pb[2] = { 0, 0 };
		if (uc_mem_read(uc, address, pb, 2) == UC_ERR_OK &&
			(pb[0] == 0xF3 || pb[0] == 0xF2) &&
			(pb[1] >= 0xA4 && pb[1] <= 0xAF))
			return;   /* skip repeat iterations */
	}
	if (g_ft64_n >= FT64_MAX)
	{
		g_ft64_stop = 1;
		uc_emu_stop(uc);
		return;
	}
	/* VM-exit feature (kanxue 280283: vm_start=pushfd, vm_exit=popfd):
	   0x9D = popf/popfd, REX.W 0x48 0x9D = popfq (x64). A popfq/popfd in
	   the VM section is the eflags-restore of the VM exit sequence. But
	   VMP puts the return value computation ("xor ebx,[rsp+..]; ror; mov
	   rax,rbx; ret") in the .text restored stub AFTER the popfq, so we
	   must NOT stop at popfq - chase the stub and stop at its ret (0xC3). */
	{
		unsigned char pb2[2] = { 0, 0 };
		if (uc_mem_read(uc, address, pb2, 2) == UC_ERR_OK)
		{
			if (g_ft64_vmexit_phase && pb2[0] == 0xC3)
			{
				/* .text restored stub's ret = real vmret */
				g_ft64_stop = 2;
				uc_emu_stop(uc);
				return;
			}
			if (pb2[0] == 0x9D || (pb2[0] == 0x48 && pb2[1] == 0x9D))
			{
				g_ft64_vmexit_phase = 1;   /* vm-exit marker: chase stub */
			}
		}
	}
	/* module escape: EIP left the module (base..base+size+slack) -> VM function returned */
	if (g_module_base && (address < g_module_base ||
		address >= g_module_base + g_module_size + 0x100000ULL))
	{
		g_ft64_stop = 3;
		uc_emu_stop(uc);
		return;
	}
	/* normal-function head/tail boundary (x32 vmp38_is_func_boundary parity):
	   when a ret/jmp/call lands on a normal function prologue or epilogue,
	   that's the end of THIS VM function -> stop. Pure byte matching, shared
	   with x32 forms (test reg,reg FPO heads, push-reg+mov-reg heads,
	   call;ret / SEH tails). */
	{
		unsigned char op[16] = { 0 };
		if (uc_mem_read(uc, address, op, sizeof(op)) == UC_ERR_OK)
		{
			int is_ctrl = 0;
			int call_len = 0, jmp_len = 0, ret_len = 0;
			int t64 = op[0];

			if (t64 == 0xC3)                       { ret_len = 1; is_ctrl = 1; }
			else if (t64 == 0xC2)                  { ret_len = 3; is_ctrl = 1; }
			else if (t64 == 0xE8)                  { call_len = 5; is_ctrl = 1; }
			else if (t64 == 0xE9)                  { jmp_len = 5; is_ctrl = 1; }
			else if (t64 == 0xEB)                  { jmp_len = 2; is_ctrl = 1; }
			else if ((t64 & 0xF0) == 0x70)         { jmp_len = 2; is_ctrl = 1; }
			else if (t64 == 0x0F && op[1] >= 0x80 && op[1] <= 0x8F)
				{ jmp_len = 6; is_ctrl = 1; }       /* jcc rel32 */
			else if (t64 == 0xFF)
			{
				int g = (op[1] >> 3) & 7, md = op[1] >> 6, rm = op[1] & 7;
				int ilen = 2;
				if (md != 3)
				{
					if (rm == 4) ilen++;
					if (md == 0 && rm == 5) ilen += 4;
					else if (md == 1) ilen += 1;
					else if (md == 2) ilen += 4;
				}
				if (g == 2) { call_len = ilen; is_ctrl = 1; }
				else if (g == 4) { jmp_len = ilen; is_ctrl = 1; }
			}

			if (is_ctrl)
			{
				/* only chase target for jmp/call (ret boundary tested at address) */
				unsigned long long target = 0;
				int have_target = 0;
				if (call_len || jmp_len)
				{
					unsigned long long rsp = 0;
					if (call_len == 5 && (call_len + 4) <= (int)sizeof(op))
					{
						int32_t rel = (int32_t)((uint32_t)op[1] | ((uint32_t)op[2] << 8) |
							((uint32_t)op[3] << 16) | ((uint32_t)op[4] << 24));
						target = address + call_len + rel;
						have_target = 1;
					}
					else if ((jmp_len == 5 && call_len == 0 && op[0] == 0xE9) && (int)sizeof(op) >= 5)
					{
						int32_t rel = (int32_t)((uint32_t)op[1] | ((uint32_t)op[2] << 8) |
							((uint32_t)op[3] << 16) | ((uint32_t)op[4] << 24));
						target = address + jmp_len + rel;
						have_target = 1;
					}
					else if (jmp_len == 2 && (op[0] == 0xEB || (op[0] & 0xF0) == 0x70))
					{
						int8_t rel = (int8_t)op[1];
						target = address + jmp_len + rel;
						have_target = 1;
					}
					(void)rsp;
				}
				if (have_target && vmp38_is_func_boundary64(uc, target))
				{
					g_ft64_stop = 4;
					uc_emu_stop(uc);
					return;
				}
				if (ret_len && g_ft64_vmexit_phase &&
				    vmp38_is_func_boundary64(uc, address))
				{
					g_ft64_stop = 4;
					uc_emu_stop(uc);
					return;
				}
			}
		}
	}
	{
		static const int regs64[16] = {
			UC_X86_REG_RAX, UC_X86_REG_RCX, UC_X86_REG_RDX, UC_X86_REG_RBX,
			UC_X86_REG_RSP, UC_X86_REG_RBP, UC_X86_REG_RSI, UC_X86_REG_RDI,
			UC_X86_REG_R8, UC_X86_REG_R9, UC_X86_REG_R10, UC_X86_REG_R11,
			UC_X86_REG_R12, UC_X86_REG_R13, UC_X86_REG_R14, UC_X86_REG_R15 };
		int ids[17];
		void *vals[17];
		unsigned long long vv[17];
		int i2;
		g_ft64_eip[g_ft64_n] = address;
		for (i2 = 0; i2 < 16; i2++) { ids[i2] = regs64[i2]; vals[i2] = &vv[i2]; }
		ids[16] = UC_X86_REG_EFLAGS; vals[16] = &vv[16];
		uc_reg_read_batch(uc, ids, vals, 17);
		for (i2 = 0; i2 < 16; i2++)
			g_ft64_regs[g_ft64_n][i2] = vv[i2];
		g_ft64_ef[g_ft64_n] = vv[16];
		g_ft64_n++;
	}
}

/* ---------------- init: open + map + dump ---------------- */
static int ft64_uc_init(unsigned long long vm_lo, unsigned long long vm_hi)
{
	uc_err err;
	if (g_uc64)
	{
		uc_close(g_uc64);
		g_uc64 = 0;
	}
	out("[uc64] uc_open...");
	/* module base for escape detection (x32 parity) */
	{
		Script::Module::ModuleInfo mi64b;
		if (Script::Module::GetMainModuleInfo(&mi64b))
		{
			g_module_base = mi64b.base;
			g_module_size = mi64b.size;
		}
	}
	err = uc_open(UC_ARCH_X86, UC_MODE_64, &g_uc64);
	if (err != UC_ERR_OK)
	{
		out("[uc64] uc_open failed err=%d", (int)err);
		return 0;
	}
	out("[uc64] uc_open OK");

	/* live-memory fallback: unmapped reads pull real process memory;
	   int3 (VMP anti-debug/junk) is skipped so the trace continues */
	{
		uc_hook hhm = 0;
		{
			uc_err he = uc_hook_add(g_uc64, &hhm,
				UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED | UC_HOOK_MEM_FETCH_UNMAPPED,
				(void*)uc_hook_mem_unmapped, NULL, 1, 0);
			out("[uc64] mem-unmapped hook %s", he == UC_ERR_OK ? "enabled" : "FAILED");
		}
		uc_hook_add(g_uc64, &hhm, UC_HOOK_INTR, (void*)uc_hook_intr, NULL, 1, 0);
	}

	/* map address space: try big blocks, fall back to chunked */
	{
		unsigned long long map_sz[] = { 0x80000000ULL, 0x20000000ULL, 0x4000000ULL };
		int mi, mapped = 0;
		for (mi = 0; mi < 3 && !mapped; mi++)
		{
			if (uc_mem_map(g_uc64, 0, map_sz[mi], UC_PROT_ALL) == UC_ERR_OK)
			{
				out("[uc64] map %llu MB OK", (unsigned long long)(map_sz[mi] >> 20));
				mapped = 1;
			}
		}
		if (!mapped)
		{
			/* chunked 1MB up to 512MB */
			unsigned long long a;
			for (a = 0; a < 0x20000000ULL; a += 0x100000ULL)
			{
				if (uc_mem_map(g_uc64, a, 0x100000ULL, UC_PROT_ALL) != UC_ERR_OK)
					break;
			}
			out("[uc64] chunked map up to %08llx", a);
		}
	}

	/* dump module image + VM section + stack region into Unicorn memory */
	{
		Script::Module::ModuleInfo mi;
		if (Script::Module::GetMainModuleInfo(&mi))
		{
			unsigned long long lo = mi.base & ~0xFFFULL;
			unsigned long long hi = mi.base + mi.size + 0x100000ULL;
			unsigned char buf[0x1000];
			unsigned long long a;
			/* x64 modules live at 0x7ff7... - far above the 0..2GB map.
			   Map the module address region explicitly. */
			if (uc_mem_map(g_uc64, lo, (size_t)((hi - lo + 0xFFF) & ~0xFFFULL), UC_PROT_ALL) != UC_ERR_OK)
				out("[uc64] module region map FAILED %08llx-%08llx", lo, hi);
			else
				out("[uc64] module region mapped %08llx-%08llx", lo, hi);
			out("[uc64] dump module %08llx-%08llx", lo, hi);
			for (a = lo; a < hi; a += 0x1000)
			{
				duint szr = 0;
				if (Script::Memory::Read((duint)a, buf, sizeof(buf), &szr) && szr > 0)
					uc_mem_write(g_uc64, a, buf, (size_t)((szr < 0x1000) ? szr : 0x1000));
			}
		}
		if (vm_lo && vm_hi > vm_lo)
		{
			unsigned char buf[0x1000];
			unsigned long long a;
			out("[uc64] dump VM section %08llx-%08llx", vm_lo, vm_hi);
			for (a = vm_lo; a < vm_hi; a += 0x1000)
			{
				duint szr = 0;
				size_t n = (size_t)(((vm_hi - a) < 0x1000) ? (vm_hi - a) : 0x1000);
				if (Script::Memory::Read((duint)a, buf, n, &szr) && szr > 0)
					uc_mem_write(g_uc64, a, buf, n);
			}
		}
	}
	/* stack region near the LIVE process RSP (Unicorn RSP is not set yet) */
	{
		unsigned long long rsp = (unsigned long long)Script::Register::GetRSP();
		if (rsp > 0x20000)
		{
			unsigned long long slo = (rsp - 0x20000) & ~0xFFFULL;
			unsigned long long shi = rsp + 0x20000;
			unsigned char buf[0x1000];
			unsigned long long a;
			out("[uc64] dump stack %08llx-%08llx", slo, shi);
			/* stack region is far above the 0..2GB map - map it first */
			{
				unsigned long long mlo = slo & ~0xFFFULL;
				unsigned long long mhi = (shi + 0xFFF) & ~0xFFFULL;
				if (uc_mem_map(g_uc64, mlo, (size_t)(mhi - mlo), UC_PROT_ALL) != UC_ERR_OK)
					out("[uc64] stack region map FAILED %08llx-%08llx", mlo, mhi);
			}
			for (a = slo; a < shi; a += 0x1000)
			{
				duint szr = 0;
				size_t n = (size_t)(((shi - a) < 0x1000) ? (shi - a) : 0x1000);
				if (Script::Memory::Read((duint)a, buf, n, &szr) && szr > 0)
					uc_mem_write(g_uc64, a, buf, n);
			}
		}
	}
	return 1;
}

/* ---------------- full trace entry ---------------- */
int vmp38_unicorn_full_trace64(unsigned long long entry_ip, unsigned long long entry_rsp,
	unsigned long long *out_last_eip)
{
	uc_err err;
	int log_owned = vmp38_log_open("restore");
	uc_hook hh = 0;
	unsigned long long last_eip = 0;

	if (!g_uc64 && !ft64_uc_init(0, 0))
		return 0;

	/* initial registers: RAX..RDI + RSP/RBP + RIP from live process */
	{
		static const int rmap[16] = {
			UC_X86_REG_RAX, UC_X86_REG_RCX, UC_X86_REG_RDX, UC_X86_REG_RBX,
			UC_X86_REG_RSP, UC_X86_REG_RBP, UC_X86_REG_RSI, UC_X86_REG_RDI,
			UC_X86_REG_R8, UC_X86_REG_R9, UC_X86_REG_R10, UC_X86_REG_R11,
			UC_X86_REG_R12, UC_X86_REG_R13, UC_X86_REG_R14, UC_X86_REG_R15 };
		static const Script::Register::RegisterEnum renum[16] = {
			Script::Register::RegisterEnum::RAX, Script::Register::RegisterEnum::RCX,
			Script::Register::RegisterEnum::RDX, Script::Register::RegisterEnum::RBX,
			Script::Register::RegisterEnum::RSP, Script::Register::RegisterEnum::RBP,
			Script::Register::RegisterEnum::RSI, Script::Register::RegisterEnum::RDI,
			Script::Register::RegisterEnum::R8, Script::Register::RegisterEnum::R9,
			Script::Register::RegisterEnum::R10, Script::Register::RegisterEnum::R11,
			Script::Register::RegisterEnum::R12, Script::Register::RegisterEnum::R13,
			Script::Register::RegisterEnum::R14, Script::Register::RegisterEnum::R15 };
		int i2;
		for (i2 = 0; i2 < 16; i2++)
		{
			unsigned long long v = 0;
			unsigned long long rv = 0;
			v = (unsigned long long)Script::Register::Get(renum[i2]);
			uc_reg_write(g_uc64, rmap[i2], &v);
			(void)rv;
		}
		{
			unsigned long long rip = entry_ip;
			uc_reg_write(g_uc64, UC_X86_REG_RIP, &rip);
		}
		{
			unsigned long long ef = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::CFLAGS);
			ef &= ~0x100ULL;   /* clear TF (VMP anti-debug) */
			uc_reg_write(g_uc64, UC_X86_REG_EFLAGS, &ef);
		}
	}
	/* reset trace */
	g_ft64_n = 0;
	g_ft64_stop = 0;
	g_ft64_vmexit_phase = 0;

	out("=== VMP 3.8 x64 FULL TRACE (Unicorn, entry=%016llx) ===", entry_ip);
	if (uc_hook_add(g_uc64, &hh, UC_HOOK_CODE, (void*)ft64_hook_code, NULL, 1, 0) != UC_ERR_OK)
	{
		out("[uc64] hook_add failed");
		if (log_owned) vmp38_log_close();
		return 0;
	}
	err = uc_emu_start(g_uc64, entry_ip, 0, 0, 0);
	uc_hook_del(g_uc64, hh);
	if (g_ft64_n > 0)
		last_eip = g_ft64_eip[g_ft64_n - 1];
	if (out_last_eip)
		*out_last_eip = last_eip;
	out("=== full trace done: %d instrs, emu err=%d last_eip=%016llx ===", g_ft64_n, (int)err, last_eip);
	if (log_owned) vmp38_log_close();
	return g_ft64_n;
}


/* ============================================================================
   x64 handler semantic classification (x32 identify_handler parity, core ALU
   mapping + control-flow dispatch). Pure mnemonic-based - no engine deps.
   ========================================================================== */
static const char *vmp38_classify_handler64(const char *core_ops, int n_cld,
	int n_cmp, int is_ret, int is_call, int n_not, int n_and, int n_or)
{
	if (n_cld >= 1 && !core_ops[0])
		return "vInit";
	if (is_ret)
		return "vRet";
	if (is_call)
		return "vCall";
	if (n_cmp >= 1)
		return "vCmp";
	if (!core_ops[0])
		return "vDispatch";   /* jmp/ctrl-only group */
	/* Order-sensitive NAND/NOR (x32 parity, kanxue vmp394-handlers):
	   - and;not  or not;and  -> NAND (last gate op = and)
	   - or;not   or not;or   -> NOR  (last gate op = or)
	   Determine by the LAST and/or token in core_ops and whether a NOT
	   token follows it (tokens are '+' separated, e.g. "and+not+shl"). */
	if (n_not > 0 && (n_and > 0 || n_or > 0))
	{
		int last_gate = -1;      /* 0=and 1=or */
		int trailing_not = 0;
		char tmp[160];
		_snprintf(tmp, sizeof(tmp), "%s", core_ops);
		char *tok = strtok(tmp, "+");
		while (tok)
		{
			if (!strcmp(tok, "and")) { last_gate = 0; trailing_not = 0; }
			else if (!strcmp(tok, "or")) { last_gate = 1; trailing_not = 0; }
			else if (!strcmp(tok, "not") && last_gate >= 0)
				trailing_not = 1;
			tok = strtok(NULL, "+");
		}
		if (trailing_not)
			return (last_gate == 0) ? "vNand" : "vNor";
		/* fallback: no trailing not - gate-count heuristic (x32 parity) */
		return (n_or >= 1) ? "vNand" : "vNor";
	}
	/* vALU mapping (x32 parity): count adc vs sbb, then precedence */
	{
		int n_adc = 0, n_sbb = 0;
		const char *p2 = core_ops;
		while ((p2 = strstr(p2, "adc")) != 0) { n_adc++; p2 += 3; }
		p2 = core_ops;
		while ((p2 = strstr(p2, "sbb")) != 0) { n_sbb++; p2 += 3; }
		if (n_sbb > n_adc) return "vSbb";
		if (n_adc > n_sbb) return "vAdc";
		if (strstr(core_ops, "sbb")) return "vSbb";
		if (strstr(core_ops, "adc")) return "vAdc";
		if (strstr(core_ops, "add") && !strstr(core_ops, "sub")) return "vAdd";
		if (strstr(core_ops, "sub")) return "vSub";
		if (strstr(core_ops, "xor")) return "vXor";
		if (strstr(core_ops, "and")) return "vAnd";
		if (strstr(core_ops, "or")) return "vOr";
		if (strstr(core_ops, "imul") || strstr(core_ops, "mul")) return "vMul";
		if (strstr(core_ops, "idiv") || strstr(core_ops, "div")) return "vDiv";
		if (strstr(core_ops, "shl") || strstr(core_ops, "sal") || strstr(core_ops, "shld")) return "vShl";
		if (strstr(core_ops, "shr") || strstr(core_ops, "sar") || strstr(core_ops, "shrd")) return "vShr";
		if (strstr(core_ops, "rol")) return "vRol";
		if (strstr(core_ops, "ror")) return "vRor";
	}
	return "vALU";
}

/* is mnemonic a core ALU op? (VMP handler bodies are built around one) */
static int vmp38_is_core_op64(const char *mn)
{
	static const char *core_ops[] = {
		"adc","sbb","add","sub","xor","and","or","not",
		"imul","mul","idiv","div","shl","sal","shld",
		"shr","sar","shrd","rol","ror", NULL };
	for (int i = 0; core_ops[i]; i++)
		if (!strcmp(mn, core_ops[i]))
			return 1;
	return 0;
}

/* ============================================================================
   x64 constant extraction ([const] parity with x32):
   parse "0x..." immediates out of a disasm string.
   ========================================================================== */
static int vmp38_parse_num64(const char *s, unsigned long long *out)
{
	unsigned long long v = 0;
	int any = 0;
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		s += 2;
	else if (s[0] == '-')
	{
		unsigned long long neg = 0;
		if (!vmp38_parse_num64(s + 1, &neg))
			return 0;
		*out = (unsigned long long)(-(long long)neg);
		return 1;
	}
	while (*s)
	{
		int d;
		if (*s >= '0' && *s <= '9') d = *s - '0';
		else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
		else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
		else return any ? 1 : 0;
		v = (v << 4) | (unsigned long long)d;
		any = 1;
		s++;
	}
	if (!any)
		return 0;
	*out = v;
	return 1;
}

static int vmp38_parse_imm64(const char *disasm, unsigned long long *out)
{
	const char *p = disasm;
	const char *last_num = NULL;
	int have = 0;
	while (*p)
	{
		if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') ||
			(*p >= 'A' && *p <= 'F') || *p == 'x')
		{
			const char *s = p;
			while (s > disasm && ((s[-1] >= '0' && s[-1] <= '9') ||
				(s[-1] >= 'a' && s[-1] <= 'f') || (s[-1] >= 'A' && s[-1] <= 'F') ||
				s[-1] == 'x'))
				s--;
			last_num = s;
			have = 1;
			while (*p && ((*p >= '0' && *p <= '9') ||
				(*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F') || *p == 'x'))
				p++;
			continue;
		}
		p++;
	}
	if (have && last_num && vmp38_parse_num64(last_num, out))
		return 1;
	return 0;
}

/* ============================================================================
   x64 [decrypt] dump (x32 parity): for a handler's STEP_REC array, find
   mov/push imm candidates and try to decrypt the encrypted constant via the
   ValueCommand::Calc chain extractor. Requires per-handler STEP_REC fill.
   ========================================================================== */
static void vmp38_decrypt_dump64(FILE *fo, STEP_REC *recs, int n, int h_idx)
{
	int i;
	for (i = 0; i < n; i++)
	{
		char m[32];
		unsigned long long enc = 0;
		get_mnemonic(recs[i].disasm, m, sizeof(m));
		if (!strcmp(m, "mov") && !strchr(recs[i].disasm, '[') &&
			strchr(recs[i].disasm, ',') &&
			parse_imm_from_disasm(recs[i].disasm, &enc))
		{
			/* mov reg, imm(enc) - carrier = dest reg */
			char op1[64] = "";
			int carrier = -1;
			int is_cand;
			const char *p = recs[i].disasm;
			unsigned long long dec = 0;
			while (*p && *p != ' ') p++;
			while (*p == ' ') p++;
			{
				int k = 0;
				while (*p && *p != ',' && k < 63) op1[k++] = *p++;
				op1[k] = 0;
			}
			carrier = reg_index_from_name(op1);
			is_cand = (enc > 0x40000000ULL && enc <= 0xffffffffULL) ||
				(enc > 0xffffffffULL);
			if (is_cand && carrier >= 0 &&
				try_decrypt_enc_imm(enc, recs, n, i + 1, carrier, &dec))
			{
				fprintf(fo, "[decrypt] handler[%d] @%016llx enc=0x%llx -> real=0x%llx  %s\n",
					h_idx, recs[i].ip, enc, dec, recs[i].disasm);
			}
		}
		else if ((!strcmp(m, "push")) && strchr(recs[i].disasm, ',') == 0 &&
			parse_imm_from_disasm(recs[i].disasm, &enc))
		{
			unsigned long long dec = 0;
			if (enc > 0x40000000ULL &&
				try_decrypt_enc_imm(enc, recs, n, i + 1, -1, &dec))
			{
				fprintf(fo, "[decrypt] handler[%d] @%016llx enc=0x%llx -> real=0x%llx  %s\n",
					h_idx, recs[i].ip, enc, dec, recs[i].disasm);
			}
		}
	}
}

/* ============================================================================
   x64 restore_from_trace64 — simplified handler grouping + .data.txt output
   (semantic classification left to the standalone analyzer)
   ========================================================================== */

/* group instructions into "handlers": a new handler starts when the previous
   instruction was a control-flow (jmp/call/ret) or the vsp changed direction.
   Every executed instruction is recorded (I lines) so the analyzer can
   classify; H lines mark the groups. */

/* ============================================================================
   x64 restore_from_trace64 — simplified handler grouping + .data.txt output
   (semantic classification left to the standalone analyzer)
   ========================================================================== */

/* group instructions into "handlers": a new handler starts when the previous
   instruction was a control-flow (jmp/call/ret) or the vsp changed direction.
   Every executed instruction is recorded (I lines) so the analyzer can
   classify; H lines mark the groups. */

/* ============================================================================
   x64 restore_from_trace64 — handler grouping + semantic classification
   ========================================================================== */
void vmp38_restore_from_trace64(int n)
{
	FILE *g_fout64 = 0;
	FILE *g_fdata64 = 0;
	char fpath[MAX_PATH] = "";
	char dpath[MAX_PATH] = "";
	char exedir[MAX_PATH] = "";
	DISASM_INSTR di;
	int h_idx = 0;
	int h_n = 0;
	unsigned long long h_vsp_in = 0;
	int h_start = 0;
	int log_owned = vmp38_log_open("restore");
	SYSTEMTIME st;
	/* per-handler accumulators */
	char h_core[160] = "";
	int h_n_core = 0;
	int h_n_cld = 0;
	int h_n_cmp = 0;
	int h_is_ret = 0;
	int h_is_call = 0;
	int h_n_not = 0, h_n_and = 0, h_n_or = 0;

	GetModuleFileNameA(NULL, exedir, sizeof(exedir));
	{
		char *sl = strrchr(exedir, '\\');
		if (sl) *sl = 0;
	}
	GetLocalTime(&st);
	_snprintf(fpath, sizeof(fpath), "%s\\xxvm\\vmp38_restore_%04d%02d%02d-%02d%02d%02d-%03d.txt",
		exedir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	_snprintf(dpath, sizeof(dpath), "%s\\xxvm\\vmp38_restore_%04d%02d%02d-%02d%02d%02d-%03d.data.txt",
		exedir, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	g_fout64 = fopen(fpath, "w");
	if (g_fout64) setvbuf(g_fout64, NULL, _IOFBF, 1 << 20);
	g_fdata64 = fopen(dpath, "w");
	if (g_fdata64) setvbuf(g_fdata64, NULL, _IOFBF, 1 << 20);

	if (g_fout64)
		fprintf(g_fout64, "=== xxvm vmp38 x64 restore from Unicorn trace ===\n");
	if (g_fdata64)
		fprintf(g_fdata64, "# xxvm vmp38 x64 restore data v1 - standalone analyzer input\n"
			"# H idx=<handler> n=<insns> vsp_in vsp_out | I ip len alu jmp core cond ctgt val wreg wval disasm\n");

	/* parameter capture at entry (VMP 3.8 leaves args on the VM entry stack) */
	if (n > 0)
	{
		unsigned long long rsp0 = g_ft64_regs[0][4];
		unsigned long long p1 = 0, p2 = 0, p3 = 0;
		duint szr = 0;
		(void)rsp0;
		if (Script::Memory::Read((duint)(rsp0 + 0x18), &p1, 8, &szr) && szr == 8 &&
			Script::Memory::Read((duint)(rsp0 + 0x20), &p2, 8, &szr) && szr == 8 &&
			Script::Memory::Read((duint)(rsp0 + 0x28), &p3, 8, &szr) && szr == 8)
		{
			out("[param] entry rsp=%016llx p1=%016llx p2=%016llx p3=%016llx (esp+0x18..)",
				rsp0, p1, p2, p3);
			if (g_fout64)
				fprintf(g_fout64, "[param] p1=%016llx p2=%016llx p3=%016llx\n", p1, p2, p3);
		}
	}

	h_vsp_in = (n > 0) ? g_ft64_regs[0][4] : 0;
	h_n = 0;

	for (int i = 0; i < n; i++)
	{
		unsigned long long ip = g_ft64_eip[i];
		DISASM_INSTR ins;
		const char *dis = "";
		int len = 1;
		int is_ctrl = 0;
		const char *mn = "";

		memset(&ins, 0, sizeof(ins));
		DbgDisasmAt((duint)ip, &ins);
		if (ins.instruction[0])
		{
			dis = ins.instruction;
			len = ins.instr_size > 0 ? ins.instr_size : 1;
		}
		else
		{
			/* unreadable - skip */
			continue;
		}
		{
			static char mnbuf[64] = "";
			const char *ipstr = ins.instruction;
			int k = 0;
			mnbuf[0] = 0;
			if (ipstr)
			{
				while (ipstr[k] && !isspace((unsigned char)ipstr[k]) && k < 63) { mnbuf[k] = ipstr[k]; k++; }
				mnbuf[k] = 0;
			}
			mn = mnbuf;
		}
		if (!strcmp(mn, "jmp") || !strcmp(mn, "call") || !strcmp(mn, "ret") ||
			!strcmp(mn, "retn") || mn[0] == 'j')
			is_ctrl = 1;
		/* jcc = conditional jump, ctgt = jump target */
		{
			int is_jcc = (mn[0] == 'j' && strcmp(mn, "jmp") != 0);
			unsigned long long ct = 0;
			if (mn[0] == 'j') {
				const char *sp = strchr(dis, ' ');
				if (sp) {
					while (*sp == ' ' || *sp == '\t') sp++;
					if (sp[0] == '0' && (sp[1] == 'x' || sp[1] == 'X')) {
						ct = strtoull(sp + 2, NULL, 16);
					}
				}
			}
			g_ft64_cond[g_ft64_n] = is_jcc ? 1 : 0;
			g_ft64_ctgt[g_ft64_n] = ct;
		}

		/* accumulate per-handler semantics */
		if (vmp38_is_core_op64(mn))
		{
			if (strlen(h_core) < 100)
			{
				if (h_n_core) SCAT(h_core, "+");
				SCAT(h_core, mn);
			}
			h_n_core++;
		}
		if (!strcmp(mn, "cld")) h_n_cld++;
		if (!strcmp(mn, "cmp")) h_n_cmp++;



		if (!strcmp(mn, "not")) h_n_not++;
		if (!strcmp(mn, "and")) h_n_and++;
		if (!strcmp(mn, "or")) h_n_or++;
		if (!strcmp(mn, "ret") || !strcmp(mn, "retn")) h_is_ret = 1;
		if (!strcmp(mn, "call")) h_is_call = 1;

		/* new handler on control flow boundary */
		if (is_ctrl && h_n > 0)
		{
			const char *tag = vmp38_classify_handler64(h_core, h_n_cld, h_n_cmp, h_is_ret, h_is_call, h_n_not, h_n_and, h_n_or);
			/* [const] dump: ALU ops with immediate (x32 parity) */
			if (g_fout64) for (int ci = h_start; ci < i; ci++)
			{
				DISASM_INSTR ci_ins;
				const char *ci_dis = "";
				char ci_mn[32] = "";
				unsigned long long cimm = 0;
				memset(&ci_ins, 0, sizeof(ci_ins));
				DbgDisasmAt((duint)g_ft64_eip[ci], &ci_ins);
				if (ci_ins.instruction[0]) ci_dis = ci_ins.instruction;
				{ const char *ip_ = ci_dis; int k_ = 0; while (ip_[k_] && !isspace((unsigned char)ip_[k_]) && k_ < 31) { ci_mn[k_] = ip_[k_]; k_++; } ci_mn[k_] = 0; }
				if (strchr(ci_dis, ',') && vmp38_parse_imm64(ci_dis, &cimm) &&
					(!strcmp(ci_mn, "add") || !strcmp(ci_mn, "sub") || !strcmp(ci_mn, "xor") ||
					 !strcmp(ci_mn, "and") || !strcmp(ci_mn, "or") || !strcmp(ci_mn, "rol") ||
					 !strcmp(ci_mn, "ror") || !strcmp(ci_mn, "shl") || !strcmp(ci_mn, "shr") ||
					 !strcmp(ci_mn, "sar") || !strcmp(ci_mn, "imul")))
				{
					fprintf(g_fout64, "[const] handler[%d] @%016llx %s imm=0x%llx\n",
						h_idx, g_ft64_eip[ci], ci_mn, cimm);
					if (g_fdata64)
						fprintf(g_fdata64, "C ip=%llx mn=%s imm=%llx\n", g_ft64_eip[ci], ci_mn, cimm);
				}
			}
			/* [decrypt] dump (x32 parity): build STEP_REC for this handler */
			if (g_fout64)
			{
				int hlen = i - h_start;
				STEP_REC *drecs = (STEP_REC*)calloc(hlen ? hlen : 1, sizeof(STEP_REC));
				if (drecs)
				{
					for (int di = 0; di < hlen; di++)
					{
						DISASM_INSTR dis2;
						memset(&dis2, 0, sizeof(dis2));
						drecs[di].ip = g_ft64_eip[h_start + di];
						DbgDisasmAt((duint)drecs[di].ip, &dis2);
						if (dis2.instruction[0])
						{
							_snprintf(drecs[di].disasm, sizeof(drecs[di].disasm), "%s", dis2.instruction);
							drecs[di].instr_len = dis2.instr_size > 0 ? dis2.instr_size : 1;
						}
					}
					vmp38_decrypt_dump64(g_fout64, drecs, hlen, h_idx);
					free(drecs);
				}
			}
			if (g_fout64)
				fprintf(g_fout64, "handler[%d] %s (core: %s) n=%d vsp=%016llx->%016llx\n",
					h_idx, tag, h_core[0] ? h_core : "-", h_n, h_vsp_in, g_ft64_regs[i][4]);
			if (g_fdata64)
			{
				fprintf(g_fdata64, "T type=%s\n", tag);
				fprintf(g_fdata64, "H idx=%d n=%d vsp_in=%llx vsp_out=%llx\n",
					h_idx, h_n, h_vsp_in, g_ft64_regs[i][4]);
			}
			h_idx++;
			h_n = 0;
			h_vsp_in = g_ft64_regs[i][4];
			h_start = i;
			h_core[0] = 0; h_n_core = 0; h_n_cld = 0; h_n_cmp = 0;
			h_is_ret = 0; h_is_call = 0;
			h_n_not = 0; h_n_and = 0; h_n_or = 0;
		}

		/* I line with register snapshots */
		if (g_fdata64)
		{
			fprintf(g_fdata64, "I ip=%llx len=%d alu=%d jmp=%d core=%d cond=%d ctgt=%llx val=0 wreg=0 wval=0 disasm=\"%s\"",
				ip, len, 0, is_ctrl, vmp38_is_core_op64(mn), g_ft64_cond[g_ft64_n], g_ft64_ctgt[g_ft64_n], dis ? dis : "");
			fprintf(g_fdata64, " r0=%llx r1=%llx r2=%llx r3=%llx r4=%llx r5=%llx r6=%llx r7=%llx ef=%llx\n",
				g_ft64_regs[i][0], g_ft64_regs[i][1], g_ft64_regs[i][2], g_ft64_regs[i][3],
				g_ft64_regs[i][4], g_ft64_regs[i][5], g_ft64_regs[i][6], g_ft64_regs[i][7],
				g_ft64_ef[i]);
		}
		if (g_fout64)
			fprintf(g_fout64, "    [%d] @%016llx %s\n", i, ip, dis ? dis : "?");
		h_n++;
	}
	/* last handler */
	if (h_n > 0)
	{
		const char *tag = vmp38_classify_handler64(h_core, h_n_cld, h_n_cmp, h_is_ret, h_is_call, h_n_not, h_n_and, h_n_or);
		if (g_fout64) for (int ci = h_start; ci < n; ci++)
		{
			DISASM_INSTR ci_ins;
			const char *ci_dis = "";
			char ci_mn[32] = "";
			unsigned long long cimm = 0;
			memset(&ci_ins, 0, sizeof(ci_ins));
			DbgDisasmAt((duint)g_ft64_eip[ci], &ci_ins);
			if (ci_ins.instruction[0]) ci_dis = ci_ins.instruction;
			{ const char *ip_ = ci_dis; int k_ = 0; while (ip_[k_] && !isspace((unsigned char)ip_[k_]) && k_ < 31) { ci_mn[k_] = ip_[k_]; k_++; } ci_mn[k_] = 0; }
			if (strchr(ci_dis, ',') && vmp38_parse_imm64(ci_dis, &cimm) &&
				(!strcmp(ci_mn, "add") || !strcmp(ci_mn, "sub") || !strcmp(ci_mn, "xor") ||
				 !strcmp(ci_mn, "and") || !strcmp(ci_mn, "or") || !strcmp(ci_mn, "rol") ||
				 !strcmp(ci_mn, "ror") || !strcmp(ci_mn, "shl") || !strcmp(ci_mn, "shr") ||
				 !strcmp(ci_mn, "sar") || !strcmp(ci_mn, "imul")))
				fprintf(g_fout64, "[const] handler[%d] @%016llx %s imm=0x%llx\n",
					h_idx, g_ft64_eip[ci], ci_mn, cimm);
		}
		if (g_fout64) {
			int hlen = n - h_start;
			STEP_REC *drecs = (STEP_REC*)calloc(hlen ? hlen : 1, sizeof(STEP_REC));
			if (drecs) {
				for (int di = 0; di < hlen; di++) {
					DISASM_INSTR dis2;
					memset(&dis2, 0, sizeof(dis2));
					drecs[di].ip = g_ft64_eip[h_start + di];
					DbgDisasmAt((duint)drecs[di].ip, &dis2);
					if (dis2.instruction[0]) {
						_snprintf(drecs[di].disasm, sizeof(drecs[di].disasm), "%s", dis2.instruction);
						drecs[di].instr_len = dis2.instr_size > 0 ? dis2.instr_size : 1;
					}
				}
				vmp38_decrypt_dump64(g_fout64, drecs, hlen, h_idx);
				free(drecs);
			}
		}
		if (g_fout64)
			fprintf(g_fout64, "handler[%d] %s (core: %s) n=%d vsp=%016llx->%016llx\n",
				h_idx, tag, h_core[0] ? h_core : "-", h_n, h_vsp_in, g_ft64_regs[n-1][4]);
		if (g_fdata64)
		{
			fprintf(g_fdata64, "T type=%s\n", tag);
			fprintf(g_fdata64, "H idx=%d n=%d vsp_in=%llx vsp_out=%llx\n",
				h_idx, h_n, h_vsp_in, g_ft64_regs[n-1][4]);
		}
	}

	/* result: last non-zero rax snapshot */
	{
		unsigned long long last_rax = 0;
		for (int i = n - 1; i >= 0; i--)
		{
			if (g_ft64_regs[i][0] != 0)
			{
				last_rax = g_ft64_regs[i][0];
				break;
			}
		}
		if (last_rax)
		{
			out("[result] rax = %016llx (last non-zero snapshot)", last_rax);
			if (g_fout64)
				fprintf(g_fout64, "[result] rax = %016llx\n", last_rax);
		}
	}

	/* [vmret]: trace ended by module escape (stop=3) or popfq (stop=2)
	   = VM function returned to caller. */
	if (n > 0)
	{
		unsigned long long last_eip = g_ft64_eip[n - 1];
		const char *why = "";
		if (g_ft64_stop == 3) why = "module escape";
		else if (g_ft64_stop == 2) why = "popfq vm-exit";
		if (g_ft64_stop == 2 || g_ft64_stop == 3)
		{
			out("[vmret] trace ended @ %016llx (%s) - VM function returned", last_eip, why);
			if (g_fout64)
				fprintf(g_fout64, "[vmret] @ %016llx (%s)\n", last_eip, why);
		}
	}

	/* [vregs]: entry register snapshot = per-sample VR mapping evidence
	   (kanxue vmp394-handlers: VR4=r8 etc. - sample-specific). */
	if (n > 0 && g_fout64)
	{
		fprintf(g_fout64, "[vregs] entry rax=%016llx rcx=%016llx rdx=%016llx rbx=%016llx rsp=%016llx rbp=%016llx rsi=%016llx rdi=%016llx\n",
			g_ft64_regs[0][0], g_ft64_regs[0][1], g_ft64_regs[0][2], g_ft64_regs[0][3],
			g_ft64_regs[0][4], g_ft64_regs[0][5], g_ft64_regs[0][6], g_ft64_regs[0][7]);
		fprintf(g_fout64, "[vregs] entry r8=%016llx r9=%016llx r10=%016llx r11=%016llx r12=%016llx r13=%016llx r14=%016llx r15=%016llx eflags=%016llx\n",
			g_ft64_regs[0][8], g_ft64_regs[0][9], g_ft64_regs[0][10], g_ft64_regs[0][11],
			g_ft64_regs[0][12], g_ft64_regs[0][13], g_ft64_regs[0][14], g_ft64_regs[0][15], g_ft64_ef[0]);
	}

	if (g_fout64) { fprintf(g_fout64, "=== trace restore done: %d handlers in %d instrs ===\n", h_idx + (h_n ? 1 : 0), n); fclose(g_fout64); }
	if (g_fdata64) fclose(g_fdata64);
	out("=== trace restore done: %d handlers in %d instrs (see xxvm dir) ===", h_idx + (h_n ? 1 : 0), n);
	if (log_owned) vmp38_log_close();
}

/* ============================================================================
   x64 vmp38_scan_all_entries — call-target scan (x32 parity, pure byte scan)
   Reads the on-disk .text (plaintext even when in-memory is VMP-encrypted
   pre-stub), finds direct call rel32 whose target lands in the VM section.
   Each target = REAL virtualized function entry; call site = its caller.
   No disassembler engine dependency.
   ========================================================================== */
int vmp38_scan_all_entries(void)
{
	int log_owned = vmp38_log_open("scan");
	Script::Module::ModuleInfo mi;
	unsigned long long vmsect_begin = 0, vmsect_end = 0;
	int i, found = 0;

	if (!Script::Module::GetMainModuleInfo(&mi))
	{
		if (log_owned) vmp38_log_close();
		return 0;
	}
	/* VM section = the largest non-.text section */
	{
		int n = Script::Module::SectionCountFromAddr(mi.base);
		unsigned long long max_size = 0;
		for (i = 0; i < n; i++)
		{
			Script::Module::ModuleSectionInfo si;
			if (Script::Module::SectionFromAddr(mi.base, i, &si))
			{
				if (strcmp(si.name, ".text") != 0 && si.size > max_size)
				{
					max_size = si.size;
					vmsect_begin = si.addr;
					vmsect_end = si.addr + si.size;
				}
			}
		}
	}
	if (!vmsect_begin)
	{
		if (log_owned) vmp38_log_close();
		return 0;
	}
	out("vmp38_scan_all_entries: vmsect=[%08llx-%08llx]", vmsect_begin, vmsect_end);

	/* CALL-TARGET scan: read .text bytes from the FILE (VMP encrypts
	   in-memory .text before the stub runs), find E8 call rel32 -> VM */
	{
		unsigned long long ent_static[256];
		unsigned long long ec_static[256] = {0};
		unsigned long long *entries = ent_static;
		unsigned long long *entry_call = ec_static;
		int n_entries_cap = 256;
		{
			unsigned long long *ep = (unsigned long long*)malloc(65536 * sizeof(unsigned long long));
			unsigned long long *ecp = (unsigned long long*)calloc(65536, sizeof(unsigned long long));
			if (ep && ecp) { entries = ep; entry_call = ecp; n_entries_cap = 65536; }
			else { if (ep) free(ep); if (ecp) free(ecp); }
		}
		int n_entries = 0, kk;
		FILE *pf = fopen(mi.path, "rb");
		out("  [call-scan] fopen %s = %s", mi.path, pf ? "OK" : "FAIL");
		if (pf)
		{
			unsigned char ph[0x400];
			unsigned int peo, nse, opsz;
			long fsz;
			int i2;
			fseek(pf, 0, SEEK_END); fsz = ftell(pf);
			fseek(pf, 0x3c, SEEK_SET);
			if (fread(ph, 1, 4, pf) == 4)
			{
				peo = ph[0] | (ph[1] << 8) | (ph[2] << 16) | (ph[3] << 24);
				fseek(pf, peo, SEEK_SET);
				if (fread(ph, 1, 0x18, pf) == 0x18)
				{
					nse = ph[6] | (ph[7] << 8);
					opsz = ph[20] | (ph[21] << 8);
					for (i2 = 0; i2 < nse; i2++)
					{
						unsigned char sh[40];
						unsigned int vsz, vad, rptr, rsz;
						long off = peo + 0x18 + opsz + 40 * i2;
						fseek(pf, off, SEEK_SET);
						if (fread(sh, 1, 40, pf) != 40) break;
						vsz = sh[8] | (sh[9] << 8) | (sh[10] << 16) | (sh[11] << 24);
						vad = sh[12] | (sh[13] << 8) | (sh[14] << 16) | (sh[15] << 24);
						rsz = sh[16] | (sh[17] << 8) | (sh[18] << 16) | (sh[19] << 24);
						rptr = sh[20] | (sh[21] << 8) | (sh[22] << 16) | (sh[23] << 24);
						if (memcmp(sh, ".text", 5) == 0)
						{
							unsigned char *tb = (unsigned char*)malloc(rsz ? rsz : 1);
							unsigned int off3;
							if (tb)
							{
								fseek(pf, rptr, SEEK_SET);
								if (fread(tb, 1, rsz, pf) == rsz)
								{
									for (off3 = 0; off3 + 5 < rsz; off3++)
									{
										if (tb[off3] == 0xE8)
										{
											int rel = (int)(tb[off3+1] | (tb[off3+2] << 8) |
												(tb[off3+3] << 16) | (tb[off3+4] << 24));
											unsigned long long tgt = mi.base + (unsigned long long)(vad + off3 + 5) + (unsigned long long)rel;
											if (tgt >= vmsect_begin && tgt < vmsect_end)
											{
												int dup = 0;
												for (kk = 0; kk < n_entries; kk++)
													if (entries[kk] == tgt) dup = 1;
												if (!dup && n_entries < n_entries_cap)
												{
													entries[n_entries] = tgt;
													entry_call[n_entries] = mi.base + (unsigned long long)(vad + off3);
													n_entries++;
												}
											}
										}
									}
								}
								free(tb);
			if (entries != ent_static) { free(entries); free(entry_call); }
							}
							break;
						}
					}
				}
			}
			fclose(pf);
		}
		out("=== [vm-func] %d VM-ized function call sites found ===", n_entries);
		for (kk = 0; kk < n_entries; kk++)
		{
			out("[vm-func] call site 0x%llX (rva 0x%llX) -> VM-ized impl 0x%llX (rva 0x%llX) <== REAL VM-ized function",
				entry_call[kk], entry_call[kk] - mi.base, entries[kk], entries[kk] - mi.base);
			found++;
		}
	}
	if (log_owned) vmp38_log_close();
	return found;
}

/* DYN background thread wrapper (x32 parity): the trace runs on its own
   thread so the GUI stays responsive; VMP38-Stop sets g_dyn_stop. */
DWORD WINAPI vmp38_dyn_thread(LPVOID param)
{
	(void)param;
	unsigned long long start_ip = g_dyn_start ? g_dyn_start : (unsigned long long)Script::Register::GetEIP();
	_plugin_logprintf("[VMP38] dyn thread start ip=%016llx", start_ip);
	vmp38_dynamic_trace(start_ip);
	g_dyn_running = 0;
	return 0;
}

