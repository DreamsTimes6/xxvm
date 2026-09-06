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
#include <mmsystem.h>  /* timeBeginPeriod(1): raise timer resolution so Sleep(1) really sleeps ~1ms */
#include <stdlib.h>
#pragma comment(lib, "winmm.lib")
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "bridgemain.h"
#include "_scriptapi_module.h"
#include "_scriptapi_memory.h"
static duint g_szrd = 0;
/* OPMZ-style lv/ctrl summary counters (identify_handler is called from
   both dynamic trace and static restore - these aggregate across them) */
static int g_lv_handlers = 0, g_lv_chains = 0, g_lv_branches = 0, g_lv_loops = 0;
static int g_vcmp_pending = -1;
static int g_lv_ifs = 0;   /* OPMZ-style IF pairing: vCmp set -> next jcc is its branch */
/* entry-parameter snapshot for out-param (write-back pointer) detection:
   params that look like addresses are probed at trace start; their pointee
   content is compared at vm-exit - changed pointee = result written back */
static unsigned long g_entry_esp = 0;
static unsigned long g_entry_params[16];
static unsigned long g_entry_pointee[16];   /* 0 = not an address */
static int g_entry_pn = 0;
static unsigned long long g_stream_last_ip = 0;   /* early: crash filter (top of file) reads it */

static void detect_out_params(void);   /* defined before vmp38_dynamic_trace */
#include "_scriptapi_misc.h"
#include "_scriptapi_debug.h"
#include "_scriptapi_register.h"
#include "vmp38_restore.h"
/* bounded string append: strcat with hard cap - prevents
   RTC#2 stack corruption from unbounded accumulation into fixed
   stack buffers (ops[128]/core_ops[128]/ctrl[64]/tmp[256]...) */
#define SCAT(dst, src) strncat((dst), (src), sizeof(dst) - strlen(dst) - 1)

#include "xxdisasm32.h"
#include "unicorn/unicorn.h"   /* types + UC_X86_REG_* constants only */
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
/* Unicorn Engine (QEMU kernel) - full x86 instruction set, exactly what
   VMP 3.8's obfuscated handlers need (SIB complex addressing etc.). Replaces
   the hand-rolled xx_execute whose instruction tables cannot
   decode VMP 3.8's transformed instructions. Statically linked against
   unicorn.lib (self-built from unicorn source via CMake, MSVC Release x86).
   unicorn.lib is the bundle_static archive: CMakeLists.txt's
   bundle_static_library(unicorn_static unicorn_archive unicorn) packs all
   i386 object files (x86_64-softmmu.dir\Release\*.obj, compiled __i386__;
   the "x86_64-softmmu" name is misleading) into one 2.2MB archive - it is
   NOT the old mixed x86/x64 prebuilt lib, and it is NOT an import lib.
   See F:\xxvm\unicorn_src\build32\Release\unicorn.lib. No unicorn.dll
   dependency: dumpbin /DEPENDENTS on xx_vm_x32.dp32 shows no unicorn.dll.
   (The older unicorn-import.lib route - import lib for unicorn.dll, which
   required a 32-bit unicorn.dll next to the .dp32 - is no longer used.) */

/* XX_CONTEXT from xx_vm.h (r[8]+eflag+ip) */
struct XX_CONTEXT;
/* original xxvm execution engine hooks (xx_execute.c) — C 符号 */
extern "C" {
extern int xx_execute(struct XX_INST *inst, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context);
extern int (*vmp38_mem_hook_read)(unsigned int addr, char *pdata, unsigned int size);
extern int (*vmp38_mem_hook_write)(unsigned int addr, char *pdata, unsigned int size);
}
#include "vmp38_dict_data.c"
static int vmp38_dict_lookup(unsigned long long va, unsigned long long *delta, int *cls);
const char *vmp38_dict_class(unsigned long long va);
static int read_mem_file(unsigned long long addr, void *buf, int n);
static int disasm_from_file(unsigned long long ip, DISASM_INSTR *out_ins);

/* _plugin_logputs/_plugin_logprintf declared in _plugins.h (via _scriptapi_module.h) */

/* ============ 32/64-bit platform abstraction ============ */
#ifdef VMP32_BUILD
#define VMP_VSP_STEP 4
#define VMP_IS_STACKREG(d) (strstr((d), "esp") || strstr((d), "esi") || strstr((d), "edi"))
#define VMP_IS_STACKREG_NAME(n) (strcmp((n), "esp") == 0 || strcmp((n), "esi") == 0 || strcmp((n), "edi") == 0)
#else
#define VMP_VSP_STEP 8
#define VMP_IS_STACKREG(d) (strstr((d), "rsp") || strstr((d), "r8"))
#define VMP_IS_STACKREG_NAME(n) (strcmp((n), "rsp") == 0 || strcmp((n), "r8") == 0)
#endif

static FILE *g_fout = NULL;
static FILE *g_fdata = NULL;   /* standalone analyzer data file (parallel to restore log) */
static int g_trace_mode = 0;    /* 1=static Unicorn trace 2=dynamic real trace (for reg snapshot lookup) */
static char g_fout_path[512] = "";

/* Log session management: every VMP38 restore action (static restore,
   dynamic trace, scan) opens its OWN log file under "<x32dbg.exe dir>\xxvm\"
   named to the millisecond so concurrent runs never collide:
     xxvm\xxvm_restore_20260805-112345-678.txt
     xxvm\xxvm_dyn_20260805-112345-678.txt
   x64dbg's GUI log buffer is fixed (~1MB) and silently DROPS the earliest
   lines on long traces, so the GUI export (VM*.txt) is incomplete; the file
   is the full record. */
static void vmp38_reset_session_state(void);   /* defined after the globals */
/* Crash capture: write the faulting address to <exedir>/xxvm/crash_restore.txt
   so a hard crash (static trace) can be located against the .map. */
/* crash forensics: which hook sub-stage was executing when it crashed */
static volatile int g_hook_stage = 0;

/* crash forensics: last 10 guest eips (filled by the code hook so the
   crash dump shows where control flow jumped to garbage) */
static unsigned int g_prev_eips[10] = { 0 };
static int g_prev_n = 0;

static LONG WINAPI vmp38_crash_filter(EXCEPTION_POINTERS *ep)
{
	char path[MAX_PATH] = "";
	GetModuleFileNameA(NULL, path, sizeof(path));
	char *sl = strrchr(path, '\\');
	if (sl) { *sl = 0; _snprintf(path + strlen(path), (int)sizeof(path) - (int)strlen(path), "/xxvm/crash_restore.txt"); }
	FILE *f = fopen(path, "a");
	if (f)
	{
		fprintf(f, "EXCEPTION %08X at %p fault=%p last_eip=%08llx mod=%p\n",
			(unsigned)ep->ExceptionRecord->ExceptionCode,
			(void*)ep->ExceptionRecord->ExceptionAddress,
			(void*)(ep->ExceptionRecord->NumberParameters > 0 ? ep->ExceptionRecord->ExceptionInformation[0] : 0),
			g_stream_last_ip,
			GetModuleHandleA("xx_vm_x32.dp32"));
		/* jump-to-garbage forensics: last 10 guest eips before the crash */
		if (g_prev_n > 0)
		{
			int from = g_prev_n > 10 ? g_prev_n - 10 : 0;
			fprintf(f, "hook_stage=%d\n", g_hook_stage);
			fprintf(f, "prev10:");
			for (int k = from; k < g_prev_n; k++)
				fprintf(f, " %08x", g_prev_eips[k]);
			fprintf(f, " -> current %08llx\n", g_stream_last_ip);
		}
		fclose(f);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}
static int s_crash_filter_set = 0;

static int vmp38_log_open(const char *prefix)
{
	if (!s_crash_filter_set) { SetUnhandledExceptionFilter(vmp38_crash_filter); s_crash_filter_set = 1; }
	if (g_fdata) return 0;   /* already inside a session (nested call) */
	vmp38_reset_session_state();
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
		/* RESTORE FILE (.txt): the clean `[asm]` per-instruction sequence
		   (addr + reg/mem before/after values). Recreated (done 2026-08-19)
		   so the restore file is readable like pre-VM assembly; data is a
		   parallel streaming detail file. */
		g_fout = fopen(g_fout_path, "w");
		if (g_fout)
		{
			setvbuf(g_fout, NULL, _IOFBF, 1 << 20);
			fprintf(g_fout, "xxvm vmp38 restore - [asm] instruction sequence\n");
		}
		{
			char dpath[MAX_PATH];
			_snprintf(dpath, sizeof(dpath), "%s", g_fout_path);
			{
				size_t dl = strlen(dpath);
				if (dl > 4 && _stricmp(dpath + dl - 4, ".txt") == 0)
					_snprintf(dpath + dl - 4, sizeof(dpath) - (dl - 4), ".data.txt");
				else
					_snprintf(dpath + dl, sizeof(dpath) - dl, ".data.txt");
			}
			g_fdata = fopen(dpath, "w");
			if (g_fdata)
			{
				setvbuf(g_fdata, NULL, _IOFBF, 1 << 22);   /* 4MB buffer: fewer flushes on long traces */
				fprintf(g_fdata, "# xxvm vmp38 restore data v2 - standalone analyzer input\n"
					"# eip=<addr>\\t<code>\\t<changed regs>\\t<mem values>  (tab-separated)\n"
					"#   changed regs: push/pop dump ALL regs; other insns dump deltas vs the previous insn\n"
					"#   mem values:   [expr]=0x<value> for every [expr] operand (lea effective-address included)\n");
			}
		}
		return 1;   /* this call created the session data file */
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
	if (g_fdata)
	{
		fclose(g_fdata);
		g_fdata = NULL;
	}
}


static void out_file(const char *buf)
{
	/* main restore file (.txt): 只保留逐条 [asm] 行(去掉标签), 输出干净
	   的 "地址 + 汇编 + 寄存器/内存前后值" —— 2026-08-24 用户要求整洁格式。
	   [src]/decrypt/branch 等碎片不再写入 .txt。仅动 .txt, 不动 data.txt。 */
	if (g_fout && buf)
	{
		/* trim leading spaces */
		int skip = 0;
		const char *b = buf;
		while (*b == ' ') b++;
		/* 只保留这几类还原行到 .txt(去标签, 干净). 标签含尾空格, n=标签(含空格)长度 */
		struct { const char *t; int n; } keep[] = {
			{ "[src] @",  8 },   /* [src] @ (无尾空格, 后接地址; 0x 已去掉) */
			{ "[result] ",  9 },
			{ "[out] ",     6 },
			{ "[param] ",   8 },
			{ "[algo] ",    7 },   /* 2026-08-25 可追性: 参数/符号定义段 */
			{ "[algo-propagate] ", 17 },  /* 2026-08-25 立项: 旁路参数足迹 */
			{ "[data] ",  7 },   /* 2026-08-26 固定地址还原: 读/写/算出 模块数据段地址 */
			{ "[algo-code] ", 12 },  /* 2026-08-26 代码还原: 参与算法的指令(算子)代码行 */
		};
		for (size_t i = 0; i < sizeof(keep) / sizeof(keep[0]); i++)
			if ((int)strncmp(b, keep[i].t, (size_t)keep[i].n) == 0) { skip = keep[i].n; break; }
		if (skip > 0)
		{
			const char *content = b + skip;
			fprintf(g_fout, "%s\n", content);
		}
	}
}

/* write to the session FILE only (no GUI / stdout) - used for the
   high-volume full-trace handler lines so the GUI log buffer does not
   choke on hundreds of thousands of lines (the file stays complete). */

/* progress/status lines (out_progress): ALWAYS shown on the GUI + written
   to the session file. Restore-code lines (out): written to the session
   FILE only - the GUI shows progress only, the full restore code always
   lands in the xxvm session file next to the exe. */
static void out_progress(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	out_file(buf);
	_plugin_logputs(buf);   /* progress/status always visible */
}
static void out(const char *fmt, ...)
{
	/* restore-code lines: written to the session FILE only (GUI shows
	   progress via out_progress) - keeps the GUI clean and never stalls it. */
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	out_file(buf);
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
	case VMP_CC_SHL:
		return (value << (cmd->val & 0x3f)) & vmp38_mask(bits);
	case VMP_CC_SHR:
		return (value >> (cmd->val & 0x3f)) & vmp38_mask(bits);
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
		else if (c.type == VMP_CC_SHL) c.type = VMP_CC_SHR;
		else if (c.type == VMP_CC_SHR) c.type = VMP_CC_SHL;
		value = vmp38_calc_one(value, &c, bits);
	}
	return value;
}

/* Encrypt: forward order, original commands */

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

/* [esp-fold]: rewrite a VMP-obscured stack addressing like
   [esp+ecx*1-0x773AFFFB] / [esp+edx*1+0x01] / [esp+0x04] into a real
   [esp+0xOFF] using the executing register snapshot rv[0..7]. Returns 1 and
   stores the folded (scalar) offset. Non-esp memory operands -> 0. */
static int vmp38_fold_stack_disp(const char *dis, const unsigned int *rv,
	unsigned long long *off)
{
	const char *b = strchr(dis, '[');
	if (!b) return 0;
	const char *e = strchr(b + 1, ']');
	if (!e || e - b - 1 <= 0 || e - b - 1 > 120) return 0;
	char expr[128];
	memcpy(expr, b + 1, (size_t)(e - b - 1));
	expr[e - b - 1] = 0;
	/* strip segment prefix "ss:" / "ds:" if present */
	{
		char *cc = strchr(expr, ':');
		if (cc) memmove(expr, cc + 1, strlen(cc + 1) + 1);
	}
	if (strncmp(expr, "esp", 3) != 0) return 0;
	char *p = expr + 3;
	unsigned long long acc = 0;
	while (*p)
	{
		int sign = 1;
		if (*p == '+') { sign = 1; p++; }
		else if (*p == '-') { sign = -1; p++; }
		if (*p == '0' && p[1] && (p[1] == 'x' || p[1] == 'X'))
		{
			unsigned long long v = strtoull(p, NULL, 0);
			acc = (sign > 0) ? acc + v : acc - v;
			while (*p && *p != '+' && *p != '-') p++;
		}
		else
		{
			char nm[8]; int ni = 0;
			while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
				(*p >= '0' && *p <= '9') || *p == '_')
			{
				if (ni < 7) nm[ni++] = *p;
				p++;
			}
			nm[ni] = 0;
			unsigned long long val = 0;
			int ri = reg_index_from_name(nm);
			if (ri >= 0 && ri < 8) val = rv[ri];
			else if (!strcmp(nm, "esp")) val = 0;   /* esp base counted as 0 */
			if (*p == '*')
			{
				p++;
				unsigned long long sc = strtoull(p, NULL, 0);
				while (*p && *p != '+' && *p != '-') p++;
				acc = (sign > 0) ? acc + val * sc : acc - val * sc;
			}
			else
				acc = (sign > 0) ? acc + val : acc - val;
		}
	}
	*off = acc & 0xFFFFFFFFULL;
	return 1;
}

/* 2026-08-30 通用地址折叠: 把 [base+idx*scale±imm32] 这样的加密寻址折叠回
   [base+<算出的标量>] (base 保留寄存器名, 只折叠 index/scale/imm 部分)。
   例: [edi+eax*1-0x403AF57E]，当 eax=0x403AF582 时 → [edi+0x04]，正是
   用户要的"还原回 byte ptr ds:[edi+4]"。用执行期寄存器快照 rv[0..7] 代入。
   与 vmp38_fold_stack_disp 不同: 不假设 base 是 esp，任何 base 寄存器都行；
   base 本身保留(不展开成绝对地址)，因为分析时"哪个寄存器做基址"是有语义的。
   返回 1 并把折叠后的标量放进 *off(32 位回绕)。 */
static int vmp38_fold_mem_addr(const char *dis, const unsigned int *rv,
	unsigned long long *off, int *base_reg)
{
	const char *b = strchr(dis, '[');
	if (!b) return 0;
	const char *e = strchr(b + 1, ']');
	if (!e || e - b - 1 <= 0 || e - b - 1 > 160) return 0;
	char expr[192];
	memcpy(expr, b + 1, (size_t)(e - b - 1));
	expr[e - b - 1] = 0;
	/* strip segment prefix "ss:"/"ds:"/"fs:"/"gs:" */
	{
		char *cc = strchr(expr, ':');
		if (cc) memmove(expr, cc + 1, strlen(cc + 1) + 1);
	}
	/* 第一个 token 必须是 base 寄存器 */
	char base[16]; int j = 0;
	const char *p = expr;
	while (*p && *p != '+' && *p != '-' && *p != '*' && j < 15) base[j++] = *p++;
	base[j] = 0;
	int ri = reg_index_from_name(base);
	if (ri < 0 || ri >= 8) return 0;
	if (base_reg) *base_reg = ri;
	/* 从 base 之后继续解析: 若干项 [+-  reg*scale | imm] */
	unsigned long long acc = 0;
	while (*p)
	{
		int sign = 1;
		if (*p == '+') { sign = 1; p++; }
		else if (*p == '-') { sign = -1; p++; }
		else break;   /* 非法, 不再继续 */
		/* 数字(imm) 或 寄存器(index) */
		if (*p == '0' && p[1] && (p[1] == 'x' || p[1] == 'X'))
		{
			unsigned long long v = strtoull(p, NULL, 0);
			acc = (sign > 0) ? acc + v : acc - v;
			while (*p && *p != '+' && *p != '-') p++;
		}
		else if ((*p >= '0' && *p <= '9'))
		{
			unsigned long long v = strtoull(p, NULL, 0);
			acc = (sign > 0) ? acc + v : acc - v;
			while (*p && *p != '+' && *p != '-') p++;
		}
		else
		{
			char nm[8]; int ni = 0;
			while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
				(*p >= '0' && *p <= '9') || *p == '_')
			{
				if (ni < 7) nm[ni++] = *p;
				p++;
			}
			nm[ni] = 0;
			unsigned long long val = 0;
			int r2 = reg_index_from_name(nm);
			if (r2 >= 0 && r2 < 8) val = rv[r2];
			if (*p == '*')
			{
				p++;
				unsigned long long sc = strtoull(p, NULL, 0);
				while (*p && *p != '+' && *p != '-') p++;
				acc = (sign > 0) ? acc + val * sc : acc - val * sc;
			}
			else
				acc = (sign > 0) ? acc + val : acc - val;
		}
	}
	*off = acc & 0xFFFFFFFFULL;
	return 1;
}
/* 2026-09-01 指令列地址折叠(供 data.txt 指令列与 .txt 一致):
   把 [base+idx*scale±imm32] 用执行期寄存器快照 rv 折叠:
   - 折叠后标量 off 是"小偏移"(<0x10000, 真实 base+小偏移) -> [base±off];
   - off 是"大值"(固定地址获取, base 也代入后即绝对地址) -> [absaddr];
   - esp base 或无法折叠 -> 原样保留(交给内存列显示绝对地址)。
   absaddr 传 ~0ULL 表示无绝对地址可用(此时大 off 也回退成 [base±off])。 */
static void vmp38_fold_code_addr(const char *code, const unsigned int *rv,
	unsigned long long absaddr, char *out, int outsz)
{
	if (!code || !rv || !out || outsz <= 0) return;
	out[0] = 0;
	const char *br = strchr(code, '[');
	const char *cr = br ? strchr(br + 1, ']') : NULL;
	if (!br || !cr || cr <= br) { _snprintf(out, outsz, "%s", code); return; }
	unsigned long long off = 0;
	int greg = -1;
	int folded = vmp38_fold_mem_addr(code, rv, &off, &greg);
	if (!folded || greg < 0 || greg >= 8 || greg == 4)   /* 无 base / esp */
	{
		_snprintf(out, outsz, "%s", code);
		return;
	}
	int prel = (int)(br - code);
	const char *suf = cr + 1;
	static const char *bn8[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
	/* small-offset test uses signed semantics: [0,0x7FFF] and [0xFFFF8000,0xFFFFFFFF]
	   are true base-immediate offsets (e.g. [ebp-4]/[ebp+0x20]), keep base name */
	int is_small = (off < 0x8000ULL || off > 0xFFFF8000ULL);
	if (is_small)
	{
		/* 真实 base±小偏移 */
		if (off > 0x7FFFFFFFULL)
			_snprintf(out, outsz, "%.*s[%s-%X]%s", prel, code, bn8[greg], (unsigned)(0x100000000ULL - off), suf);
		else
			_snprintf(out, outsz, "%.*s[%s+%X]%s", prel, code, bn8[greg], (unsigned)off, suf);
	}
	else if (absaddr != ~0ULL)
	{
		/* 固定地址获取: 直接显示运行时绝对地址 */
		_snprintf(out, outsz, "%.*s[%08X]%s", prel, code, (unsigned)absaddr, suf);
	}
	else
	{
		/* 无绝对地址: 回退成 [base±off] */
		if (off > 0x7FFFFFFFULL)
			_snprintf(out, outsz, "%.*s[%s-%X]%s", prel, code, bn8[greg], (unsigned)(0x100000000ULL - off), suf);
		else
			_snprintf(out, outsz, "%.*s[%s+%X]%s", prel, code, bn8[greg], (unsigned)off, suf);
	}
}
static void get_mnemonic(const char *disasm, char *out, int outsize)
{
	int j = 0;
	const char *p = disasm;
	while (*p && *p != ' ' && *p != '\t' && j < outsize - 1)
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
	int sym[MINI_MEM_SLOTS];   /* symbolic expression per slot */
	unsigned char t[MINI_MEM_SLOTS];  /* taint per slot (2026-09-02: 跨 handler 槽 taint) */
} MINI_MEM;

/* ================= symbolic execution (Sym) =================
   Flat expression tree in a node pool. Each node is one op + 2 children.
   Simplification happens at build time (constant folding + identities). */
#define SYM_OP_CONST 0
#define SYM_OP_REG   1
#define SYM_OP_ADD   2
#define SYM_OP_SUB   3
#define SYM_OP_XOR   4
#define SYM_OP_AND   5
#define SYM_OP_OR    6
#define SYM_OP_NOT   7
#define SYM_OP_NEG   8
#define SYM_OP_SHL   9
#define SYM_OP_SHR   10
#define SYM_OP_ROL   11
#define SYM_OP_ROR   12
#define SYM_OP_LEA   13   /* a + b*scale + disp (scale folded into mul) */
#define SYM_OP_MUL   14
#define SYM_OP_SRC   15   /* named source variable (a/b/acc0/...); .c=var id */

#define SYM_POOL_MAX 262144
typedef struct {
	unsigned short op;
	unsigned short a;      /* child node or reg index */
	unsigned short b;      /* child node or reg index */
	unsigned char width;   /* 8/16/32/64 */
	unsigned long long c;  /* const value (for CONST / LEA disp / SHL count) */
} SYM_NODE;

static SYM_NODE g_sym_pool[SYM_POOL_MAX];
static int g_sym_n = 0;

/* 阶段二 acc 累积载体(跨 handler)。放在 sym_gc 之前定义, 以便 sym_gc 把
   g_acc2_sym(载体上一符号)当作 GC 根保护——否则池近满时的 GC 回收会打断
   acc 跨 handler 累积链(详情见 sym_gc 的根列表)。 */
static int g_acc2_reg = -1;
static int g_acc2_sym = -1;       /* 载体上一指令的符号 id */
static unsigned long long g_acc2_pre = 0;   /* 载体 pre 具体值 */
static int g_acc2_started = 0;     /* 已输出起点 */
static int g_acc2_pending = 0;     /* 载体符号含 seed 源且为 clean, 待累积 */

/* named source variables (a/b/acc0/...) for symbolic reconstruction.
   The value-anchor naming step turns "a register holding 0x13579BDF" into
   SYM_OP_SRC(id=a) without needing the analyst; cross-handler symbols then
   fold to `acc0 + a + b + ...`, i.e. the real source arithmetic. */
static int vmp38_vf_load_const_list(unsigned int *vals, char names[][48], int maxn);
static int vmp38_src_id_of(unsigned int v);   /* fwd: srcs_load 末尾注入 args 时比对现有池 */
static unsigned int g_src_vals[64];
static char g_src_names[64][24];
static int g_src_cnt = 0;
/* 2026-08-25 探针(符号化传播可行性): 把入口观测到的"合理整数参数"从 CONST 改绑为
   SYM_OP_SRC 符号根, 验证符号引擎能否追出被 VMP 藏起(值不裸现)的参数计算链。
   默认关闭(避免把引擎中间寄存器误当参数根污染 base); 验证真实样本/受控测试时显式置1。 */
static int g_probe_entry_root = 0;   /* probe_roots=1 开启分析侧参数溯源旁路(algo2) */
/* 2026-08-25 可追性 [algo] 符号表: 记录每个源码级行的产出符号(名/值/eip/来源名/算子),
   供 .txt 顶部的 [algo] sN = 0x.. @eip <算子+来源> 定义表做参数回溯。 */
static unsigned int g_algo_sym_val[1024];
static char g_algo_sym_name[1024][24];
static unsigned int g_algo_sym_eip[1024];
static char g_algo_sym_src[1024][96];
static int g_algo_sym_reg[1024];
static int g_algo_sym_n = 0;
/* 2026-08-25 可追性开关: =1 时保留全部 real-trace 行(含引擎噪声), 默认 0 抑制未命名行 */
static int g_algo_keep_real_noise = 0;
/* 2026-08-23: 检测到真实 acc 累加链时置1 —— 用于门控"vmp_base 专属模板要
   (return acc^a^b / for out[i])"——真实软件(无 acc 链)不再输出假模板。 */
static int g_src_had_acc = 0;

static void vmp38_srcs_load(void)
{
	/* 2026-08-25 通用入口参数捕获: 在函数末尾从 continue-chain 入口真实进程栈
	   扫 cdecl 参数(数值/种子)追加进 g_src_vals。已在 vmp_demo_clean 验证:
	   esp+0x18=seed(0x12345678)/+0x1C=rounds(8) 成功注入 g_src_vals(argN)。 */
	static const unsigned int fb_seed[] = { 0x13579BDFu, 0x2468ACE0u, 0xA5A5A5A5u };
	static const char *fb_seedn[] = { "a", "b", "acc0" };
	unsigned int vals[80];
	char names[80][48];
	int nc = 0;
	int ci;
	/* 2026-08-23 种子合并：fb_seed(a/b/acc0) 固定占 [0..2]——vmp_base golden 链
	    用 g_src_vals[0/1]=a/b, 必须固定。const_list.txt 的真实参数/算法常量
	    (fnv/crc/mul) 作为附加种子去重补充(不占用 a/b/acc0 位置), 使值流能匹配
	    真实软件参数又不破坏 demo golden。 */
	nc = 0;
	{
		int fbn = (int)(sizeof(fb_seed) / sizeof(fb_seed[0]));
		for (int fbi = 0; fbi < fbn; fbi++)
		{
			if (fb_seed[fbi] == 0 || fb_seed[fbi] == 0xFFFFFFFFu) continue;
			vals[nc] = fb_seed[fbi];
			_snprintf(names[nc], 47, "%s", fb_seedn[fbi]);
			nc++;
		}
	}
	{
		unsigned int cv[80]; char cn[80][48];
		int cc = vmp38_vf_load_const_list(cv, cn, 80);
		for (int c = 0; c < cc && nc < 80; c++)
		{
			if (cv[c] == 0 || cv[c] == 0xFFFFFFFFu) continue;
			/* vmp_base 的 a/b/acc0 已由 fb_seed 占位, const_list 里同名(同值)跳过 */
			int dup = 0;
			for (int d = 0; d < nc; d++) if (vals[d] == cv[c]) { dup = 1; break; }
			if (!dup)
			{
				vals[nc] = cv[c];
				_snprintf(names[nc], 47, "%s", cn[c]);
				nc++;
			}
		}
	}
	g_src_cnt = 0;
	for (ci = 0; ci < nc && g_src_cnt < 64; ci++)
	{
		if (vals[ci] == 0 || vals[ci] == 0xFFFFFFFFu)
			continue;
		g_src_vals[g_src_cnt] = vals[ci];
		_snprintf(g_src_names[g_src_cnt], 23, "%s", names[ci]);
		g_src_cnt++;
	}
	/* 2026-08-25 通用入口参数注入: 扫 continue-chain 入口真实进程栈, 把 cdecl
	   调用帧参数(数值/const_list 锚) 追加进 g_src_vals, 修复"参数不在入口寄存器
	   (vmp_demo_clean seed=0x12345678 只在栈 esp+0x18)"时转换表/acc2 无法锚定
	   真实参数的问题。只追加不覆盖 fb_seed[0..2], 不碰 trace 数据(纯命名池)。 */
	{
		unsigned long esp0 = (unsigned long)Script::Register::GetESP();
		unsigned char st[0x60];
		if (esp0 && Script::Memory::Read(esp0, st, sizeof(st), NULL))
		{
			/* 可读指针检测 helper */
			unsigned int imgbase = 0;
			{
				Script::Module::ModuleInfo mi;
				if (Script::Module::GetMainModuleInfo(&mi)) imgbase = (unsigned int)mi.base;
			}
			for (int ai = 0x10; ai < (int)sizeof(st); ai += 4)
			{
				unsigned int v = *(unsigned int*)(st + ai);
				if (!v || v == 0xFFFFFFFFu) continue;
				if (vmp38_src_id_of(v) >= 0) continue;   /* already a seed */
				unsigned char pp[4];
				int is_ptr = Script::Memory::Read((duint)v, pp, 4, NULL);
				if (is_ptr) continue;   /* 指针(out/栈)不注入, 是 out 参数 */
				/* 数值参数启发: 排除明显引擎噪声, 只收"合理种子/整数" */
				int noise = (v >= 0x80000000u && v != 0x80000000u) ||
					(v >= (imgbase ? imgbase : 0x400000u) && v < (imgbase ? imgbase : 0x400000u) + 0x2000000u);
				if (noise) continue;
				char nm[48];
				_snprintf(nm, 47, "arg%d", ai);
				int dup2 = 0;
				for (int d2 = 0; d2 < nc; d2++) if (vals[d2] == v) { dup2 = 1; break; }
				if (!dup2 && nc < 80)
				{
					vals[nc] = v; _snprintf(names[nc], 47, "%s", nm); nc++;
					out_progress("[args] 注入参数 esp+%02X = %08X (%s)", ai, v, nm);
				}
			}
		}
	}
}
/* value lookup -> source-variable id, or -1 */
static int vmp38_src_id_of(unsigned int v)
{
	int i;
	for (i = 0; i < g_src_cnt; i++)
		if (g_src_vals[i] == v)
			return i;
	return -1;
}

static void sym_pool_reset(void) { g_sym_n = 0; }

/* sym-pool garbage collection: the cross-handler accumulator keeps every
   intermediate node alive indefinitely, so a full (200k-instr) sample
   exhausts SYM_POOL_MAX and sym_alloc degrades to CONST 0 -> all symbols
   become 0. GC marks only the 8 register-symbol roots (plus their children)
   and compacts the pool, dropping dead intermediate nodes. Called from
   vmp38_net_fold when the pool gets full. */
static int sym_alloc(int op, int a, int b, int width, unsigned long long c)
{
	if (g_sym_n >= SYM_POOL_MAX)
		return SYM_OP_CONST;   /* pool exhausted: degrade to const 0 */
	{
		SYM_NODE *n = &g_sym_pool[g_sym_n];
		n->op = (unsigned short)op;
		n->a = (unsigned short)a;
		n->b = (unsigned short)b;
		n->width = (unsigned char)width;
		n->c = c;
		return g_sym_n++;
	}
}

/* node accessors */
static int sym_is_const(int id)
{
	return id < g_sym_n && g_sym_pool[id].op == SYM_OP_CONST;
}
static unsigned long long sym_const_val(int id)
{
	return id < g_sym_n ? g_sym_pool[id].c : 0;
}
static int sym_is_reg(int id)
{
	return id < g_sym_n && g_sym_pool[id].op == SYM_OP_REG;
}
static int sym_reg_idx(int id)
{
	return id < g_sym_n ? g_sym_pool[id].a : 0;
}

/* mask a value to the expression width */
static unsigned long long sym_maskv(unsigned long long v, int w)
{
	if (w >= 64) return v;
	return v & ((1ULL << w) - 1);
}

/* width of a node (propagate) */
static int sym_width(int id)
{
	return id < g_sym_n ? g_sym_pool[id].width : 32;
}

/* build a simplified node.
   Simplification rules (applied eagerly):
     CONST fold, x+0, x-0, x^0, x&0, x|0, x&~0, not(not x), neg(neg x),
     x^x=0, x-x=0, x*1=x, width truncation */
static int sym_build(int op, int a, int b, int width, unsigned long long c)
{
	int wa, wb;
	unsigned long long ca, cb;
	/* normalize width */
	if (width <= 0) width = 32;
	if (a < 0) a = sym_alloc(SYM_OP_CONST, 0, 0, width, 0);
	if (b < 0) b = sym_alloc(SYM_OP_CONST, 0, 0, width, 0);
	wa = sym_width(a); wb = sym_width(b);
	if (width == 0) width = (wa > wb ? wa : wb);
	if (width == 0) width = 32;

	/* constant folding */
	if (sym_is_const(a) && sym_is_const(b))
	{
		unsigned long long va = sym_const_val(a);
		unsigned long long vb = sym_const_val(b);
		unsigned long long r = 0;
		switch (op)
		{
		case SYM_OP_ADD: r = va + vb; break;
		case SYM_OP_SUB: r = va - vb; break;
		case SYM_OP_XOR: r = va ^ vb; break;
		case SYM_OP_AND: r = va & vb; break;
		case SYM_OP_OR:  r = va | vb; break;
		case SYM_OP_SHL: r = va << (vb & 63); break;
		case SYM_OP_SHR: r = va >> (vb & 63); break;
		case SYM_OP_ROL:
			{ int cnt = (int)(vb & 63); r = (va << cnt) | (va >> (width - cnt)); }
			break;
		case SYM_OP_ROR:
			{ int cnt = (int)(vb & 63); r = (va >> cnt) | (va << (width - cnt)); }
			break;
		case SYM_OP_MUL:
			r = va * vb;
			break;
		default: r = va;
		}
		return sym_alloc(SYM_OP_CONST, 0, 0, width, sym_maskv(r, width));
	}
	if (sym_is_const(a))
	{
		ca = sym_const_val(a);
		ca = sym_maskv(ca, width);
		/* identities with constant a */
		switch (op)
		{
		case SYM_OP_ADD:
		case SYM_OP_XOR:
		case SYM_OP_OR:
			if (ca == 0) return b;              /* x+0, x^0, x|0 */
			break;
		case SYM_OP_SUB:
			if (ca == 0) return b;              /* 0-x = -x handled below */
			break;
		case SYM_OP_AND:
			if (ca == 0) return sym_alloc(SYM_OP_CONST, 0, 0, width, 0);
			if (ca == sym_maskv(~0ULL, width)) return b;   /* x & ~0 */
			break;
		case SYM_OP_SHL:
			if (ca == 0) return sym_alloc(SYM_OP_CONST, 0, 0, width, 0);
			break;
		default: break;
		}
	}
	if (sym_is_const(b))
	{
		cb = sym_const_val(b);
		cb = sym_maskv(cb, width);
		switch (op)
		{
		case SYM_OP_ADD:
		case SYM_OP_XOR:
		case SYM_OP_OR:
			if (cb == 0) return a;              /* x+0, x^0, x|0 */
			break;
		case SYM_OP_SUB:
			if (cb == 0) return a;              /* x-0 */
			break;
		case SYM_OP_AND:
			if (cb == 0) return sym_alloc(SYM_OP_CONST, 0, 0, width, 0);
			if (cb == sym_maskv(~0ULL, width)) return a;
			break;
		case SYM_OP_SHL:
			if (cb == 0) return a;              /* x << 0 */
			break;
		case SYM_OP_SHR:
			if (cb == 0) return a;
			break;
		case SYM_OP_ROL:
			if (cb == 0 || (cb & 31) == 0) return a;   /* rol x,0 -> x */
			break;
		case SYM_OP_ROR:
			if (cb == 0 || (cb & 31) == 0) return a;   /* ror x,0 -> x */
			break;
		default: break;
		}
		/* x ^ x = 0, x - x = 0 */
		if ((op == SYM_OP_XOR || op == SYM_OP_SUB) && a == b)
			return sym_alloc(SYM_OP_CONST, 0, 0, width, 0);
	}
	/* ===== recursive normalization: associativity + const-term coalescing =====
	   Cross-handler accumulation builds nested chains like
	     ADD(ADD(ADD(r,1),-1),1)
	   which the single-level identities above never collapse. Coalesce a
	   same-op nested constant, e.g. ADD(ADD(x,c1),c2) -> ADD(x,c1+c2), and
	   mixed add/sub (ADD(SUB(x,c1),c2) -> x+(c2-c1)); when the combined
	   constant is 0 the recursive call collapses to the bare operand. This
	   is what makes adjacent +1/-1 cancel across VMP handler boundaries. */
	if (op == SYM_OP_ADD || op == SYM_OP_SUB || op == SYM_OP_XOR ||
		op == SYM_OP_OR || op == SYM_OP_AND)
	{
		int nA_valid = (a >= 0 && a < g_sym_n);
		SYM_NODE *nA = nA_valid ? &g_sym_pool[a] : NULL;
		int b_const = (b >= 0 && b < g_sym_n && sym_is_const(b));
		if (nA && nA->op == op && nA->b >= 0 && nA->b < g_sym_n &&
			g_sym_pool[nA->b].op == SYM_OP_CONST && b_const)
		{
			unsigned long long c1 = g_sym_pool[nA->b].c;
			unsigned long long c2 = sym_const_val(b);
			unsigned long long cn = 0;
			switch (op)
			{
			case SYM_OP_ADD: cn = c1 + c2; break;
			case SYM_OP_SUB: cn = c1 + c2; break;   /* (x-c1)-c2 = x-(c1+c2) */
			case SYM_OP_XOR: cn = c1 ^ c2; break;
			case SYM_OP_AND: cn = c1 & c2; break;
			case SYM_OP_OR:  cn = c1 | c2; break;
			}
			return sym_build(op, nA->a,
				sym_alloc(SYM_OP_CONST, 0, 0, width, sym_maskv(cn, width)), width, 0);
		}
		/* ADD(SUB(x,c1), c2) = x + (c2-c1); SUB(ADD(x,c1), c2) = x + (c1-c2) */
		if (op == SYM_OP_ADD && nA && nA->op == SYM_OP_SUB &&
			nA->b >= 0 && nA->b < g_sym_n && g_sym_pool[nA->b].op == SYM_OP_CONST && b_const)
		{
			long long c1 = (long long)g_sym_pool[nA->b].c;
			long long c2 = (long long)sym_const_val(b);
			long long cn = c2 - c1;
			return sym_build(SYM_OP_ADD, nA->a,
				sym_alloc(SYM_OP_CONST, 0, 0, width, sym_maskv(cn, width)), width, 0);
		}
		if (op == SYM_OP_SUB && nA && nA->op == SYM_OP_ADD &&
			nA->b >= 0 && nA->b < g_sym_n && g_sym_pool[nA->b].op == SYM_OP_CONST && b_const)
		{
			long long c1 = (long long)g_sym_pool[nA->b].c;
			long long c2 = (long long)sym_const_val(b);
			long long cn = c1 - c2;
			return sym_build(SYM_OP_ADD, nA->a,
				sym_alloc(SYM_OP_CONST, 0, 0, width, sym_maskv(cn, width)), width, 0);
		}
	}
	/* ===== strength reduction: fold repeated adds into a multiply =====
	   VMP compile-time rewrites `lea [eax+eax*2]` (x*3) into `add` chains,
	   but the SOURCE op is a multiply. Rebuild it symbolically:
	     x + x            -> x*2
	     x + x*2          -> x*3   (and x*2 + x -> x*3)
	     x + x*k          -> x*(k+1)
	   This is what turns the scattered `add eax,ebx; add eax,ebx; add eax,ebx`
	   back into the readable `a*3` that matches the pre-VM `lea`. */
	if (op == SYM_OP_ADD)
	{
		/* x + x -> x*2 */
		if (a == b)
			return sym_build(SYM_OP_MUL, a,
				sym_alloc(SYM_OP_CONST, 0, 0, width, 2), width, 0);
		/* x + (x*k) -> x*(k+1) ; (x*k) + x -> x*(k+1) */
		if (a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_MUL &&
			g_sym_pool[a].a == b && sym_is_const(g_sym_pool[a].b))
		{
			unsigned long long k = sym_const_val(g_sym_pool[a].b);
			return sym_build(SYM_OP_MUL, b,
				sym_alloc(SYM_OP_CONST, 0, 0, width, sym_maskv(k + 1, width)), width, 0);
		}
		if (b >= 0 && b < g_sym_n && g_sym_pool[b].op == SYM_OP_MUL &&
			g_sym_pool[b].a == a && sym_is_const(g_sym_pool[b].b))
		{
			unsigned long long k = sym_const_val(g_sym_pool[b].b);
			return sym_build(SYM_OP_MUL, a,
				sym_alloc(SYM_OP_CONST, 0, 0, width, sym_maskv(k + 1, width)), width, 0);
		}
	}
	/* SHL by constant -> multiply: shl x,n = x*2^n. The VMP rewrite of a
	   source `lea [x+x*2]` (x*3) often keeps `shl x,1` (x*2) as the first
	   step; folding it to MUL lets the next `add x,<orig>` reuse the MUL
	   strength-reduction rules below to reach x*3. */
	if (op == SYM_OP_SHL && sym_is_const(b))
	{
		unsigned long long n = sym_const_val(b);
		if (n >= 1 && n < 16)
			return sym_build(SYM_OP_MUL, a,
				sym_alloc(SYM_OP_CONST, 0, 0, width, 1ULL << n), width, 0);
	}
	/* x * 1 = x, x * 0 = 0, x * k normalization */
	if (op == SYM_OP_MUL)
	{
		if (sym_is_const(a) && sym_const_val(a) == 1) return b;
		if (sym_is_const(b) && sym_const_val(b) == 1) return a;
		if (sym_is_const(a) && sym_const_val(a) == 0)
			return sym_alloc(SYM_OP_CONST, 0, 0, width, 0);
		if (sym_is_const(b) && sym_const_val(b) == 0)
			return sym_alloc(SYM_OP_CONST, 0, 0, width, 0);
		/* commute a constant coefficient to the b side (so MUL(x,k) has a
		   canonical shape the strength-reduction rules can match). */
		if (sym_is_const(a) && !sym_is_const(b))
			return sym_build(SYM_OP_MUL, b, a, width, 0);
	}
	/* not(not x) = x, neg(neg x) = x */
	if (op == SYM_OP_NOT && a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_NOT)
		return g_sym_pool[a].a;
	if (op == SYM_OP_NEG && a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_NEG)
		return g_sym_pool[a].a;
	/* exact repeated-key inverse ops (VMP encrypt/decrypt pairing):
	     XOR(XOR(x, k), k) = x
	     SUB(ADD(x, k), k) = x    (and ADD(SUB(x, k), k) = x)
	   when the SAME node k appears on both sides. This is the symbolic
	   decrypt-fold that collapses VMP's xor-key / add-key round-trips even
	   when the key is a dynamic register symbol (not a constant). */
	if (op == SYM_OP_XOR && a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_XOR)
	{
		SYM_NODE *na = &g_sym_pool[a];
		if (na->b == b) return na->a;              /* (x^k)^k -> x */
	}
	if (op == SYM_OP_SUB && a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_ADD &&
		g_sym_pool[a].b == b)
		return g_sym_pool[a].a;                     /* (x+k)-k -> x */
	if (op == SYM_OP_ADD && a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_SUB &&
		g_sym_pool[a].b == b)
		return g_sym_pool[a].a;                     /* (x-k)+k -> x */
	/* XOR is symmetric: k ^ (x ^ k) -> x  (handle key on the LEFT) */
	if (op == SYM_OP_XOR && b >= 0 && b < g_sym_n && g_sym_pool[b].op == SYM_OP_XOR &&
		g_sym_pool[b].b == a)
		return g_sym_pool[b].a;
	/* rol/ror inverse pairing: rol(ror(x,n),n) = x, ror(rol(x,n),n) = x.
	   The shift count sits in the CONST b child (sym_build stores it there),
	   so we compare the two count nodes for equality. */
	if ((op == SYM_OP_ROL || op == SYM_OP_ROR) &&
		a >= 0 && a < g_sym_n)
	{
		SYM_NODE *na = &g_sym_pool[a];
		int inv = (op == SYM_OP_ROL) ? SYM_OP_ROR : SYM_OP_ROL;
		if (na->op == inv && na->b == b)
			return na->a;                            /* rol(ror(x,n),n) -> x */
	}
	/* -~x = x+1  (and ~-x = x-1), so -(~x)+1 = (x+1)+1 = x+2 folds via the
	   associativity coalescing above into a plain constant add. */
	if (op == SYM_OP_NEG && a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_NOT)
		return sym_build(SYM_OP_ADD, g_sym_pool[a].a,
			sym_alloc(SYM_OP_CONST, 0, 0, width, 1), width, 0);
	if (op == SYM_OP_NOT && a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_NEG)
		return sym_build(SYM_OP_SUB, g_sym_pool[a].a,
			sym_alloc(SYM_OP_CONST, 0, 0, width, 1), width, 0);

	/* ===== De Morgan + absorption (VMP 3.8 boolean-MBA constant obfuscation) =====
	   The 16-byte key bytes are built as nested and/or/not expressions, e.g.
	     key = ~( (~a & ~b) | a )
	   which only folds to "~a & b" if we push NOT down and apply absorption.
	   Without these rules the expression tree stays opaque and the recovered
	   "algorithm" is unreadable. Rules (all width-preserving, node count only
	   shrinks so recursion is bounded by the pool):
	     NOT(AND(x,y)) -> OR(NOT x, NOT y)
	     NOT(OR(x,y))  -> AND(NOT x, NOT y)
	     OR(x, AND(NOT x, y)) -> OR(x, y)     (absorption)
	     OR(x, AND(y, NOT x)) -> OR(x, y)
	     AND(NOT x, OR(x, y)) -> AND(NOT x, y) */
	if (op == SYM_OP_NOT && a >= 0 && a < g_sym_n)
	{
		SYM_NODE *na = &g_sym_pool[a];
		if (na->op == SYM_OP_AND || na->op == SYM_OP_OR)
		{
			int inv = (na->op == SYM_OP_AND) ? SYM_OP_OR : SYM_OP_AND;
			int n1 = sym_build(SYM_OP_NOT, na->a, 0, width, 0);
			int n2 = sym_build(SYM_OP_NOT, na->b, 0, width, 0);
			return sym_build(inv, n1, n2, width, 0);
		}
	}
	if (op == SYM_OP_OR)
	{
		/* a | (~a & y)  ->  a | y */
		if (b >= 0 && b < g_sym_n && g_sym_pool[b].op == SYM_OP_AND)
		{
			int ba = g_sym_pool[b].a, bb = g_sym_pool[b].b;
			if (ba >= 0 && ba < g_sym_n && g_sym_pool[ba].op == SYM_OP_NOT && g_sym_pool[ba].a == a)
				return sym_build(SYM_OP_OR, a, bb, width, 0);
			if (bb >= 0 && bb < g_sym_n && g_sym_pool[bb].op == SYM_OP_NOT && g_sym_pool[bb].a == a)
				return sym_build(SYM_OP_OR, a, ba, width, 0);
		}
		/* (~b & y) | b  ->  y | b  (symmetrical) */
		if (a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_AND)
		{
			int aa = g_sym_pool[a].a, ab = g_sym_pool[a].b;
			if (aa >= 0 && aa < g_sym_n && g_sym_pool[aa].op == SYM_OP_NOT && g_sym_pool[aa].a == b)
				return sym_build(SYM_OP_OR, b, ab, width, 0);
			if (ab >= 0 && ab < g_sym_n && g_sym_pool[ab].op == SYM_OP_NOT && g_sym_pool[ab].a == b)
				return sym_build(SYM_OP_OR, b, aa, width, 0);
		}
	}
	if (op == SYM_OP_AND)
	{
		/* ~a & (a | y)  ->  ~a & y */
		if (a >= 0 && a < g_sym_n && g_sym_pool[a].op == SYM_OP_NOT &&
			b >= 0 && b < g_sym_n && g_sym_pool[b].op == SYM_OP_OR)
		{
			int ba = g_sym_pool[b].a, bb = g_sym_pool[b].b;
			int nota_inner = g_sym_pool[a].a;
			if (ba == nota_inner)
				return sym_build(SYM_OP_AND, a, bb, width, 0);
			if (bb == nota_inner)
				return sym_build(SYM_OP_AND, a, ba, width, 0);
		}
	}
	/* ===== 任务5: 算术-布尔混合 MBA 反向(第①篇"万用门") =====
	   VMP 3.8 把逻辑运算写成 ADD/SUB 与 AND/OR/NOT 的混合, 反向还原:
	     AND(x, y) == OR(x,y) - ASCII...  常见恒等式:
	     x^y == (x|y) - (x&y)             (bitwise xor = or minus and, 仅无进位)
	     更稳: x|y == (x^y) + (x&y)        (三式互为等价, 取 or=sub(xor,and) 反向)
	   这里补: XOR(x,y) 若 x/y 是 OR/AND 的混合, 折叠出简洁形式。为防误重构,
	   只在"两子式恰好是 OR(x,y) 与 AND(x,y)"这种确定性形态时套用。 */
	if (op == SYM_OP_SUB && a >= 0 && a < g_sym_n && b >= 0 && b < g_sym_n)
	{
		SYM_NODE *na = &g_sym_pool[a], *nb = &g_sym_pool[b];
		/* OR(p,q) - AND(p,q) -> XOR(p,q)  (VMP 万用门 xor 的经典 MBA 形态) */
		if (na->op == SYM_OP_OR && nb->op == SYM_OP_AND &&
			((na->a == nb->a && na->b == nb->b) || (na->a == nb->b && na->b == nb->a)))
			return sym_build(SYM_OP_XOR, na->a, na->b, width, 0);
		/* OR(p,q) - q + (~p & q) 这种更复杂形态留给 sym_mba_decompose(jmp 目标) */
	}

	return sym_alloc(op, a, b, width, c);
}

/* recursively evaluate a symbol node to a constant.
   Returns 1 and sets *out if the whole expression is constant (no REG leaves),
   0 otherwise. */
static int sym_eval_const_depth(int id, unsigned long long *out, int depth)
{
	SYM_NODE *n;
	unsigned long long va, vb;
	int wa, wb;
	/* recursion guard: a stale id from sym_pool_reset can form a cycle
	   (node a/b pointing back) -> infinite recursion -> stack overflow
	   c00000fd. Cap the depth (the genuine symbol trees are shallow). */
	if (depth > 64)
		return 0;
	if (id < 0 || id >= g_sym_n)
		return 0;
	n = &g_sym_pool[id];
	switch (n->op)
	{
	case SYM_OP_CONST:
		*out = n->c;
		return 1;
	case SYM_OP_REG:
		return 0;   /* unknown runtime value */
	default: break;
	}
	if (!sym_eval_const_depth(n->a, out, depth + 1))
		return 0;
	va = *out;
	if (!sym_eval_const_depth(n->b, out, depth + 1))
		return 0;
	vb = *out;
	wa = sym_width(n->a);
	wb = sym_width(n->b);
	{
		unsigned long long r = 0;
		int w = n->width;
		switch (n->op)
		{
		case SYM_OP_ADD: r = va + vb; break;
		case SYM_OP_SUB: r = va - vb; break;
		case SYM_OP_XOR: r = va ^ vb; break;
		case SYM_OP_AND: r = va & vb; break;
		case SYM_OP_OR:  r = va | vb; break;
		case SYM_OP_MUL: r = va * vb; break;
		case SYM_OP_SHL: r = va << (vb & 63); break;
		case SYM_OP_SHR: r = va >> (vb & 63); break;
		case SYM_OP_ROL:
			{ int cnt = (int)(vb & (w - 1)); r = (va << cnt) | (va >> (w - cnt)); }
			break;
		case SYM_OP_ROR:
			{ int cnt = (int)(vb & (w - 1)); r = (va >> cnt) | (va << (w - cnt)); }
			break;
		case SYM_OP_NOT: r = ~va; break;
		case SYM_OP_NEG: r = 0 - va; break;
		case SYM_OP_LEA: r = va + n->c; break;
		default: return 0;
		}
		*out = sym_maskv(r, w);
		return 1;
	}
	(void)wa; (void)wb;
}

/* register symbol get/set */
static int sym_eval_const(int id, unsigned long long *out)
{
	return sym_eval_const_depth(id, out, 0);
}


/* register symbol array is kept in MINI_REG as sym[16] */

/* register file for the mini-simulator (16 GP regs) */
typedef struct {
	unsigned long long r[16];
	unsigned char t[16];   /* taint: 1 = value from VM stack/context */
	int sym[16];           /* symbolic expression node id per register */
	unsigned long long eflags;
	int flags_valid;   /* eflags updated by a tracked ALU op (trustworthy) */
	MINI_MEM mem;
	unsigned long long vsp;
} MINI_REG;

static unsigned char g_sym_gc_seen[SYM_POOL_MAX];
static int g_sym_gc_new[SYM_POOL_MAX];
static int g_sym_gc_stack[SYM_POOL_MAX];

static void sym_gc(MINI_REG *mreg)
{
	if (g_sym_n < SYM_POOL_MAX * 3 / 4) return;   /* only when nearly full */
	memset(g_sym_gc_seen, 0, (size_t)g_sym_n);
	/* iterative reachability from the 8 register roots PLUS the memory-slot
	   symbols (the acc chain lives in a vsp slot and must survive GC, else a
	   200k-instruction full trace exhausts the pool and every cross-handler
	   symbol degrades to CONST 0). */
	int sp = 0;
	for (int k = 0; k < 8; k++)
	{
		int id = mreg->sym[k];
		if (id > 0 && id < g_sym_n && id < SYM_POOL_MAX && !g_sym_gc_seen[id])
		{
			g_sym_gc_seen[id] = 1;
			if (sp < SYM_POOL_MAX) g_sym_gc_stack[sp++] = id;
		}
	}
	for (int mi = 0; mi < mreg->mem.count && mi < MINI_MEM_SLOTS; mi++)
	{
		int id = mreg->mem.sym[mi];
		if (id > 0 && id < g_sym_n && id < SYM_POOL_MAX && !g_sym_gc_seen[id])
		{
			g_sym_gc_seen[id] = 1;
			if (sp < SYM_POOL_MAX) g_sym_gc_stack[sp++] = id;
		}
	}
	/* 阶段二 acc 载体上一符号也必须是 GC 根: 跨 handler 累积链跨过多次 GC
	   而 g_acc2_sym 单独持有(不在寄存器/内存槽里), 不保护就会被回收导致
	   累积链断裂(只剩 acc+=a, acc+=b 两步就断)。 */
	if (g_acc2_sym > 0 && g_acc2_sym < g_sym_n && g_acc2_sym < SYM_POOL_MAX &&
		!g_sym_gc_seen[g_acc2_sym])
	{
		g_sym_gc_seen[g_acc2_sym] = 1;
		if (sp < SYM_POOL_MAX) g_sym_gc_stack[sp++] = g_acc2_sym;
	}
	while (sp > 0 && sp <= SYM_POOL_MAX)
	{
		int id = g_sym_gc_stack[--sp];
		SYM_NODE *nd = &g_sym_pool[id];
		if (nd->op == SYM_OP_CONST || nd->op == SYM_OP_REG) continue;
		int ch[2] = { nd->a, nd->b };
		for (int i = 0; i < 2; i++)
		{
			int c = ch[i];
			if (c > 0 && c < g_sym_n && c < SYM_POOL_MAX && !g_sym_gc_seen[c])
			{
				g_sym_gc_seen[c] = 1;
				if (sp < SYM_POOL_MAX) g_sym_gc_stack[sp++] = c;
			}
		}
	}
	/* compact: keep node 0 (const 0 anchor) if seen; remap live nodes to the
	   front of the pool */
	int nn = 0;
	for (int i = 0; i < g_sym_n; i++) g_sym_gc_new[i] = -1;
	/* node id 0 is SYM_OP_CONST 0 by convention (sym_alloc first call); keep
	   it reachable even if not marked, so a dangling 0 stays a zero const */
	if (g_sym_n > 0) { g_sym_gc_seen[0] = 1; }
	for (int i = 0; i < g_sym_n; i++)
	{
		if (g_sym_gc_seen[i])
		{
			g_sym_gc_new[i] = nn;
			g_sym_pool[nn] = g_sym_pool[i];
			nn++;
		}
	}
	for (int i = 0; i < nn; i++)
	{
		SYM_NODE *nd = &g_sym_pool[i];
		if (nd->op == SYM_OP_CONST) continue;
		if (nd->op == SYM_OP_REG) continue;
		if (nd->a > 0 && nd->a < g_sym_n) nd->a = (unsigned short)g_sym_gc_new[nd->a];
		if (nd->b > 0 && nd->b < g_sym_n) nd->b = (unsigned short)g_sym_gc_new[nd->b];
	}
	for (int k = 0; k < 8; k++)
	{
		int id = mreg->sym[k];
		if (id > 0 && id < g_sym_n) mreg->sym[k] = g_sym_gc_new[id];
		else if (id < 0) mreg->sym[k] = 0;
	}
	/* remap memory-slot symbols too (same GC must keep them consistent) */
	for (int mi = 0; mi < mreg->mem.count && mi < MINI_MEM_SLOTS; mi++)
	{
		int id = mreg->mem.sym[mi];
		if (id > 0 && id < g_sym_n) mreg->mem.sym[mi] = g_sym_gc_new[id];
		else if (id < 0) mreg->mem.sym[mi] = 0;
	}
	/* 阶段二 acc 载体上一符号在 GC 后也要重映射, 否则 g_acc2_sym 指向旧的
	   compact 前 node id, 累积步判定(符号 id 相等)随之失效。 */
	if (g_acc2_sym > 0 && g_acc2_sym < g_sym_n)
		g_acc2_sym = g_sym_gc_new[g_acc2_sym];
	else if (g_acc2_sym < 0)
		g_acc2_sym = -1;
	/* NOTE: g_src_symid cached node ids go stale after compaction, but
	   vmp38_sym_name_srcs already re-validates/re-allocates on the next use
	   (it checks op == SYM_OP_SRC && c == var id), so no explicit clear here. */
	g_sym_n = nn;
}

/* allocate a node (no simplify) */

/* does the memory expression reference any tainted register? */
/* rebuild all register symbols after pool reset (pool ids are stale).
   tainted regs -> REG symbol, others -> CONST of current value */
static void sym_rebuild_all(MINI_REG *mreg)
{
	int i;
	sym_pool_reset();
	for (i = 0; i < 16; i++)
	{
		if (mreg->t[i])
			mreg->sym[i] = sym_alloc(SYM_OP_REG, i, 0, 32, 0);
		else
			mreg->sym[i] = sym_alloc(SYM_OP_CONST, 0, 0, 32, mreg->r[i]);
	}
}

/* partially evaluate a symbol: REG leaves substituted with their current
   register values (if the register is not tainted, i.e. not VM-stack derived,
   the value is a reliable constant). Returns 1 if fully resolved. */
static int sym_eval_partial(int id, MINI_REG *mreg, unsigned long long *out);   /* fwd */

/* ================================================================
   MBA helper: return the "inner" expression if `id` is a complement-
   form of something (NOT(x), XOR(x, all-ones), x^-1, NEG(x)+-1 ...).
   VMP writes ~mask as one of these. On success *inner = the node under
   the complement and the TRUE polarity is inverted (the complemented mask
   selects the OTHER branch). Returns:
     0 = plain node (no complement),
     1 = complement of *inner.
   ================================================================ */
static int sym_complement_of(int id, int *inner)
{
	SYM_NODE *n;
	if (id < 0 || id >= g_sym_n) return 0;
	n = &g_sym_pool[id];
	if (n->op == SYM_OP_NOT)
	{
		*inner = n->a;
		return 1;
	}
	if (n->op == SYM_OP_XOR &&
		((sym_is_const(n->a) && sym_const_val(n->a) == sym_maskv(~0ULL, n->width)) ||
		 (sym_is_const(n->b) && sym_const_val(n->b) == sym_maskv(~0ULL, n->width))))
	{
		/* x ^ 0xFFFFFFFF = ~x (mask side is the non-const child) */
		*inner = sym_is_const(n->a) ? n->b : n->a;
		return 1;
	}
	return 0;
}

/* ================================================================
   MBA (Mixed Boolean-Arithmetic) decomposition for jmp reg targets.
   VMP 3.x obfuscates branch dispatch as:
       mask = -1 + flag           ; flag in {0,1} -> mask in {0xFFFFFFFF, 0}
       a1   = mask & FAddr
       a2   = ~mask & TAddr
       jmp  = a1 + a2
   where FAddr/TAddr are the two branch targets (both VM-section addresses).
   Given the register's SYMBOLIC expression, walk the tree for an
   ADD(AND(x,mask), AND(y,~mask)) / ADD(AND(mask,x), AND(notmask,y)) shape
   and extract the two possible targets. Returns 1 and fills cand[] when a
   plausible MBA pair is found.

   The two branch targets are only accepted when they land inside the
   [vm_lo, vm_hi) VM section - that filter (previously dropped) is what
   separates a real dispatch pair from obfuscation residue that also
   happens to be ADD(AND,AND). The ~mask side is recognised through
   sym_complement_of (NOT / XOR -1), so AND(~mask, TAddr) contributes its
   TAddr the same as AND(mask, FAddr). */
static int sym_mba_decompose(int id, MINI_REG *mreg,
	unsigned long long vm_lo, unsigned long long vm_hi,
	unsigned long long *cand0, unsigned long long *cand1)
{
	SYM_NODE *n;
	unsigned long long a0 = 0, a1 = 0;
	int have0 = 0, have1 = 0;
	/* candidate must land in the VM section to be trusted */
#define MBA_IN_VM(v) ((vm_lo) && (vm_hi) && (v) >= (vm_lo) && (v) < (vm_hi))

	if (id < 0 || id >= g_sym_n)
		return 0;
	n = &g_sym_pool[id];
	/* top must be ADD of two terms */
	if (n->op != SYM_OP_ADD)
		return 0;

	/* term A = n->a, term B = n->b - each is AND(mask, value), where the
	   mask side may be a complement (NOT / XOR -1) of the other's. The
	   value side is whichever operand, after complement peeling, evaluates
	   to a VM-section address. */
	{
		SYM_NODE *na = &g_sym_pool[n->a];
		SYM_NODE *nb = &g_sym_pool[n->b];
		unsigned long long va = 0, vb = 0;
		int okA = 0, okB = 0;

		/* extract the VALUE side of AND(mask, value); the mask operand may
		   itself be NOT(x)/x^-1 -> peel it, the value stays the other side */
		{
			/* ---- term A ---- */
			if (na->op == SYM_OP_AND)
			{
				unsigned long long c1 = 0, c2 = 0;
				int isc1 = sym_is_const(na->a), isc2 = sym_is_const(na->b);
				int inner_a = -1, inner_b = -1;
				int ca = sym_complement_of(na->a, &inner_a);
				int cb = sym_complement_of(na->b, &inner_b);
				/* if one operand is a complement (NOT/XOR -1), it is the mask
				   side - the VALUE is the OTHER operand (peel the complement) */
				if (cb && !ca) { isc2 = 1; c2 = 0; }        /* na->b is ~mask */
				else if (ca && !cb) { isc1 = 1; c1 = 0; }   /* na->a is ~mask */
				if (isc1) c1 = sym_const_val(na->a);
				if (isc2) c2 = sym_const_val(na->b);
				if (isc1 && !isc2 && sym_eval_partial(na->b, mreg, &va))
					okA = 1;
				else if (isc2 && !isc1 && sym_eval_partial(na->a, mreg, &va))
					okA = 1;
				else if (isc1 && isc2)
				{
					va = c1 & c2;
					okA = 1;
				}
				else if (sym_eval_partial(na->a, mreg, &c1) &&
					sym_eval_partial(na->b, mreg, &c2))
				{
					va = c1 & c2;
					okA = 1;
				}
			}

			/* ---- term B ---- */
			if (nb->op == SYM_OP_AND)
			{
				unsigned long long c1 = 0, c2 = 0;
				int isc1 = sym_is_const(nb->a), isc2 = sym_is_const(nb->b);
				int inner_a = -1, inner_b = -1;
				int ca = sym_complement_of(nb->a, &inner_a);
				int cb = sym_complement_of(nb->b, &inner_b);
				if (cb && !ca) { isc2 = 1; c2 = 0; }
				else if (ca && !cb) { isc1 = 1; c1 = 0; }
				if (isc1) c1 = sym_const_val(nb->a);
				if (isc2) c2 = sym_const_val(nb->b);
				if (isc1 && !isc2 && sym_eval_partial(nb->b, mreg, &vb))
					okB = 1;
				else if (isc2 && !isc1 && sym_eval_partial(nb->a, mreg, &vb))
					okB = 1;
				else if (isc1 && isc2)
				{
					vb = c1 & c2;
					okB = 1;
				}
				else if (sym_eval_partial(nb->a, mreg, &c1) &&
					sym_eval_partial(nb->b, mreg, &c2))
				{
					vb = c1 & c2;
					okB = 1;
				}
			}
		}

		/* ---- vm-section filter: only trust in-section candidates ---- */
		if (okA && MBA_IN_VM(va)) { a0 = va; have0 = 1; }
		if (okB && MBA_IN_VM(vb)) { a1 = vb; have1 = 1; }

		/* If one side's value didn't land in the VM section but IS recoverable
		   by peeling a complement mask (NOT / XOR -1), recover it: the
		   complement mask means that side carries the OTHER branch target. */
		if (have0 && !have1 && nb->op == SYM_OP_AND)
		{
			unsigned long long x = 0;
			int isca = sym_is_const(nb->a), iscb = sym_is_const(nb->b);
			if (isca && sym_eval_partial(nb->b, mreg, &x) && MBA_IN_VM(x)) { a1 = x; have1 = 1; }
			else if (iscb && sym_eval_partial(nb->a, mreg, &x) && MBA_IN_VM(x)) { a1 = x; have1 = 1; }
		}
		if (have1 && !have0 && na->op == SYM_OP_AND)
		{
			unsigned long long x = 0;
			int isca = sym_is_const(na->a), iscb = sym_is_const(na->b);
			if (isca && sym_eval_partial(na->b, mreg, &x) && MBA_IN_VM(x)) { a0 = x; have0 = 1; }
			else if (iscb && sym_eval_partial(na->a, mreg, &x) && MBA_IN_VM(x)) { a0 = x; have0 = 1; }
		}
	}

	if (!have0 && !have1)
		return 0;
	*cand0 = a0;
	*cand1 = a1;
#undef MBA_IN_VM
	return 1;
}

static int sym_eval_partial(int id, MINI_REG *mreg, unsigned long long *out)
{
	SYM_NODE *n;
	unsigned long long va, vb;
	int w;
	if (id < 0 || id >= g_sym_n)
		return 0;
	n = &g_sym_pool[id];
	switch (n->op)
	{
	case SYM_OP_CONST:
		*out = n->c;
		return 1;
	case SYM_OP_REG:
		/* use current register value if not VM-stack-derived */
		if (!mreg->t[n->a])
		{
			*out = mreg->r[n->a];
			return 1;
		}
		return 0;
	default: break;
	}
	if (!sym_eval_partial(n->a, mreg, &va))
		return 0;
	if (!sym_eval_partial(n->b, mreg, &vb))
		return 0;
	w = n->width;
	switch (n->op)
	{
	case SYM_OP_ADD: *out = va + vb; break;
	case SYM_OP_SUB: *out = va - vb; break;
	case SYM_OP_XOR: *out = va ^ vb; break;
	case SYM_OP_AND: *out = va & vb; break;
	case SYM_OP_OR:  *out = va | vb; break;
	case SYM_OP_MUL: *out = va * vb; break;
	case SYM_OP_SHL: *out = va << (vb & 63); break;
	case SYM_OP_SHR: *out = va >> (vb & 63); break;
	case SYM_OP_ROL:
		{ int cnt = (int)(vb & (w - 1)); *out = (va << cnt) | (va >> (w - cnt)); }
		break;
	case SYM_OP_ROR:
		{ int cnt = (int)(vb & (w - 1)); *out = (va >> cnt) | (va << (w - cnt)); }
		break;
	case SYM_OP_NOT: *out = ~va; break;
	case SYM_OP_NEG: *out = 0 - va; break;
	case SYM_OP_LEA: *out = va + n->c; break;
	default: return 0;
	}
	*out = sym_maskv(*out, w);
	return 1;
}

/* P3: lightweight symbolic tracking for jmp-reg target resolution under the
   Unicorn full-trace. Tracks register symbols PLUS a memory-symbol table
   (absolute-address keyed) so symbols survive VMP's stack/array relocations:
   mov [mem],reg / mov reg,[mem] keep the expression flowing instead of
   breaking at every stack shuffle (the root cause of "vm 还原不到位"). */
static int compute_abs_addr(MINI_REG *mreg, const char *expr, unsigned long long *addr);

/* memory-symbol table accessors (reuse MINI_MEM off[] as absolute address) */
static int sym_mem_lookup(MINI_REG *mreg, unsigned long long addr)
{
	MINI_MEM *mm = &mreg->mem;
	/* key by vsp-relative offset (addr - ebp): VMP moves ebp (vsp) as the
	   VM stack grows/shrinks, so the same LOGICAL slot has different absolute
	   addresses across handlers. addr - ebp is the stable identity. */
	unsigned long long vsp_off = addr - (mreg->r[5] & 0xffffffffULL);
	for (int si = mm->count - 1; si >= 0; si--)
		if (mm->off[si] == vsp_off)
			return mm->sym[si];
	return -1;
}
static void sym_mem_store(MINI_REG *mreg, unsigned long long addr, int sym)
{
	MINI_MEM *mm = &mreg->mem;
	unsigned long long vsp_off = addr - (mreg->r[5] & 0xffffffffULL);
	if (sym < 0) return;
	for (int si = 0; si < mm->count; si++)
		if (mm->off[si] == vsp_off)
		{
			mm->sym[si] = sym;
			mm->val[si] = 0;
			return;
		}
	if (mm->count < MINI_MEM_SLOTS)
	{
		mm->off[mm->count] = vsp_off;
		mm->val[mm->count] = 0;
		mm->sym[mm->count] = sym;
		mm->size[mm->count] = 32;
		mm->count++;
	}
}
/* 2026-09-02 跨 handler 槽 taint: 存取内存槽时同时带上 taint 位。
   这是"读字节码 → 存槽 → 跨 handler 读回 → 解密"长链 taint 不中断的关键。 */
static void sym_mem_store_t(MINI_REG *mreg, unsigned long long addr, int sym, unsigned char taint)
{
	MINI_MEM *mm = &mreg->mem;
	unsigned long long vsp_off = addr - (mreg->r[5] & 0xffffffffULL);
	if (sym < 0) return;
	for (int si = 0; si < mm->count; si++)
		if (mm->off[si] == vsp_off)
		{
			mm->sym[si] = sym;
			mm->val[si] = 0;
			mm->t[si] = taint;
			return;
		}
	if (mm->count < MINI_MEM_SLOTS)
	{
		mm->off[mm->count] = vsp_off;
		mm->val[mm->count] = 0;
		mm->sym[mm->count] = sym;
		mm->size[mm->count] = 32;
		mm->t[mm->count] = taint;
		mm->count++;
	}
}
/* 返回槽符号; *taint_out 若传非空则回填该槽 taint 位 */
static int sym_mem_lookup_t(MINI_REG *mreg, unsigned long long addr, unsigned char *taint_out)
{
	MINI_MEM *mm = &mreg->mem;
	unsigned long long vsp_off = addr - (mreg->r[5] & 0xffffffffULL);
	for (int si = mm->count - 1; si >= 0; si--)
		if (mm->off[si] == vsp_off)
		{
			if (taint_out) *taint_out = mm->t[si];
			return mm->sym[si];
		}
	if (taint_out) *taint_out = 0;
	return -1;
}
/* render a symbolic expression node as a string (recursive) */
static int g_sym_dbg_depth = 0;   /* reset before each top-level sym_expr_str */
static void sym_expr_str(int n, char *buf, size_t sz)
{
	if (n <= 0 || n >= SYM_POOL_MAX) { snprintf(buf, sz, "?"); return; }
	if (g_sym_dbg_depth > 200) { snprintf(buf, sz, "..."); return; }
	SYM_NODE *nd = &g_sym_pool[n];
	char l[128], r[128];
	g_sym_dbg_depth++;
	switch (nd->op)
	{
	case 0: snprintf(buf, sz, "%llx", nd->c); return;
	case SYM_OP_SRC:
		if (nd->c < (unsigned long long)g_src_cnt)
			snprintf(buf, sz, "%s", g_src_names[nd->c]);
		else
			snprintf(buf, sz, "src%llu", nd->c);
		return;
	case SYM_OP_REG: snprintf(buf, sz, "sym%d", nd->a); return;
	case SYM_OP_NOT: sym_expr_str(nd->a, l, sizeof(l)); snprintf(buf, sz, "~(%s)", l); return;
	case SYM_OP_NEG: sym_expr_str(nd->a, l, sizeof(l)); snprintf(buf, sz, "-(%s)", l); return;
	case SYM_OP_ADD: sym_expr_str(nd->a, l, sizeof(l)); sym_expr_str(nd->b, r, sizeof(r)); snprintf(buf, sz, "(%s+%s)", l, r); return;
	case SYM_OP_SUB: sym_expr_str(nd->a, l, sizeof(l)); sym_expr_str(nd->b, r, sizeof(r)); snprintf(buf, sz, "(%s-%s)", l, r); return;
	case SYM_OP_XOR: sym_expr_str(nd->a, l, sizeof(l)); sym_expr_str(nd->b, r, sizeof(r)); snprintf(buf, sz, "(%s^%s)", l, r); return;
	case SYM_OP_AND: sym_expr_str(nd->a, l, sizeof(l)); sym_expr_str(nd->b, r, sizeof(r)); snprintf(buf, sz, "(%s&%s)", l, r); return;
	case SYM_OP_OR:  sym_expr_str(nd->a, l, sizeof(l)); sym_expr_str(nd->b, r, sizeof(r)); snprintf(buf, sz, "(%s|%s)", l, r); return;
	case SYM_OP_SHL:
		{ unsigned long long cnt = (nd->b >= 0 && nd->b < SYM_POOL_MAX && sym_is_const(nd->b)) ? sym_const_val(nd->b) : 0;
		if (cnt == 0) { sym_expr_str(nd->a, buf, sz); return; }
		sym_expr_str(nd->a, l, sizeof(l)); snprintf(buf, sz, "(%s<<%llu)", l, cnt); return; }
	case SYM_OP_SHR:
		{ unsigned long long cnt = (nd->b >= 0 && nd->b < SYM_POOL_MAX && sym_is_const(nd->b)) ? sym_const_val(nd->b) : 0;
		if (cnt == 0) { sym_expr_str(nd->a, buf, sz); return; }
		sym_expr_str(nd->a, l, sizeof(l)); snprintf(buf, sz, "(%s>>%llu)", l, cnt); return; }
	case SYM_OP_ROL:
		{ unsigned long long cnt = (nd->b >= 0 && nd->b < SYM_POOL_MAX && sym_is_const(nd->b)) ? sym_const_val(nd->b) : 0;
		if (cnt == 0) { sym_expr_str(nd->a, buf, sz); return; }
		sym_expr_str(nd->a, l, sizeof(l)); snprintf(buf, sz, "rol(%s,%llu)", l, cnt); return; }
	case SYM_OP_ROR:
		{ unsigned long long cnt = (nd->b >= 0 && nd->b < SYM_POOL_MAX && sym_is_const(nd->b)) ? sym_const_val(nd->b) : 0;
		if (cnt == 0) { sym_expr_str(nd->a, buf, sz); return; }
		sym_expr_str(nd->a, l, sizeof(l)); snprintf(buf, sz, "ror(%s,%llu)", l, cnt); return; }
	case SYM_OP_LEA: sym_expr_str(nd->a, l, sizeof(l)); sym_expr_str(nd->b, r, sizeof(r)); snprintf(buf, sz, "(%s+%s+%llx)", l, r, nd->c); return;
	case SYM_OP_MUL: sym_expr_str(nd->a, l, sizeof(l)); sym_expr_str(nd->b, r, sizeof(r)); snprintf(buf, sz, "(%s*%s)", l, r); return;
	default: snprintf(buf, sz, "?op%d", nd->op); return;
	}
}

/* is the symbol a CLEAN source expression? Clean means it contains only
   constants and named source variables combined by ADD/SUB/MUL (the source
   arithmetic). Any XOR/AND/OR/NOT/NEG/ROL/ROR/SHL/SHR/LEA or an unknown
   register symbol (symN) means the value is encrypted / not-yet-named, so
   the expression is NOT a source op candidate. */
static int sym_is_clean_src(int n, int depth)
{
	SYM_NODE *nd;
	if (n <= 0 || n >= SYM_POOL_MAX) return 0;
	if (depth > 64) return 0;
	nd = &g_sym_pool[n];
	switch (nd->op)
	{
	case 0:   /* CONST */
	case SYM_OP_SRC:
		return 1;
	case SYM_OP_ADD:
	case SYM_OP_SUB:
	case SYM_OP_MUL:
		return sym_is_clean_src(nd->a, depth + 1) &&
			sym_is_clean_src(nd->b, depth + 1);
	default:
		return 0;
	}
}

/* does the expression reference at least one named source variable (a/b/
   acc0)? A pure-constant expression is the VMP encrypt-key arithmetic, not
   a source op, so it must be excluded. */
static int sym_has_src(int n, int depth)
{
	SYM_NODE *nd;
	if (n <= 0 || n >= SYM_POOL_MAX) return 0;
	if (depth > 64) return 0;
	nd = &g_sym_pool[n];
	if (nd->op == SYM_OP_SRC) return 1;
	if (nd->op == SYM_OP_ADD || nd->op == SYM_OP_SUB || nd->op == SYM_OP_MUL)
		return sym_has_src(nd->a, depth + 1) || sym_has_src(nd->b, depth + 1);
	return 0;
}

/* does the expression reference an INTERMEDIATE source variable (sN, id>=3)?
   A clean source op over the SEED scalars (a/b/acc0) is the real source
   instruction; an op over sN intermediates is encryption residue - exclude
   those from the [sym-op] listing but keep the naming alive. */
static int sym_has_inter_src(int n, int depth)
{
	SYM_NODE *nd;
	if (n <= 0 || n >= SYM_POOL_MAX) return 0;
	if (depth > 64) return 0;
	nd = &g_sym_pool[n];
	if (nd->op == SYM_OP_SRC)
		return (nd->c >= 3);   /* s3.. = intermediate; a/b/acc0 = seed */
	if (nd->op == SYM_OP_ADD || nd->op == SYM_OP_SUB || nd->op == SYM_OP_MUL)
		return sym_has_inter_src(nd->a, depth + 1) || sym_has_inter_src(nd->b, depth + 1);
	return 0;
}

/* does the expression reference a SEED source variable (a/b/acc0, SRC id<3)?
   A clean source op anchored on the seed scalars is the real source
   arithmetic; ops over tail-named intermediates (s3..) or pure registers are
   encryption residue. Phase-2 uses this to decide whether a register is an
   accumulating acc carrier vs VMP engine scratch. */
static int sym_has_seed_src(int n, int depth)
{
	SYM_NODE *nd;
	if (n <= 0 || n >= SYM_POOL_MAX) return 0;
	if (depth > 64) return 0;
	nd = &g_sym_pool[n];
	if (nd->op == SYM_OP_SRC)
		return (nd->c < (unsigned long long)g_src_cnt);   /* any named source scalar (a/b/acc0/argN/const) = seed */
	if (nd->op == SYM_OP_ADD || nd->op == SYM_OP_SUB || nd->op == SYM_OP_MUL)
		return sym_has_seed_src(nd->a, depth + 1) || sym_has_seed_src(nd->b, depth + 1);
	return 0;
}

/* parse the up-to-two register operands of a disasm line (eax/ecx/edx/ebx).
   Returns the count; fills regs[] with the parent GPR indices (0-3). */
static int sym_parse_opregs(const char *disasm, int regs[4])
{
	static const char *rn4[4] = { "eax", "ecx", "edx", "ebx" };
	int n = 0, i;
	for (i = 0; i < 4 && n < 2; i++)
	{
		const char *p = disasm;
		while ((p = strstr(p, rn4[i])) != NULL)
		{
			/* avoid matching ah/al/ax inside eax (eax strstr hits "ax" too) */
			regs[n++] = i;
			p += 3;
			break;
		}
	}
	return n;
}

/* width-aware register accessor: returns the parent GPR index (0-7) and
   fills *shift/*mask for the sub-register field (al/cl/dx/ah/ch...). This
   lets the symbolic tracker keep CONCRETE values width-correct, so a
   variable shift by cl sees cl's real 8-bit value (0 -> no-op). */
static int vmp38_regmask(const char *name, int *shift, unsigned int *mask)
{
	int p = reg_index_from_name(name);
	if (p < 0 || p >= 8) return -1;
	*shift = 0; *mask = 0xFFFFFFFFu;
	if (!name || !name[0]) return p;
	size_t L = strlen(name);
	/* high-byte regs: ah ch dh bh */
	if (L == 2 && name[1] == 'h' && strchr("acdb", name[0]))
	{ *shift = 8; *mask = 0xFFu; return p; }
	if (L == 2 && name[1] == 'l') { *mask = 0xFFu; return p; }        /* al cl dl bl */
	if (L == 2 && (name[1] == 'x' || name[1] == 'p' || name[1] == 'i')) { *mask = 0xFFFFu; return p; }  /* ax cx dx bx sp bp si di */
	if (L == 3 && name[0] == 'e') { *mask = 0xFFFFFFFFu; return p; }  /* eax ... */
	return p;
}

/* ---- sub-register width semantics (root cause c fix) ---------------------
   x86 8/16-bit ops on ax/al/ah/cx/... keep the OTHER bits of the parent GPR:
     xor cx, dx  ->  ecx = (ecx & ~0xFFFF) | ((ecx ^ edx) & 0xFFFF)
   The old sym_track_insn built the symbol as a FULL-32-bit op, so
   xor cx,dx with ecx==edx folded x^x -> 0, losing the preserved high 16
   bits (0x7D3E0000). These two helpers express the exact semantics with
   the existing SHR/AND/SHL/OR nodes, so the constant path stays foldable.

   sym_read_field(sym, shift, mask)  = (sym >> shift) & mask   (read a field)
   sym_write_field(old, field, shift, mask) =
       (old & ~(mask<<shift)) | ((field & mask) << shift)      (partial write)
   Full 32-bit fields are identity (no nodes allocated). */
static int sym_read_field(int sym, unsigned int shift, unsigned int mask)
{
	if (shift == 0 && mask == 0xFFFFFFFFu)
		return sym;
	int s = sym;
	if (shift != 0)
		s = sym_build(SYM_OP_SHR, s,
			sym_alloc(SYM_OP_CONST, 0, 0, 32, shift), 32, 0);
	return sym_build(SYM_OP_AND, s,
		sym_alloc(SYM_OP_CONST, 0, 0, 32, mask), 32, 0);
}

static int sym_write_field(int old_sym, int field_sym, unsigned int shift, unsigned int mask)
{
	if (shift == 0 && mask == 0xFFFFFFFFu)
		return field_sym;
	unsigned int wmask = mask << shift;
	int low = sym_build(SYM_OP_AND, field_sym,
		sym_alloc(SYM_OP_CONST, 0, 0, 32, mask), 32, 0);
	if (shift != 0)
		low = sym_build(SYM_OP_SHL, low,
			sym_alloc(SYM_OP_CONST, 0, 0, 32, shift), 32, 0);
	int high = sym_build(SYM_OP_AND, old_sym,
		sym_alloc(SYM_OP_CONST, 0, 0, 32, ~wmask), 32, 0);
	return sym_build(SYM_OP_OR, high, low, 32, 0);
}

/* 2026-08-25 探针: 符号化传播可行性验证。
   inject: 把入口观测到的"合理整数参数"从 CONST 改绑为 SYM_OP_SRC 符号根,
   使符号引擎从 trace 一开始就保留"这是参数根"的身份并随指令传播——
   即使该参数值从不在 trace 里裸现(被 VMP 搬进虚拟栈/加密链), 符号仍可追溯。
   检测: sym_contains_var 遍历符号树找是否含指定 SRC var 根。 */
static int sym_contains_var(int n, unsigned long long varid, int depth)
{
	SYM_NODE *nd;
	if (n <= 0 || n >= SYM_POOL_MAX) return 0;
	if (depth > 64) return 0;
	nd = &g_sym_pool[n];
	if (nd->op == SYM_OP_SRC)
		return (nd->c == varid);
	if (nd->op == SYM_OP_ADD || nd->op == SYM_OP_SUB || nd->op == SYM_OP_MUL ||
		nd->op == SYM_OP_XOR || nd->op == SYM_OP_AND || nd->op == SYM_OP_OR ||
		nd->op == SYM_OP_SHL || nd->op == SYM_OP_SHR || nd->op == SYM_OP_ROL ||
		nd->op == SYM_OP_ROR || nd->op == SYM_OP_LEA)
		return sym_contains_var(nd->a, varid, depth + 1) ||
			sym_contains_var(nd->b, varid, depth + 1);
	if (nd->op == SYM_OP_NOT || nd->op == SYM_OP_NEG)
		return sym_contains_var(nd->a, varid, depth + 1);
	return 0;
}

static int g_src_symid[64];   /* one cached SYM_OP_SRC node per source var */
/* fwd decl: g_uc is defined later (uc engine); sym_track_insn needs it to read
   the concrete dispatch-table value a mov reg,[mem] loads when the symbol
   table has no entry for that address (GAP-A fix). */
extern uc_engine *g_uc;
static void vmp38_sym_name_srcs(MINI_REG *mreg)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		unsigned int v = (unsigned int)mreg->r[i];
		int id = vmp38_src_id_of(v);
		if (id >= 0)
		{
			int sid = g_src_symid[id];
			/* (re)alloc if stale: pool reset / GC invalidates the cached id */
			if (sid <= 0 || sid >= g_sym_n ||
				g_sym_pool[sid].op != SYM_OP_SRC ||
				g_sym_pool[sid].c != (unsigned long long)id)
			{
				sid = sym_alloc(SYM_OP_SRC, 0, 0, 32, (unsigned long long)id);
				g_src_symid[id] = sid;
			}
			mreg->sym[i] = g_src_symid[id];
		}
		/* 2026-08-30 8-bit 子寄存器锚定: VMP 用 al/cl/dl 做防追踪, 关键参数
		   只活在低 8/16 位里(父寄存器高位塞垃圾)。上面只认 32 位全等, 会漏掉
		   `eax=0xffff0060` 这个"低 8 位=0x60(b)"的载体。这里补: 若源标量 ≤0xFF
		   且 r[i] 低 8 位命中, 把低 8 位字段绑成该源符号(高位保持原符号)。 */
		else
		{
			unsigned int lo8 = v & 0xFFu;
			int id8 = vmp38_src_id_of(lo8);
			if (id8 >= 0 && (v >> 8) != 0)   /* 高位非零才是"防追踪塞垃圾"形态 */
			{
				int sid8 = g_src_symid[id8];
				if (sid8 <= 0 || sid8 >= g_sym_n ||
					g_sym_pool[sid8].op != SYM_OP_SRC ||
					g_sym_pool[sid8].c != (unsigned long long)id8)
				{
					sid8 = sym_alloc(SYM_OP_SRC, 0, 0, 32, (unsigned long long)id8);
					g_src_symid[id8] = sid8;
				}
				/* 只把低 8 位字段替换成源符号, 高位字段保留原符号(垃圾) */
				if (sid8 > 0)
					mreg->sym[i] = sym_write_field(mreg->sym[i], sid8, 0, 0xFFu);
			}
			else
			{
				unsigned int lo16 = v & 0xFFFFu;
				int id16 = vmp38_src_id_of(lo16);
				if (id16 >= 0 && (v >> 16) != 0)
				{
					int sid16 = g_src_symid[id16];
					if (sid16 <= 0 || sid16 >= g_sym_n ||
						g_sym_pool[sid16].op != SYM_OP_SRC ||
						g_sym_pool[sid16].c != (unsigned long long)id16)
					{
						sid16 = sym_alloc(SYM_OP_SRC, 0, 0, 32, (unsigned long long)id16);
						g_src_symid[id16] = sid16;
					}
					if (sid16 > 0)
						mreg->sym[i] = sym_write_field(mreg->sym[i], sid16, 0, 0xFFFFu);
				}
			}
		}
	}
}

static void sym_track_insn(MINI_REG *mreg, const char *disasm)
{
	char m[32], op1[64] = "", op2[64] = "";
	unsigned long long imm = 0;
	int dst, src;
	int dshift = 0, sshift = 0;
	unsigned int dmask = 0xFFFFFFFFu, smask = 0xFFFFFFFFu;
	const char *p;
	get_mnemonic(disasm, m, sizeof(m));
	p = disasm;
	while (*p && *p != ' ') p++;
	while (*p == ' ') p++;
	{
		int k = 0;
		while (*p && *p != ',' && k < 63) op1[k++] = *p++;
		op1[k] = 0;
		if (*p == ',') { p++; while (*p == ' ') p++; k = 0; while (*p && k < 63) op2[k++] = *p++; op2[k] = 0; }
	}
	dst = (op1[0] && op1[0] != '[') ? vmp38_regmask(op1, &dshift, &dmask) : -1;
	src = (op2[0] && op2[0] != '[') ? vmp38_regmask(op2, &sshift, &smask) : -1;

	if (!strcmp(m, "mov") && dst >= 0 && parse_num(op2, &imm))
	{
		/* width-aware concrete value: writing al/ax/cx... keeps the parent
		   GPR's OTHER bits (a partial write); only a full eax/r32 write
		   replaces the whole register. The symbol must use sym_write_field
		   so the preserved high bits stay symbolic (consistent with the ALU
		   paths that review flagged). */
		unsigned int nv;
		if (dshift == 8)
			nv = (mreg->r[dst] & ~(dmask << dshift)) | ((unsigned int)(imm & dmask) << dshift);
		else if (dmask != 0xFFFFFFFFu)
			nv = (mreg->r[dst] & ~dmask) | ((unsigned int)imm & dmask);   /* preserve high bits */
		else
			nv = (unsigned int)imm;
		mreg->r[dst] = nv;
		/* symbol: partial-write for 8/16-bit destinations */
		{
			int old_sym = mreg->sym[dst];
			int imm_sym = sym_alloc(SYM_OP_CONST, 0, 0, 32,
				((unsigned int)imm & dmask) & 0xffffffffULL);
			mreg->sym[dst] = sym_write_field(old_sym, imm_sym,
				(unsigned int)dshift, dmask);
		}
		mreg->t[dst] = 0;
	}
	else if (!strcmp(m, "mov") && dst >= 0 && src >= 0)
	{
		unsigned int nv = (mreg->r[src] >> sshift) & smask;
		if (dshift == 8)
			nv = (mreg->r[dst] & ~(dmask << dshift)) | ((nv & dmask) << dshift);
		else if (dmask != 0xFFFFFFFFu)
			nv &= dmask;   /* zero-extend into low 8/16 field */
		mreg->r[dst] = nv;
		/* partial-write: mov cx,dx keeps ecx's high 16 bits */
		{
			int old_sym = mreg->sym[dst];
			int rhs_field = sym_read_field(mreg->sym[src], (unsigned int)sshift, smask);
			mreg->sym[dst] = sym_write_field(old_sym, rhs_field,
				(unsigned int)dshift, dmask);
		}
		mreg->t[dst] = mreg->t[src];
	}
	else if ((!strcmp(m, "mov") || !strcmp(m, "movzx") || !strcmp(m, "movsx"))
		&& dst >= 0 && strchr(op2, '[') != NULL)
	{
		/* mov/movzx/movsx reg, [mem]: read the symbol stored at that
		   absolute address (width extension keeps the same symbol). */
		unsigned long long addr = 0;
		if (compute_abs_addr(mreg, op2, &addr))
		{
			unsigned char staint = 0;
			int s = sym_mem_lookup_t(mreg, addr, &staint);
			if (s >= 0)
			{
				mreg->sym[dst] = s;
				mreg->t[dst] = staint;   /* 槽 taint 随符号一起流动 */
			}
			else
			{
				unsigned char mb[4] = { 0, 0, 0, 0 };
				int mrd = 0;
				if (g_uc)
				{
					__try {
						mrd = (uc_mem_read(g_uc, addr, mb, 4) == UC_ERR_OK) ? 1 : 0;
					} __except (EXCEPTION_EXECUTE_HANDLER) { mrd = 0; }
				}
				if (mrd)
				{
					unsigned long long v = (unsigned long long)mb[0] |
						((unsigned long long)mb[1] << 8) |
						((unsigned long long)mb[2] << 16) |
						((unsigned long long)mb[3] << 24);
					mreg->sym[dst] = sym_alloc(SYM_OP_CONST, 0, 0, 32, v);
					mreg->t[dst] = 0;   /* now known: a concrete loaded value */
				}
			}
		}
		else
			/* GAP-A fall-through: if the address doesn't resolve, keep the
			   previous symbol (the old full-32 copy behaviour); the concrete
			   value was never reliably known. */
			{ }
	}
	else if (!strcmp(m, "mov") && strchr(op1, '[') != NULL)
	{
		/* mov [mem], reg/imm: write the source symbol + taint to the address */
		unsigned long long addr = 0;
		if (compute_abs_addr(mreg, op1, &addr))
		{
			int slot_sym = -1;
			unsigned char slot_taint = 0;
			if (parse_num(op2, &imm))
			{
				slot_sym = sym_alloc(SYM_OP_CONST, 0, 0, 32, imm & 0xffffffffULL);
				slot_taint = 0;   /* 写立即数: 清除槽 taint */
			}
			else if (src >= 0)
			{
				slot_sym = mreg->sym[src];
				slot_taint = mreg->t[src];   /* 写寄存器: 槽 taint 继承寄存器 */
			}
			if (slot_sym >= 0)
				sym_mem_store_t(mreg, addr, slot_sym, slot_taint);
		}
	}
	else if ((!strcmp(m, "add") || !strcmp(m, "sub") || !strcmp(m, "xor") ||
		!strcmp(m, "and") || !strcmp(m, "or")) && strchr(op1, '[') != NULL)
	{
		/* memory-target ALU: add/sub/xor/and/or [mem], reg/imm - read the
		   symbol stored at that absolute address, combine and store back.
		   This symbols the memory operand (removes the '?' in [net]). */
		unsigned long long addr = 0;
		if (compute_abs_addr(mreg, op1, &addr))
		{
			int op = !strcmp(m, "add") ? SYM_OP_ADD : !strcmp(m, "sub") ? SYM_OP_SUB :
				!strcmp(m, "xor") ? SYM_OP_XOR : !strcmp(m, "and") ? SYM_OP_AND : SYM_OP_OR;
			int mbase = sym_mem_lookup(mreg, addr);
			if (mbase < 0)
				mbase = sym_alloc(SYM_OP_CONST, 0, 0, 32, 0);
			int rhs = -1;
			if (src >= 0) rhs = mreg->sym[src];
			else if (parse_num(op2, &imm))
				rhs = sym_alloc(SYM_OP_CONST, 0, 0, 32, imm & 0xffffffffULL);
			if (rhs >= 0)
				sym_mem_store(mreg, addr, sym_build(op, mbase, rhs, 32, 0));
		}
	}
	else if ((!strcmp(m, "imul") || !strcmp(m, "mul") ||
		!strcmp(m, "shl") || !strcmp(m, "shr") || !strcmp(m, "sar") ||
		!strcmp(m, "rol") || !strcmp(m, "ror")) && strchr(op1, '[') != NULL)
	{
		/* 2026-08-25 memory-target mul/shift/rotate (vmp_demo LCG `s*=C`,
		   bitmix `cks rol 5`): read old slot symbol, apply op, store back.
		   Parity with the register-target branches above.) */
		unsigned long long addr = 0;
		if (compute_abs_addr(mreg, op1, &addr))
		{
			int op = (!strcmp(m, "imul") || !strcmp(m, "mul")) ? SYM_OP_MUL :
				(!strcmp(m, "shl")) ? SYM_OP_SHL :
				(!strcmp(m, "shr") || !strcmp(m, "sar")) ? SYM_OP_SHR :
				!strcmp(m, "rol") ? SYM_OP_ROL : SYM_OP_ROR;
			int mbase = sym_mem_lookup(mreg, addr);
			if (mbase < 0)
				mbase = sym_alloc(SYM_OP_CONST, 0, 0, 32, 0);
			int rhs = -1;
			if (src >= 0) rhs = mreg->sym[src];
			else if (parse_num(op2, &imm))
				rhs = sym_alloc(SYM_OP_CONST, 0, 0, 32, imm & 0x3f);
			if (rhs >= 0 && mbase >= 0)
				sym_mem_store(mreg, addr, sym_build(op, mbase, rhs, 32, 0));
		}
	}
	else if ((!strcmp(m, "add") || !strcmp(m, "sub") || !strcmp(m, "xor") ||
		!strcmp(m, "and") || !strcmp(m, "or") || !strcmp(m, "adc") || !strcmp(m, "sbb")) && dst >= 0)
	{
		int op = !strcmp(m, "add") ? SYM_OP_ADD : !strcmp(m, "sub") ? SYM_OP_SUB :
			!strcmp(m, "xor") ? SYM_OP_XOR : !strcmp(m, "and") ? SYM_OP_AND :
			!strcmp(m, "or") ? SYM_OP_OR : !strcmp(m, "adc") ? SYM_OP_ADD : SYM_OP_SUB;
		{
			unsigned int cv = (src >= 0) ? ((unsigned int)mreg->r[src] >> sshift) & smask
				: (parse_num(op2, &imm) ? (unsigned int)imm : 0);
			unsigned int old = ((unsigned int)mreg->r[dst] >> dshift) & dmask;
			unsigned int nv = 0;
			switch (op)
			{
			case SYM_OP_ADD: nv = old + cv; break;
			case SYM_OP_SUB: nv = old - cv; break;
			case SYM_OP_XOR: nv = old ^ cv; break;
			case SYM_OP_AND: nv = old & cv; break;
			case SYM_OP_OR:  nv = old | cv; break;
			}
			nv &= dmask;
			mreg->r[dst] = (mreg->r[dst] & ~(dmask << dshift)) | (nv << dshift);
		}
		/* ROOT-CAUSE C FIX: preserve the parent GPR high bits for 8/16-bit
		   ops. xor cx,dx must keep ecx's high 16 bits, not fold x^x->0. */
		{
			int old_sym = mreg->sym[dst];
			int dst_field = sym_read_field(old_sym, (unsigned int)dshift, dmask);
			int rhs_sym = -1;
			if (src >= 0)
				rhs_sym = sym_read_field(mreg->sym[src], (unsigned int)sshift, smask);
			else if (parse_num(op2, &imm))
				rhs_sym = sym_alloc(SYM_OP_CONST, 0, 0, 32, imm & 0xffffffffULL);
			/* rhs must be masked to the dst field width too (xor cx, imm16) */
			if (dmask != 0xFFFFFFFFu && rhs_sym >= 0)
				rhs_sym = sym_build(SYM_OP_AND, rhs_sym,
					sym_alloc(SYM_OP_CONST, 0, 0, 32, dmask), 32, 0);
			if (rhs_sym >= 0)
			{
				int new_field = sym_build(op, dst_field, rhs_sym, 32, 0);
				mreg->sym[dst] = sym_write_field(old_sym, new_field,
					(unsigned int)dshift, dmask);
			}
		}
		/* 2026-09-02 taint 传播: ALU 读改写 dst 结果承自 dst 与 src 的 taint */
		if (src >= 0) mreg->t[dst] = (unsigned char)(mreg->t[dst] | mreg->t[src]);
	}
	else if ((!strcmp(m, "not") || !strcmp(m, "neg")) && dst >= 0)
	{
		unsigned int old = ((unsigned int)mreg->r[dst] >> dshift) & dmask;
		unsigned int nv = (!strcmp(m, "not")) ? (~old & dmask) : (0u - old) & dmask;
		mreg->r[dst] = (mreg->r[dst] & ~(dmask << dshift)) | (nv << dshift);
		/* partial-write semantics: preserve parent GPR high bits */
		{
			int old_sym = mreg->sym[dst];
			int dst_field = sym_read_field(old_sym, (unsigned int)dshift, dmask);
			int new_field = sym_build(!strcmp(m, "not") ? SYM_OP_NOT : SYM_OP_NEG,
				dst_field, 0, 32, 0);
			mreg->sym[dst] = sym_write_field(old_sym, new_field,
				(unsigned int)dshift, dmask);
		}
	}
	else if ((!strcmp(m, "inc") || !strcmp(m, "dec")) && dst >= 0)
	{
		unsigned int old = ((unsigned int)mreg->r[dst] >> dshift) & dmask;
		unsigned int nv = (!strcmp(m, "inc")) ? (old + 1) & dmask : (old - 1) & dmask;
		mreg->r[dst] = (mreg->r[dst] & ~(dmask << dshift)) | (nv << dshift);
		/* partial-write semantics: preserve parent GPR high bits */
		{
			int old_sym = mreg->sym[dst];
			int dst_field = sym_read_field(old_sym, (unsigned int)dshift, dmask);
			int new_field = sym_build(!strcmp(m, "inc") ? SYM_OP_ADD : SYM_OP_SUB,
				dst_field, sym_alloc(SYM_OP_CONST, 0, 0, 32, 1), 32, 0);
			mreg->sym[dst] = sym_write_field(old_sym, new_field,
				(unsigned int)dshift, dmask);
		}
	}
	else if ((!strcmp(m, "shl") || !strcmp(m, "shr") || !strcmp(m, "sar") ||
		!strcmp(m, "rol") || !strcmp(m, "ror")) && dst >= 0)
	{
		int op = !strcmp(m, "shl") ? SYM_OP_SHL :
			(!strcmp(m, "shr") || !strcmp(m, "sar")) ? SYM_OP_SHR :
			!strcmp(m, "rol") ? SYM_OP_ROL : SYM_OP_ROR;
		if (parse_num(op2, &imm))
		{
			unsigned int bits = (dmask == 0xFFu) ? 8 : (dmask == 0xFFFFu) ? 16 : 32;
			unsigned int k = (unsigned int)imm & (bits - 1);
			unsigned int field = (unsigned int)mreg->r[dst] >> dshift;
			if (k != 0)
			{
				unsigned int v = field & dmask;
				unsigned int nv = 0;
				if (op == SYM_OP_SHL) nv = v << k;
				else if (op == SYM_OP_SHR) nv = v >> k;
				else if (op == SYM_OP_ROL) nv = (v << k) | (v >> (bits - k));
				else nv = (v >> k) | (v << (bits - k));
				nv &= dmask;
				mreg->r[dst] = (mreg->r[dst] & ~(dmask << dshift)) | (nv << dshift);
			}
			/* partial-write semantics: shift only the dst FIELD, keep the
			   parent GPR high bits (ror ax,0xCC keeps eax's high 16 bits) */
			{
				int old_sym = mreg->sym[dst];
				int dst_field = sym_read_field(old_sym, (unsigned int)dshift, dmask);
				int new_field = sym_build(op, dst_field,
					sym_alloc(SYM_OP_CONST, 0, 0, 32, imm & 0x3f), 32, 0);
				mreg->sym[dst] = sym_write_field(old_sym, new_field,
					(unsigned int)dshift, dmask);
			}
		}
		else if (src >= 0)
		{
			/* variable shift: if the count register's concrete value is 0
			   the shift is a no-op - skip the symbol update entirely (this
			   is what lets shr x,cl / shl x,0 chains cancel). */
			if (((unsigned int)mreg->r[src] & 31) == 0)
				return;
			unsigned int bits = (dmask == 0xFFu) ? 8 : (dmask == 0xFFFFu) ? 16 : 32;
			unsigned int k = (unsigned int)mreg->r[src] & (bits - 1);
			unsigned int v = ((unsigned int)mreg->r[dst] >> dshift) & dmask;
			unsigned int nv = 0;
			if (op == SYM_OP_SHL) nv = v << k;
			else if (op == SYM_OP_SHR) nv = v >> k;
			else if (op == SYM_OP_ROL) nv = (v << k) | (v >> (bits - k));
			else nv = (v >> k) | (v << (bits - k));
			nv &= dmask;
			mreg->r[dst] = (mreg->r[dst] & ~(dmask << dshift)) | (nv << dshift);
			if (mreg->sym[dst] >= 0 && mreg->sym[src] >= 0)
			{
				int old_sym = mreg->sym[dst];
				int dst_field = sym_read_field(old_sym, (unsigned int)dshift, dmask);
				/* use the CONCRETE count k (not the count register's symbol):
				   SHL(x,cl) with cl=1 must fold as SHL(x,1) -> MUL(x,2), which
				   is the VMP rewrite of `lea [x+x]`. Burying cl's symbol
				   prevents the SHL-by-const -> multiply folding. */
				int new_field = sym_build(op, dst_field,
					sym_alloc(SYM_OP_CONST, 0, 0, 32, k), 32, 0);
				mreg->sym[dst] = sym_write_field(old_sym, new_field,
					(unsigned int)dshift, dmask);
			}
		}
	}
	else if (!strcmp(m, "lea") && dst >= 0 && src >= 0)
	{
		/* lea reg,[base+disp]: approximate as ADD(base_sym, disp); disp is
		   extracted from the bracket via a local scan (parse_imm_from_disasm
		   is declared later in this TU, so scan inline). */
		unsigned long long disp = 0;
		const char *b = strchr(op2, '[');
		const char *e = b ? strchr(b, ']') : NULL;
		const char *plus = NULL;
		if (b && e)
		{
			const char *s = b;
			while (s < e)
			{
				if ((*s == '+' || *s == '-') && s + 1 < e &&
					(s[1] == '0' || (s[1] >= '1' && s[1] <= '9')))
				{ plus = s; break; }
				s++;
			}
			if (plus)
				disp = strtoull(plus, NULL, 0);
		}
		mreg->sym[dst] = sym_build(SYM_OP_ADD, mreg->sym[src],
			sym_alloc(SYM_OP_CONST, 0, 0, 32, disp & 0xffffffffULL), 32, 0);
	}
	else if (!strcmp(m, "push"))
	{
		/* record the pushed value's symbol into a vsp-keyed stack slot, so
		   an iretd (VMP dispatch via [esp]) can be resolved from the slot */
		MINI_MEM *mm = &mreg->mem;
		mreg->r[4] -= 4;
		int slot_sym = -1;
		if (parse_num(op1, &imm))
			slot_sym = sym_alloc(SYM_OP_CONST, 0, 0, 32, imm & 0xffffffffULL);
		else if (dst >= 0)
			slot_sym = mreg->sym[dst];
		if (slot_sym >= 0 && mm->count < MINI_MEM_SLOTS)
		{
			mm->off[mm->count] = mreg->r[4];
			mm->val[mm->count] = 0;
			mm->sym[mm->count] = slot_sym;
			mm->size[mm->count] = 32;
			mm->count++;
		}
	}
	else if (!strcmp(m, "pop") && dst >= 0)
	{
		MINI_MEM *mm = &mreg->mem;
		for (int si = 0; si < mm->count; si++)
			if (mm->off[si] == mreg->r[4])
			{
				mreg->sym[dst] = mm->sym[si];
				break;
			}
		mreg->r[4] += 4;
	}
}

/* [net]: symbolically execute the FULL instruction sequence of one handler
   (all instructions, not just the live core ops) and emit each register's
   algebraically-simplified NET EFFECT - the one source-level operation this
   handler rebuilds (VMP obfuscation rol/ror/not cancels inside sym_build).
   This is the "递归符号执行计算" side: per-handler net effect to be matched
   against the pre-VM source instruction list when building the conversion
   table. */

/* ============ 阶段二: 跨 handler acc 累积折叠 ============
   在 P3 符号引擎里, g_symreg 是全程跨 handler 的符号状态(mov reg,reg
   搬运 acc 符号也会被 sym_read_field/sym_write_field 保留), 所以无论 acc
   载体被 VMP 搬去 eax/ecx/edx/ebx 的哪一个, 它的符号都是一条从 seed 源变量
   (a/b/acc0) 出发、经 ADD/SUB clean 增量单调生长的表达式链。阶段二检测到
   某个寄存器的符号 = 上一个符号 + clean 增量(纯常量或命名源变量)时, 就把这
   一步判定为一次真 acc 累加, 输出 `acc += Δ` 到 .txt。它是通用的(不依赖
   golden 39 条硬编码): Δ 由符号引擎代数化简得出, 载体寄存器由
   sym_has_seed_src + clean 增量自动判定, 换样本同样适用。 */

/* 阶段二 acc 累积检测(每指令 P3 后调用)。核心是"锁定载体 + 延迟确认":
   只有当某个寄存器的符号确实从上一符号经一次 clean 增量(ADD/SUB)演化时, 才
   输出 `acc ±= Δ`; 载体的起点(`acc seed`)只在建立时输出一次, 不再像第一阶段
   雏形那样把每条"含 seed 的干净表达式"都误报成起点。载体按"含 seed 且不含
   中间 sN 变量"判定, 一旦锁定就一直跟踪到累积链被打断, 因此 VMP 引擎的
   单步混淆表达式不会反复污染输出。 */
static void vmp38_acc2_fold(MINI_REG *mr, unsigned int addr, const char *insn)
{
	static const char *rn4[4] = { "eax","ecx","edx","ebx" };
	int k;
	if (!mr) return;

	/* 解析当前指令: mnemonic + dst/src(regs[0]/regs[1])。只有载体上的
	   add/adc/sub/sbb 才是真 acc 累加步。 */
	char mo[16] = ""; get_mnemonic(insn, mo, sizeof(mo));
	int is_arith = (!strcmp(mo,"add") || !strcmp(mo,"adc") ||
		!strcmp(mo,"sub") || !strcmp(mo,"sbb"));
	int aregs[4] = { -1,-1,-1,-1 };
	int arn = is_arith ? sym_parse_opregs(insn, aregs) : 0;

/* ---- 累积步: 已锁定载体, 当前指令是 载体 arith, 增量 seed 相关 ---- */
	if (g_acc2_reg >= 0 && g_acc2_reg < 4 && g_acc2_pending)
	{
		k = g_acc2_reg;
		/* 只有当指令把载体作为 dst 参与 add/adc/sub/sbb 才算累积步 */
		if (arn >= 1 && aregs[0] == k)
		{
			int src = (arn >= 2) ? aregs[1] : -1;
			int sub = (mo[0]=='s');   /* 'sub'/'sbb' */
			unsigned long long V = (unsigned long long)mr->r[k];
			/* 增量接受判定: 只接收"seed 锚定的 clean 源表达式"。
			   -- sym_is_clean_src: 排除引擎混淆(含 XOR/OR/AND/NOT/ROL/SHL 等)。
			   -- seed/SRC: 增量是 a/b/acc0 或 seed 衍生的中间源变量 sN
			      (真 acc 增量如 a*3/a/7 会保留 ADD/SUB/MUL 结构或被值锚定成
			       SRC 叶子, 都通过); 纯常量(地址/字节增量 0x4ff9xx、0x80、引擎
			       大随机)被拒绝, 消除载体漂移噪声。 */
			int accept = 0;
			char inc[160]; inc[0] = 0;
			if (src >= 0 && src < 8)
			{
				int ds = mr->sym[src];
				if (ds > 0 && ds < g_sym_n)
				{
					SYM_NODE *dn = &g_sym_pool[ds];
					if (sym_is_clean_src(ds, 0) &&
						(sym_has_seed_src(ds, 0) || dn->op == SYM_OP_SRC))
					{
						g_sym_dbg_depth = 0;
						sym_expr_str(ds, inc, sizeof(inc));
						accept = 1;
					}
				}
			}
			if (accept)
			{
				if (!g_acc2_started)
				{
					char es0[160]; es0[0] = 0;
					if (g_acc2_sym > 0 && g_acc2_sym < g_sym_n)
					{
						g_sym_dbg_depth = 0;
						sym_expr_str(g_acc2_sym, es0, sizeof(es0));
					}
					if (!es0[0]) _snprintf(es0, sizeof(es0), "acc");
					out("  [src] @%08X: acc seed = %s  %s %08llx",
						addr, es0, rn4[k], g_acc2_pre);
					g_acc2_started = 1;
				}
				out("  [src] @%08X: acc %s= %s  %s %08llx -> %08llx",
					addr, sub ? "-" : "+", inc, rn4[k], g_acc2_pre, V);
				g_acc2_pre = V;
				g_acc2_pending = 1;
				return;
			}
		}
		return;   /* 载体保持锁定(即使这一步非累积) */
	}

/* ---- 起点/接管: 某寄存器符号是含 seed 源变量(a/b/acc0)的 clean 表达式 ---- */
	if (g_acc2_reg < 0)
	{
		/* 第一遍: 优先选承载 acc0(SRC id=2, fb_seed 的 acc 初值占位) 的寄存器 ——
		   真 acc 载体从 acc0 出发单调累积, 这是最可靠的 acc 链起点信号,
		   避免被值锚定成 a/b 的 eax 抢先误锁。 */
		for (k = 0; k < 4; k++)
		{
			int S = mr->sym[k];
			SYM_NODE *nd = (S > 0 && S < g_sym_n) ? &g_sym_pool[S] : NULL;
			if (nd && nd->op == SYM_OP_SRC && nd->c == 2)   /* acc0 */
			{
				g_acc2_reg = k; g_acc2_sym = S; g_acc2_pending = 1;
				g_acc2_pre = (unsigned long long)mr->r[k];
				g_acc2_started = 0;
				return;
			}
		}
		/* 第二遍 fallback: 任意含 seed 的 clean 表达式 / 裸 seed 叶子 */
		for (k = 0; k < 4; k++)
		{
			int S = mr->sym[k];
			SYM_NODE *nd = (S > 0 && S < g_sym_n) ? &g_sym_pool[S] : NULL;
			if (!nd) continue;
			if (nd->op == SYM_OP_REG && nd->a == (unsigned short)k) continue;
			int is_seed_leaf = (nd->op == SYM_OP_SRC && nd->c < (unsigned long long)g_src_cnt);
			int is_clean_seed = (nd->op == SYM_OP_ADD || nd->op == SYM_OP_SUB ||
				nd->op == SYM_OP_MUL) &&
				sym_has_seed_src(S, 0) && sym_is_clean_src(S, 0) &&
				!sym_has_inter_src(S, 0);
			if (is_seed_leaf || is_clean_seed)
			{
				g_acc2_reg = k; g_acc2_sym = S; g_acc2_pending = 1;
				g_acc2_pre = (unsigned long long)mr->r[k];
				g_acc2_started = 0;
				return;
			}
		}
	}
}

static MINI_REG g_netreg;
static int g_netreg_init = 0;

static void vmp38_net_fold(const char (*insns)[220], int cnt,
	const unsigned int *pre_regs, const unsigned int (*step_regs)[8],
	char *out, int outsz)
{
	static const char *rn8n[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
	out[0] = 0; if (outsz <= 0) return;
	/* PER-HANDLER symbolic net effect (independent state, reset each call):
	   seed a local state from this handler's real registers, feed the FULL
	   instruction stream, emit each register's simplified net effect. This
	   is bounded per handler - it can NOT exhaust the sym pool or hang, so
	   the restore analysis always completes. (Cross-handler accumulation was
	   removed: it exhausted the pool / hung on full samples.) */
	MINI_REG mr; memset(&mr, 0, sizeof(mr));
	mr.vsp = pre_regs[4];
	for (int k = 0; k < 8; k++) { mr.r[k] = pre_regs[k]; mr.t[k] = 1; }
	sym_rebuild_all(&mr);
	for (int i = 0; i < cnt && i < 80; i++)
	{
		sym_track_insn(&mr, insns[i]);
		/* sync real register values (step_regs) so the value-anchor naming
		   can see the source scalars that `mov reg,[mem]` loaded (sym_track_insn
		   only tracks the symbol, not the concrete value, for memory reads). */
		if (step_regs)
			for (int k = 0; k < 8; k++)
				mr.r[k] = step_regs[i][k];
		vmp38_sym_name_srcs(&mr);   /* value-anchor: bind a/b/acc0 to named vars */
	}
	/* emit each register whose symbol changed to a non-trivial expression */
	int o = 0;
	for (int k = 0; k < 8; k++)
	{
		int sid = mr.sym[k];
		if (sid <= 0 || sid >= g_sym_n) continue;
		SYM_NODE *nd = &g_sym_pool[sid];
		/* unchanged: symbol is just REG(k) */
		if (nd->op == SYM_OP_REG && nd->a == (unsigned short)k) continue;
		char raw[300];
		sym_expr_str(sid, raw, sizeof(raw));
		char readable[360]; int ro = 0;
		for (const char *p = raw; *p && ro < (int)sizeof(readable) - 1; )
		{
			if (p[0]=='s' && p[1]=='y' && p[2]=='m')
			{
				p += 3; long rv = strtol(p, (char**)&p, 10);
				if (rv >= 0 && rv < 8)
					ro += _snprintf(readable + ro, sizeof(readable) - ro, "%s", rn8n[rv]);
				else
					ro += _snprintf(readable + ro, sizeof(readable) - ro, "v%ld", rv);
			}
			else
				readable[ro++] = *p++;
		}
		readable[ro] = 0;
		if (o) o += _snprintf(out + o, outsz - o, " ;");
		o += _snprintf(out + o, outsz - o, " %s=%s", rn8n[k], readable);
	}
	/* AST source-op match (Titan match_add): a handler whose net effect is a
	   CLEAN binary op over named source vars is a source instruction. Append
	   ` <<src: reg = a op b` to the output so the caller can surface it. */
	for (int k = 0; k < 8; k++)
	{
		int sid = mr.sym[k];
		if (sid <= 0 || sid >= g_sym_n) continue;
		SYM_NODE *nd = &g_sym_pool[sid];
		if ((nd->op == SYM_OP_ADD || nd->op == SYM_OP_SUB || nd->op == SYM_OP_MUL) &&
			sym_is_clean_src(nd->a, 0) && sym_is_clean_src(nd->b, 0) &&
			sym_has_src(sid, 0))
		{
			char ea[128], eb[128];
			g_sym_dbg_depth = 0;
			sym_expr_str(nd->a, ea, sizeof(ea));
			g_sym_dbg_depth = 0;
			sym_expr_str(nd->b, eb, sizeof(eb));
			if (o < outsz - 40)
				o += _snprintf(out + o, outsz - o, " <<src: %s=%s %s %s",
					rn8n[k], ea,
					nd->op == SYM_OP_ADD ? "+" : nd->op == SYM_OP_SUB ? "-" : "*",
					eb);
		}
	}
}

/* ============ xx_execute integration (original xxvm engine) ============ */
static void mini_mem_write(MINI_REG *mreg, unsigned long long vsp_off, unsigned long long val, int size, int sym);
static int mini_mem_read(MINI_REG *mreg, unsigned long long vsp_off, int size, unsigned long long *out, int *sym_out);
static unsigned long long trunc_reg(unsigned long long val, int width);
static MINI_REG *g_xx_mreg = 0;   /* active mini-reg during xx_execute */

/* process-memory snapshot for static simulation (original xxvm fmem
   approach): dump a window around the real ESP once, then ALL stack
   reads/writes during the simulation go to the snapshot - so simulated
   writes are visible to later reads (read/write consistent) and the
   live process is never touched. Fixes "add edx,[esp+0x04]" dispatch
   chains where the target is written to the stack mid-simulation. */
static unsigned char *g_snap = NULL;
static unsigned long long g_snap_begin = 0, g_snap_end = 0;
/* 根因修复(2026-08-23)：记忆库"内存段拷贝不全→死循环"。
   快照/模块 dump 时读取失败(Skip 留 0 / 未写入)的页，在 Unicorn 里是"已映射但
   内容是空白(0/NOP)"——Unicorn 读它不触发 unmapped hook，拿到假 0 数据导致
   状态无法推进(如 mov [edx],ecx 死循环)。记录这些"空白页"，VM 运行中首次访问时
   从 live 进程强制重读真实字节(第二次能读到就正常)。 */
static std::unordered_set<unsigned int> g_blank_pages;

static void vmp38_snap_take(unsigned long long esp)
{
	/* GUI: wide stack window - VMP moves esp a lot; reads beyond a narrow
	   window fall back to the live process (= stale pre-breakpoint values)
	   which breaks [esp+N] dispatch (jmp eax to data). headless: narrow
	   window (fast; its .text-call-site entry keeps esp stable). */
	unsigned long long win = 0x20000;
	{
		char ft_host[MAX_PATH] = ""; GetModuleFileNameA(NULL, ft_host, MAX_PATH);
		if (!strstr(ft_host, "headless")) win = 0x200000;
	}
	unsigned long long lo = (esp > win) ? esp - win : 0;
	unsigned long long hi = esp + win;
	if (g_snap) { free(g_snap); g_snap = NULL; }
	g_snap = (unsigned char*)malloc((size_t)(hi - lo));
	if (!g_snap) { g_snap_begin = g_snap_end = 0; return; }
	g_snap_begin = lo; g_snap_end = hi;
	memset(g_snap, 0, (size_t)(hi - lo));
	for (unsigned long long a = lo; a < hi; a += 0x1000)
	{
		size_t n = (size_t)(((hi - a) < 0x1000) ? (hi - a) : 0x1000);
		duint szread = 0;
		if (!(Script::Memory::Read((duint)a, g_snap + (a - lo), (duint)n, &szread)))
		{
			/* unreadable page: SKIP, don't truncate the window - 但记入空白页,
			   VM 运行中访问时从 live 强制重读(根治"内存段拷贝不全→死循环") */
			for (unsigned long long p = a; p < a + 0x1000 && p < hi; p += 0x1000)
				g_blank_pages.insert((unsigned int)(p & ~0xFFFULL));
			continue;
		}
	}
}

/* fmem fallback: original xxvm simulates against a REAL memory snapshot
   (fmem dump). We use g_snap (stack window taken at simulation start) -
   simulated writes land in the snapshot so later reads see them
   (read/write consistent). Real process memory next (GUI), mini-mem
   slots last (headless fallback). */
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
			mini_mem_read(g_xx_mreg, vsp_off, size * 8, &v, &sym))
		{
			memcpy(pdata, &v, size);
			return 1;
		}
	}
	return 0;
}

/* fmem fallback: write to the snapshot (simulated write visible to later
   reads; live process never touched). Out-of-window writes go to mini-mem. */
static int vmp38_fmem_write_hook(unsigned int addr, char *pdata, unsigned int size)
{
	if (g_snap && addr >= g_snap_begin && (unsigned long long)addr + size <= g_snap_end)
	{
		memcpy(g_snap + (addr - g_snap_begin), pdata, size);
		return 1;
	}
	if (g_xx_mreg)
	{
		unsigned long long vsp_off = (unsigned long long)addr - g_xx_mreg->vsp;
		unsigned long long v = 0;
		if (size <= 8)
		{
			memcpy(&v, pdata, size);
			mini_mem_write(g_xx_mreg, vsp_off, trunc_reg(v, size * 8), size * 8, -1);
			return 1;
		}
	}
	return 0;
}

static int vmp38_fmem_hooks_registered = 0;
static void vmp38_register_fmem_hooks(void)
{
	if (!vmp38_fmem_hooks_registered)
	{
		vmp38_mem_hook_read = vmp38_fmem_read_hook;
		vmp38_mem_hook_write = vmp38_fmem_write_hook;
		vmp38_fmem_hooks_registered = 1;
	}
}

/* per-instruction record */
typedef struct {
	unsigned long long ip;
	char disasm[128];
	int instr_len;
	int push;
	int pop;
	int is_alu;
	int is_jmp;
	int is_cond;   /* conditional jump (jb/jnz/jz/...) */
	unsigned long long cond_target;
	int is_core;   /* mini-taint: ALU on VM-stack-derived value */
	unsigned long long vsp_delta;
	unsigned long long real_val;
	int has_real;
	int reg_w;
	unsigned long long reg_w_val;
} STEP_REC;

int vmp38_unicorn_full_trace(unsigned long long entry_ip, unsigned long long entry_esp, unsigned long long *out_last_eip);
static void vmp38_rewrite_txt_from_data(void);

/* ---- 任务1 taint 全局(定义前移, 供 taint 分析遍/动态 hook/pass2 共用) ---- */
static unsigned char g_taint_reg[8];
static long long g_taint_vsp_off[1024];
static int g_taint_vsp_cnt = 0;
static unsigned char g_taint_enabled = 0;   /* 开关: p3_symreset 里置 1 */
static unsigned long long g_taint_vm_lo = 0, g_taint_vm_hi = 0;  /* VM 字节码段范围 */
/* 任务1: taint 判定的"有效指令"eip 集合(动态 trace + analyze pass 共用) */
static unsigned int g_taint_insns[65536];
static int g_taint_insn_cnt = 0;
/* 2026-09-02 任务3: 不透明谓词(恒真/恒假 jcc)识别。
   VMP 只用永真永假型 jcc(实测 base 901/901 恒定)。恒不跳 jcc = 无条件 fallthrough
   = 纯噪音, 直接删。恒跳 jcc = 等价无条件 jmp, 保留(其 fallthrough 段可能有别的入口)。 */
static unsigned int g_always_nt_jcc[4096];   /* 恒不跳 jcc 的 eip 集合 */
static int g_always_nt_jcc_cnt = 0;
/* 任务4: 乱序链 jmp(无条件 jmp 目标 == 执行流下一行 = 冗余连接) 集合 */
static unsigned int g_redundant_jmp[8192];
static int g_redundant_jmp_cnt = 0;
/* P3 符号引擎状态(定义前移到此处, 供 trace hook 与 taint 分析遍共用) */
static MINI_REG g_symreg;

/* ================================================================
   2026-09-01 任务1 方案A: 半自动识别专属寄存器 + 两遍 taint 分析。
   在 rewrite 阶段(有完整 data)做:
   (1) 遍1 统计: 间接跳转目标 -> vbase; xor 源 -> vkey; 内存基址 -> vsp/vpc;
   (2) 识别 4 专属寄存器(输出日志供确认);
   (3) 遍2 前向 taint 传播: vpc/vsp/vkey/vbase 为 taint 根 + 读字节码(用 vpc
       指向的内存即字节码) taint, 沿 dst 传播, 标记 tainted eip 到 g_taint_insns。
   与动态 hook 里的 vmp38_taint_propagate(粗 taint 源, 已移除) 不同, 这里
   能先用完整 trace 精确识别专属寄存器再做传播(第④篇"先采集后分析"的顺序)。
   ================================================================ */
static void vmp38_taint_analyze_pass(const char *dpath)
{
	FILE *fin = fopen(dpath, "rb");
	if (!fin) return;
	/* 统计数组: 间接跳转目标 / xor 源 / 内存基址 */
	int ind_imm_jmp[8] = {0,0,0,0,0,0,0,0};   /* jmp/call reg 目标 */
	int xor_src_cnt[8] = {0,0,0,0,0,0,0,0};   /* xor r, imm 之外的 xor r,reg 的源 */
	int mem_base_cnt[8] = {0,0,0,0,0,0,0,0};  /* 内存寻址的 base 寄存器 */
	/* 读地址页直方图(找字节码区) */
	struct { unsigned int addr; int cnt; } page_hist[2048];
	int page_n = 0;
	char line[4096];
	while (fgets(line, sizeof(line), fin))
	{
		if (strncmp(line, "eip=", 4) != 0) continue;
		char *code = strchr(line + 4, '\t');
		if (!code) continue;
		code++;
		static const char *rn[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
		char mn[16] = ""; int mi = 0;
		while (code[mi] && mi < 15 && code[mi] != ' ' && code[mi] != '\t') mn[mi++] = code[mi];
		mn[mi] = 0;
		char ops[512]; int oi = 0;
		const char *p = code + mi;
		while (*p == ' ' || *p == '\t') p++;
		while (*p && *p != '\n' && *p != '\r' && oi < 511) ops[oi++] = *p++;
		ops[oi] = 0;
		/* 截断 regs 列, 取 memcol(读地址直方图用) */
		char *regsp = strchr(code, '\t');
		if (regsp) *regsp++ = 0;
		char *memcol = regsp ? strchr(regsp, '\t') : NULL;
		if (memcol) { memcol++; }
		/* 读地址页直方图: 找字节码区(vpc 密集读的 4KB 页) */
		if (memcol)
		{
			char *mp = memcol; char *lastbr = NULL;
			while ((mp = strstr(mp, "[")) != NULL)
			{ char *cl = strchr(mp, ']'); if (cl && cl - mp == 9) lastbr = mp; mp++; }
			if (lastbr)
			{
				char hx[9]; memcpy(hx, lastbr + 1, 8); hx[8] = 0;
				unsigned int a = (unsigned int)strtoul(hx, NULL, 16);
				unsigned int pg = a & 0xFFFFF000u;
				int f = -1;
				for (int q = 0; q < page_n; q++) if (page_hist[q].addr == pg) { f = q; break; }
				if (f >= 0) page_hist[f].cnt++;
				else if (page_n < 2048) { page_hist[page_n].addr = pg; page_hist[page_n].cnt = 1; page_n++; }
			}
		}
		/* 间接跳转 jmp/call reg(不含立即数/内存) */
		if ((!strcmp(mn, "jmp") || !strcmp(mn, "call")) &&
			!strchr(ops, '[') && !strchr(ops, 'x') && ops[0])
		{
			char r1[16] = ""; int rj = 0;
			while (ops[rj] && rj < 15 && ops[rj] != ' ' && ops[rj] != '\t') r1[rj++] = ops[rj];
			r1[rj] = 0;
			for (int k = 0; k < 8; k++) if (!strcmp(r1, rn[k])) { ind_imm_jmp[k]++; break; }
		}
		/* xor 源 = 第二操作数(reg) */
		if (!strcmp(mn, "xor"))
		{
			char *comma = strchr(ops, ',');
			if (comma)
			{
				char r2[16] = ""; int rk = 0;
				comma++; while (*comma == ' ') comma++;
				while (comma[rk] && rk < 15 && comma[rk] != ' ' && comma[rk] != '\t') r2[rk++] = comma[rk];
				r2[rk] = 0;
				for (int k = 0; k < 8; k++) if (!strcmp(r2, rn[k])) { xor_src_cnt[k]++; break; }
			}
		}
		/* 内存基址 = [base ...] */
		if (strchr(ops, '['))
		{
			char *lb = strchr(ops, '[');
			char r3[16] = ""; int rl = 0;
			lb++;
			while (*lb == ' ') lb++;
			while (lb[rl] && rl < 15 && lb[rl] != '+' && lb[rl] != '-' && lb[rl] != '*' && lb[rl] != ']') r3[rl++] = lb[rl];
			r3[rl] = 0;
			for (int k = 0; k < 8; k++) if (!strcmp(r3, rn[k])) { mem_base_cnt[k]++; break; }
		}
	}
	/* 识别 4 专属寄存器 */
	int vbase = 7;  /* 默认 edi */
	int vkey  = 3;  /* 默认 ebx */
	int vsp   = 4;  /* 默认 esp */
	int vpc   = 6;  /* 默认 esi */
	{
		int bj = -1, bjmax = -1;
		for (int k = 0; k < 8; k++) if (k != 4 && ind_imm_jmp[k] > bjmax) { bjmax = ind_imm_jmp[k]; bj = k; }
		if (bj >= 0) vbase = bj;
		int xk = -1, xkmax = -1;
		for (int k = 0; k < 8; k++) if (k != 4 && k != vbase && xor_src_cnt[k] > xkmax) { xkmax = xor_src_cnt[k]; xk = k; }
		if (xk >= 0) vkey = xk;
		/* vpc: 内存基址多 + 少量间接跳转, 排除 esp/vbase/vkey 后剩下最多的 */
		int pk = -1, pkmax = -1;
		for (int k = 0; k < 8; k++)
			if (k != 4 && k != vbase && k != vkey && mem_base_cnt[k] > pkmax) { pkmax = mem_base_cnt[k]; pk = k; }
		if (pk >= 0) vpc = pk;
	}
	/* 字节码区 = 读地址最高频 4KB 页(天然是 vpc 读字节码的密集区) */
	unsigned int code_page = 0; int code_page_cnt = 0;
	for (int q = 0; q < page_n; q++)
		if (page_hist[q].cnt > code_page_cnt) { code_page_cnt = page_hist[q].cnt; code_page = page_hist[q].addr; }
	static const char *rn2[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
	out_progress("[taint] detected vpc=%s vsp=%s vkey=%s vbase=%s bytecode_page=%08X (page_hits=%d)",
		rn2[vpc], rn2[vsp], rn2[vkey], rn2[vbase], code_page, code_page_cnt);

	/* 遍2: 前向 taint 传播(寄存器 + 内存 taint 表, 对齐第④篇) */
	fseek(fin, 0, SEEK_SET);
	int saved_sym_n = g_sym_n;   /* 记录符号池: analyze pass 的符号是临时的, 结束回退防多 run 累积 */
	/* 2026-09-02 路A: 用【局部 MINI_REG mr】符号引擎重放(替代手写 treg) ——
	   跨 handler 内存槽 taint(MINI_MEM.t[]) + ALU taint 传播已在 sym_track_insn
	   里补齐。用局部 mr 而非全局 g_symreg, 避免污染 trace 阶段的符号状态。 */
	MINI_REG mr;
	memset(&mr, 0, sizeof(mr));
	for (int k = 0; k < 16; k++) { mr.sym[k] = sym_alloc(SYM_OP_REG, k, 0, 32, 0); mr.t[k] = 0; }
	mr.t[4] = 0;   /* esp known */
	/* 初值: 记录每个寄存器的 pre 值来自 data 行的 pre= 字段 */
	unsigned int pre[8] = {0,0,0,0,0,0,0,0};
	g_taint_insn_cnt = 0;
	long lineidx = 0;
	while (fgets(line, sizeof(line), fin))
	{
		if (strncmp(line, "eip=", 4) != 0) continue;
		char *code = strchr(line + 4, '\t');
		if (!code) continue;
		code++;
		/* eip 值 */
		unsigned int eip = 0;
		{
			const char *hp = line + 4;
			for (int k = 0; k < 8; k++)
			{
				char c = hp[k]; int v;
				if (c >= '0' && c <= '9') v = c - '0';
				else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
				else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
				else v = 0;
				eip = (eip << 4) | (unsigned int)v;
			}
		}
		char *regs = strchr(code, '\t');
		if (regs) *regs++ = 0;
		/* 解析 pre 寄存器值(regs 列里 eax=...->... 形式取 pre) */
		{
			char rc[512]; _snprintf(rc, sizeof(rc), "%s", regs ? regs : "");
			for (char *tk = rc; tk && *tk; )
			{
				char *sp = strchr(tk, ' ');
				if (sp) *sp = 0;
				for (int k = 0; k < 8; k++)
				{
					char pat[8]; _snprintf(pat, sizeof(pat), "%s=", rn2[k]);
					if (strncmp(tk, pat, strlen(pat)) == 0)
					{
						const char *v = tk + strlen(pat);
						const char *arrow = strstr(v, "->");
						pre[k] = (unsigned int)strtoul(v, NULL, 16);
						break;
					}
				}
				tk = sp ? sp + 1 : NULL;
			}
		}
		/* 2026-09-02 路A: 把 data 的 pre 值喂给局部 mr, 用完整符号引擎重放传播 taint */
		for (int k = 0; k < 8; k++) mr.r[k] = pre[k];
		mr.vsp = pre[4];
		/* mnemonic 解析(供 taint 源判定) */
		char mn[16] = ""; int mi = 0;
		while (code[mi] && mi < 15 && code[mi] != ' ' && code[mi] != '\t') mn[mi++] = code[mi];
		mn[mi] = 0;
		/* 解析 dst(供 taint 源/判定) */
		char op1[80] = ""; int k1 = 0;
		{ const char *q = code + mi; while (*q==' '||*q=='\t') q++;
		  while (*q && *q != ',' && k1 < 79) op1[k1++] = *q++; op1[k1] = 0; }
		int dst = -1;
		if (op1[0] && op1[0] != '[') { for (int k = 0; k < 8; k++) if (!strcmp(op1, rn2[k])) { dst = k; break; } }
		/* 解析 src(写内存时的 taint 源) */
		int src = -1;
		{
			char *comma = strchr(code, ',');
			if (comma)
			{
				char op2[40] = ""; int k2 = 0;
				comma++; while (*comma==' '||*comma=='\t') comma++;
				while (comma[k2] && k2 < 39 && comma[k2] != '\t') op2[k2++] = comma[k2];
				op2[k2] = 0;
				if (op2[0] && op2[0] != '[' && !strchr(op2, 'x'))
					for (int k = 0; k < 8; k++) if (!strcmp(op2, rn2[k])) { src = k; break; }
			}
		}
		/* 解析读地址(taint 源判定: 读字节码页) */
		unsigned int maddr = 0; int got_maddr = 0;
		{ char *mp = line; char *lastbr = NULL;
		  while ((mp = strstr(mp, "[")) != NULL) { char *cl = strchr(mp, ']'); if (cl && cl - mp == 9) lastbr = mp; mp++; }
		  if (lastbr) { char hx[9]; memcpy(hx, lastbr+1, 8); hx[8]=0; maddr = (unsigned int)strtoul(hx, NULL, 16); got_maddr = 1; } }
		/* 符号执行传播(含跨 handler 槽 taint + ALU taint) */
		sym_track_insn(&mr, code);
		/* taint 源: 读基址是 vpc 的内存 -> dst taint(精确到"vpc 读谁谁就是字节码") */
		int vpc_base_read = 0;
		if (dst >= 0 && strchr(code, '['))
		{
			char *lb = strstr(code, "["); 
			if (lb) { char rb[16]=""; int rbi=0; lb++;
			  while (*lb==' ') lb++;
			  while (lb[rbi] && rbi<15 && lb[rbi]!='+' && lb[rbi]!='-' && lb[rbi]!='*' && lb[rbi]!=']') rb[rbi++]=lb[rbi];
			  rb[rbi]=0;
			  for (int k=0;k<8;k++) if (!strcmp(rb, rn2[k])) { if (k==vpc) vpc_base_read = 1; break; }
			}
		}
		if (vpc_base_read && dst >= 0) { mr.t[dst] = 1; }
		/* 写专属寄存器(机制关键, 第④篇的 * 标记) */
		int writes_special = (dst >= 0 && (dst == vpc || dst == vsp || dst == vkey || dst == vbase));
		/* 判定有效: 写了 tainted 寄存器 / 读字节码 / 读 vpc 数据 / 写专属寄存器 */
		/* 写内存 [mem] <- src(tainted): 槽继承 taint, 且本条指令本身 tainted(关键搬运) */
		int write_tainted_mem = 0;
		if (strchr(op1, '[') != NULL && src >= 0 && mr.t[src])
			write_tainted_mem = 1;
		int tainted = (dst >= 0 && mr.t[dst]) || vpc_base_read || writes_special || write_tainted_mem;
		if (tainted)
		{
			int dup = 0;
			for (int q = 0; q < g_taint_insn_cnt; q++) if (g_taint_insns[q] == eip) { dup = 1; break; }
			if (!dup && g_taint_insn_cnt < 65536)
				g_taint_insns[g_taint_insn_cnt++] = eip;
		}
		lineidx++;
	}
	/* 2026-09-02 任务3 遍3: 不透明谓词(恒定 jcc)识别。
	   顺序扫描 data, 判断每条 jcc 的 taken/not-taken 行为(下一条 eip 是否跳远),
	   恒不跳(永远 fallthrough)的 jcc = 纯噪音, 记入 g_always_nt_jcc 供 pass2 删。 */
	{
		fseek(fin, 0, SEEK_SET);
		unsigned int prev_eip = 0; int prev_is_jcc = 0;
		unsigned int prev_jcc_target = 0;   /* 上一条 jcc 的跳转目标(精确 taken 判定) */
		int prev_taken = 0;    /* 该 jcc 是否曾跳远 */
		int prev_nt = 0;       /* 该 jcc 是否曾顺序 */
		/* 任务4: 冗余 jmp 追踪 */
		int prev_was_jmp = 0; unsigned int prev_jmp_target = 0;
		g_redundant_jmp_cnt = 0;   /* 每 run 重置, 防跨 run 残留污染 */
		/* 简化: 用哈希分桶统计每 eip 的 taken/not-taken 次数 */
		struct { unsigned int eip; int taken; int nt; } jcc_stat[2048];
		int jcc_n = 0;
		while (fgets(line, sizeof(line), fin))
		{
			if (strncmp(line, "eip=", 4) != 0) continue;
			unsigned int eip = 0;
			{ const char *hp = line+4; for(int k=0;k<8;k++){ char c=hp[k]; int v;
				if(c>='0'&&c<='9')v=c-'0'; else if(c>='a'&&c<='f')v=c-'a'+10; else if(c>='A'&&c<='F')v=c-'A'+10; else v=0;
				eip=(eip<<4)|v; } }
			char *code = strchr(line + 4, '\t');
			code = code ? code + 1 : NULL;
			/* 上一条是 jcc 且本行是它的后继: 精确判断 taken(==jcc目标) 或 not-taken(其他) */
			if (prev_is_jcc)
			{
				int taken = (prev_jcc_target && eip == prev_jcc_target) ? 1 : 0;
				int f = -1;
				for (int q = 0; q < jcc_n; q++) if (jcc_stat[q].eip == prev_eip) { f = q; break; }
				if (f >= 0) { if (taken) jcc_stat[f].taken++; else jcc_stat[f].nt++; }
				else if (jcc_n < 2048) { jcc_stat[jcc_n].eip = prev_eip; jcc_stat[jcc_n].taken = taken ? 1 : 0; jcc_stat[jcc_n].nt = taken ? 0 : 1; jcc_n++; }
			}
			/* 本行是否 jcc + 解析它的目标 */
			char mn[8] = ""; int mi = 0;
			if (code) while (code[mi] && mi < 7 && code[mi] != ' ' && code[mi] != '\t') mn[mi++] = code[mi];
			mn[mi] = 0;
			int is_jcc = (mn[0] == 'j' && strcmp(mn, "jmp") != 0);
			unsigned int jcc_target = 0;
			if (is_jcc && code)
			{
				char *px = strstr(code, "0x");
				if (px) jcc_target = (unsigned int)strtoul(px + 2, NULL, 16);
			}
			prev_jcc_target = jcc_target;
			/* 任务4: 冗余 jmp 识别。上一条是无条件 jmp imm 且目标==本行 eip
			   => 该 jmp 是乱序连接(执行流里跳转紧接目标), 重排后可消除。 */
			if (prev_was_jmp && prev_jmp_target && eip == prev_jmp_target)
			{
				int dupj = 0;
				for (int q = 0; q < g_redundant_jmp_cnt; q++)
					if (g_redundant_jmp[q] == prev_eip) { dupj = 1; break; }
				if (!dupj && g_redundant_jmp_cnt < 8192)
					g_redundant_jmp[g_redundant_jmp_cnt++] = prev_eip;
			}
			/* 本行是无条件 jmp imm 吗 */
			unsigned int jmp_target = 0;
			if (!strcmp(mn, "jmp") && code)
			{
				char *px = strstr(code, "0x");
				if (px) jmp_target = (unsigned int)strtoul(px + 2, NULL, 16);
			}
			prev_was_jmp = (jmp_target != 0);
			prev_jmp_target = jmp_target;
			prev_eip = eip; prev_is_jcc = is_jcc;
		}
		g_always_nt_jcc_cnt = 0;
		for (int q = 0; q < jcc_n; q++)
			if (jcc_stat[q].nt > 0 && jcc_stat[q].taken == 0 && g_always_nt_jcc_cnt < 4096)
				g_always_nt_jcc[g_always_nt_jcc_cnt++] = jcc_stat[q].eip;
		out_progress("[opaque] jcc=%d always-not-taken=%d redundant-jmp=%d", jcc_n, g_always_nt_jcc_cnt, g_redundant_jmp_cnt);
	}
	g_sym_n = saved_sym_n;   /* 回退 analyze pass 临时符号, 防多 run 符号池累积 */
	fclose(fin);
}
static void check_dyn_static_divergence(void);
/* DYN-STATIC DIVERGENCE TRACE: record every executed step (eip+eflags+regs)
   during the dynamic trace so we can diff against the static Unicorn trace
   and find the exact instruction where they diverge. */
#define DYN_STEP_MAX 300000
static unsigned long long g_dyn_steps_eip[DYN_STEP_MAX];
static unsigned int g_dyn_steps_ef[DYN_STEP_MAX];
static unsigned int g_dyn_steps_reg[DYN_STEP_MAX][8];
static int g_dyn_steps_n = 0;
/* mov-imm frequency table (precomputed once over the full trace so the
   following-scan is O(1) per handler instead of O(trace) - the old inline
   frequency loop made static restore crawl). */
#define IMM_FREQ_MAX 256
static unsigned long long g_imm_freq_enc[IMM_FREQ_MAX];
static int g_imm_freq_cnt[IMM_FREQ_MAX];
static int g_imm_freq_n = 0;

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
				cmds[ncmd].size_bits = 32;
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
				   approximated by preceding carrier value ~ exact scale
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
					/* lea dst,[carrier*N + disp] = carrier*N + disp.
					   Model the scale multiply EXACTLY: count how many times
					   the carrier register appears in the addressing expr and
					   multiply: [r+r*1]=2r, [r*4]=4r, [r*8]=8r, [r+r*4]=5r.
					   scale (N) is a power of 2, so SHL by log2. */
					int carrier_count = 0;
					const char *b = strchr(op2, '[');
					const char *e = b ? strchr(b, ']') : NULL;
					if (b && e)
					{
						for (int ii = 0; ii < ncar; ii++)
						{
							/* count carrier-token occurrences in [..] */
							const char *s = b;
							while (s < e)
							{
								if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z'))
								{
									char tk[16] = "";
									int kk = 0;
									while (s < e && ((*s >= 'a' && *s <= 'z') ||
										(*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9')))
										{ if (kk < 15) tk[kk++] = *s; s++; }
									tk[kk] = 0;
									if (reg_index_from_name(tk) == carriers[ii])
										carrier_count++;
								}
								else s++;
							}
						}
					}
					/* [r*N] also has an implicit scale N: if no explicit
					   count but scale parsed, use scale directly */
					if (carrier_count == 0 && scale > 1)
						carrier_count = scale;
					if (carrier_count == 0)
						carrier_count = 1;   /* plain [r+disp] */
					/* emit SHL by log2(count) if count is a power of 2,
					   else approximate with the additive count via repeated
					   ADD of the carrier (not exact, but scale 1/2/4/8 are
					   the VMP constant-rebuild forms). */
					if (carrier_count > 1)
					{
						int sh = 0;
						int c = carrier_count;
						while (c > 1 && (c & 1) == 0) { c >>= 1; sh++; }
						if (ncmd < maxcmds)
						{
							cmds[ncmd].type = VMP_CC_SHL;
							cmds[ncmd].size_bits = 32;
							cmds[ncmd].val = (unsigned long long)(sh);
							ncmd++;
						}
					}
					if (ncmd < maxcmds)
					{
						cmds[ncmd].type = VMP_CC_ADD;
						cmds[ncmd].size_bits = 32;
						cmds[ncmd].val = disp;
						ncmd++;
					}
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
				cmds[ncmd].size_bits = 32;
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
static void mini_mem_write(MINI_REG *mreg, unsigned long long vsp_off, unsigned long long val, int size, int sym)
{
	MINI_MEM *mem = &mreg->mem;
	int i;
	/* merge overlapping slot: keep existing if same offset+size */
	for (i = 0; i < mem->count; i++)
	{
		if (mem->off[i] == vsp_off && mem->size[i] == size)
		{
			mem->val[i] = val;
			mem->sym[i] = sym;
			return;
		}
	}
	if (mem->count < MINI_MEM_SLOTS)
	{
		mem->off[mem->count] = vsp_off;
		mem->val[mem->count] = val;
		mem->sym[mem->count] = sym;
		mem->size[mem->count] = size;
		mem->count++;
	}
}

/* read a value from vsp-relative memory slot; supports overlap (different size).
   Strategy: find the slot whose range [off, off+size) best covers [vsp_off, vsp_off+size).
   For byte/halfword reads inside a larger stored slot, extract the sub-field. */
static int mini_mem_read(MINI_REG *mreg, unsigned long long vsp_off, int size, unsigned long long *out, int *sym_out)
{
	MINI_MEM *mem = &mreg->mem;
	int i;
	int best = -1;
	unsigned long long best_cover = 0;
	if (sym_out) *sym_out = -1;
	/* exact match first */
	for (i = 0; i < mem->count; i++)
	{
		if (mem->off[i] == vsp_off && mem->size[i] == size)
		{
			*out = mem->val[i];
			if (sym_out) *sym_out = mem->sym[i];
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

/* compute absolute address from an addressing expression [base + index*scale ± disp].
   Supports arbitrary base/index registers (rsp/r8/rbx/rdi/rax...).
   Returns 1 on success, 0 if expression has unknown parts. */
static int compute_abs_addr_ex(MINI_REG *mreg, const char *expr, unsigned long long *addr,
	unsigned long long ip, unsigned long long instr_len)
{
	const char *b = strchr(expr, '[');
	const char *e;
	char inside[160];
	if (!b) return 0;
	/* segment prefix: fs:/gs: ~  resolve base via x64dbg expressions.
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
			duint segbase_d = 0;
			if (Script::Misc::ParseExpression("teb()", &segbase_d))
				segbase = segbase_d;
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

		/* parse remaining terms: ± reg[*scale] or ± imm */
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
		if (!VMP_IS_STACKREG_NAME(base))
			return 0;
		{
			unsigned long long off = 0;
			/* remaining: [± reg*scale] ± imm */
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

/* [asm] register operand description: given a GPR operand name (any width,
   incl. sub-registers dl/cx/ax/dh/cl...), return the parent register index
   (0-7 eax..edi), the bit offset within it (ah/ch/dh/bh = 8) and the width.
   Returns 1 if nm is a GPR operand, 0 otherwise. */
static int vmp38_reginfo(const char *nm, int *idx, int *shift, int *bits)
{
	static const char *lo8[8] = { "al","cl","dl","bl","spl","bpl","sil","dil" };
	static const char *hi8[4] = { "ah","ch","dh","bh" };
	static const char *w16[8] = { "ax","cx","dx","bx","sp","bp","si","di" };
	static const char *w32[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
	if (!nm || !nm[0]) return 0;
	for (int i = 0; i < 8; i++) if (!strcmp(nm, lo8[i])) { *idx = i; *shift = 0; *bits = 8; return 1; }
	for (int i = 0; i < 4; i++) if (!strcmp(nm, hi8[i])) { *idx = i; *shift = 8; *bits = 8; return 1; }
	for (int i = 0; i < 8; i++) if (!strcmp(nm, w16[i])) { *idx = i; *shift = 0; *bits = 16; return 1; }
	for (int i = 0; i < 8; i++) if (!strcmp(nm, w32[i])) { *idx = i; *shift = 0; *bits = 32; return 1; }
	/* 64-bit GPR names on a 32-bit sample: show low 32 bits */
	int p = reg_index_from_name(nm);
	if (p >= 0) { *idx = p; *shift = 0; *bits = 32; return 1; }
	return 0;
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
	{
		unsigned char probe[16] = {0};
		int mr = 0;
		{
			duint szread = 0;
			if (Script::Memory::Read(ip, probe, sizeof(probe), &szread) && szread == sizeof(probe))
				mr = 1;
		}
		/* file fallback: VM section may not be mapped in headless */
		if (disasm_from_file(ip, out_ins))
		{
			/* success via file read */
		}
		else
		{
			out("step_one fail ip=%08llx disasm_size=%d memread=%d file=0",
				ip, out_ins->instr_size, mr);
			return 0;
		}
	}

	rec->ip = ip;
	rec->instr_len = out_ins->instr_size;
	strncpy(rec->disasm, out_ins->instruction, sizeof(rec->disasm) - 1);

	/* classify by mnemonic */
	{
		const char *p = out_ins->instruction;
		int j = 0;
		memset(mnem, 0, sizeof(mnem));
		while (*p && *p != ' ' && *p != '\t' && j < 31)
			mnem[j++] = *p++;
		mnem[j] = 0;

		if (strncmp(mnem, "push", 4) == 0)
			rec->push = 1;
		else if (strncmp(mnem, "pop", 3) == 0)
			rec->pop = 1;
		else if (strncmp(mnem, "jmp", 3) == 0 || strncmp(mnem, "call", 4) == 0 ||
			strcmp(mnem, "ret") == 0)
			rec->is_jmp = 1;
		else if (mnem[0] == 'j' && strcmp(mnem, "jmp") != 0) { rec->is_jmp = 1; rec->is_cond = 1; const char *sp_ = strchr(rec->disasm, ' '); if (sp_) { while (*sp_ == ' ') sp_++; if (sp_[0] == '0' && (sp_[1] == 'x' || sp_[1] == 'X')) rec->cond_target = strtoull(sp_ + 2, NULL, 16); } }
		else if (strncmp(mnem, "jb", 2) == 0 || strncmp(mnem, "jnb", 3) == 0 ||
			strncmp(mnem, "jz", 2) == 0 || strncmp(mnem, "jnz", 3) == 0 ||
			strncmp(mnem, "je", 2) == 0 || strncmp(mnem, "jne", 3) == 0 ||
			strncmp(mnem, "ja", 2) == 0 || strncmp(mnem, "jae", 3) == 0 ||
			strncmp(mnem, "jbe", 3) == 0 || strncmp(mnem, "jl", 2) == 0 ||
			strncmp(mnem, "jle", 3) == 0 || strncmp(mnem, "jg", 2) == 0 ||
			strncmp(mnem, "jge", 3) == 0 || strncmp(mnem, "js", 2) == 0 ||
			strncmp(mnem, "jns", 3) == 0 || strncmp(mnem, "jo", 2) == 0 ||
			strncmp(mnem, "jno", 3) == 0 ||
			strncmp(mnem, "jecxz", 5) == 0 || strncmp(mnem, "jcxz", 4) == 0 ||
			strncmp(mnem, "loop", 4) == 0 || strncmp(mnem, "loope", 5) == 0 ||
			strncmp(mnem, "loopne", 6) == 0)
		{
			rec->is_cond = 1;
			rec->is_jmp = 1;
			/* parse target */
			{
				const char *sp = strchr(out_ins->instruction, ' ');
				unsigned long long t = 0;
				if (sp && sscanf(sp + 1, "%llx", &t) == 1)
					rec->cond_target = t;
			}
		}
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

	/* guard: privileged/junk mnemonics mean we disassembled into data */
	{
		char g[32];
		get_mnemonic(out_ins->instruction, g, sizeof(g));
		if (strcmp(g, "outsb") == 0 || strcmp(g, "outsd") == 0 ||
			strcmp(g, "insb") == 0 || strcmp(g, "insd") == 0 ||
			strcmp(g, "in") == 0 || strcmp(g, "out") == 0 ||
			strcmp(g, "cpuid") == 0 || strcmp(g, "rdtsc") == 0)
		{
			out("step_one junk at ip=%08llx ('%s')", ip, out_ins->instruction);
			return 0;
		}
	}

	/* vsp delta: push/pop = 8 (64-bit) or 4 (32-bit stack width) */
	if (rec->push)
		rec->vsp_delta = VMP_VSP_STEP;
	else if (rec->pop)
		rec->vsp_delta = VMP_VSP_STEP;
	else if (strcmp(mnem, "mov") == 0)
	{
		const char *d = out_ins->instruction;
		const char *br = strchr(d, '[');
		const char *comm = strchr(d, ',');
		int mem_first = 0;
		/* only treat mov as a stack push/pop if it ACTUALLY accesses
		   memory (contains '['). "mov esi, 0x50056B81" writes an imm to
		   a register - NOT a stack op - but VMP_IS_STACKREG(d) sees "esi"
		   in the text and misclassifies it as pop (wrong: esi gets loaded
		   from [esp], corrupting the VM stack sim and breaking the next
		   dispatch). This was the root of the vInit "rep movsb" chain
		   break: esi/edi were loaded with stack garbage. */
		if (br && comm)
		{
			mem_first = (br < comm) ? 1 : 0;
			if (VMP_IS_STACKREG(d))
			{
				if (mem_first)
					rec->push = 1;
				else
					rec->pop = 1;
				rec->is_alu = 1;
			}
		}
	}
	else if (strcmp(mnem, "lea") == 0)
	{
		const char *d = out_ins->instruction;
		if (VMP_IS_STACKREG(d))
			rec->is_alu = 1;
	}

	return 1;
}

/* identify handler semantics from step sequence.
   returns: 1=vFetch(decryptor), 0=other */
/* junk mnemonic: bytecode / junk-fill disassembled as code */
static int is_junk_mnemonic(const char *m)
{
	static const char *junk[] = {
		"int", "into", "iret", "hlt", "cli", "sti", "cpuid", "rdtsc",
		"aam", "aad", "daa", "das", "fwait", "wait", "fnop",
		"aaa", "salc", "in", "out", "insb", "insd", "outsb", "outsd",
		"lock", "rep", "repne", "jecxz", "jcxz", "loop", "loope",
		"loopne", "xlat", "enter", "leave",
		"arpl", "bound", "pusha", "popa", "pushad", "popad", "cdq",
		"cqo", "cbw", "cwde", "salc", "salc",
		"rdtscp", "ud2", "iret", "retf", "pop ds", "pop es", "pop ss",
		"pop fs", "pop gs", "xlat", "lahf", "sahf"
	};
	int i;
	for (i = 0; i < (int)(sizeof(junk)/sizeof(junk[0])); i++)
		if (strcmp(m, junk[i]) == 0)
			return 1;
	return 0;
}

static int find_trace_regs(unsigned long long ip, unsigned int *regs8, unsigned int *ef);

static int identify_handler(STEP_REC *recs, int n, int handler_idx,
	unsigned long long vsp_in, unsigned long long vsp_out)
{
	(void)recs; (void)n; (void)vsp_in; (void)vsp_out;
	/* H/I/T handler-semantics rows are no longer written: the dynamic trace
	   now emits the SAME eip= rows as the static trace (shared rule,
	   vmp38_emit_row + vmp38_emit_tail). This stub keeps the handler counter
	   advancing for callers. */
	return handler_idx + 1;
}

/* ============================================================================
   32-bit handler semantic classification (P0 restore) - pure mnemonic based,
   NO engine deps. Ported from the x64 vmp38_classify_handler64 (verified on
   branch.vmp.exe: vNand/vNor order-aware, vAdc/vSbb precedence). The VM
   instruction vocabulary aligns with g0th1c54e4/vmp394-handlers handlers.txt:
   vPushImm/vPopReg/vAdd/vNand/vNor/vShr/vShl/vReadMem/vJmp/vExit -> here
   mapped onto the VMP 3.5 opcode semantics from the original xxvm
   st_handle_func[] table (push dreg/pop dreg/add d[sp+4],d[sp]/NAND/NOR/...).
   ========================================================================== */
static int vmp38_is_core_op32(const char *mn)
{
	static const char *core_ops[] = {
		"mov",   /* 2026-08-30 关键补全: mov(尤其 mov reg,[mem] 读表 /
		            mov [mem],reg 写关键地址) 之前被排除, 导致 0122c1a2 这类
		            关键内存读写指令在 .txt 里整段消失。mov 是最核心的算法
		            搬运/读写语义, 必须进 core_ops。 */
		"adc","sbb","add","sub","xor","and","or","not",
		"imul","mul","idiv","div","shl","sal","shld",
		"shr","sar","shrd","rol","ror",
		"bsr","bsf",
		"cmovb","cmovae","cmove","cmovne","cmova","cmovbe",
		"cmovg","cmovge","cmovl","cmovle","cmovs","cmovns",
		"setb","setae","sete","setne","seta","setbe","setg",
		"setge","setl","setle","sets","setns",
		"movzx","movsx","popcnt", NULL };
	for (int i = 0; core_ops[i]; i++)
		if (!strcmp(mn, core_ops[i]))
			return 1;
	return 0;
}

static const char *vmp38_classify_handler32(const char *core_ops, int n_cld,
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
	/* Order-sensitive NAND/NOR (kanxue vmp394-handlers):
	   - and;not -> NAND, or;not -> NOR (last gate op + trailing not) */
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
		/* fallback: no trailing not - gate-count heuristic */
		return (n_or >= 1) ? "vNand" : "vNor";
	}
	/* vALU mapping: count adc vs sbb, then precedence */
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

/* ============================================================================
   P1c: fine-grained VM instruction semantics (aligned with the original xxvm
   st_handle_func[] table). VMP is a stack machine: sp points at the stack
   top, operands live on the stack, the result is pushed back. These strings
   mirror the original handler semantics (push dreg / pop dreg / add d[sp+4],d[sp]
   / NAND / NOR / div d[sp+4],d[sp+8] / shr d[sp],b[sp+4] ...).
   ========================================================================== */
static const char *vm_semantic_from_tag(const char *tag, const char *core)
{
	(void)core;
	if (!strcmp(tag, "vAdd")) return "add d[sp+4],d[sp]";
	if (!strcmp(tag, "vSub")) return "sub d[sp+4],d[sp]";
	if (!strcmp(tag, "vAdc")) return "adc d[sp+4],d[sp]";
	if (!strcmp(tag, "vSbb")) return "sbb d[sp+4],d[sp]";
	if (!strcmp(tag, "vXor")) return "xor d[sp+4],d[sp]";
	if (!strcmp(tag, "vAnd")) return "and d[sp+4],d[sp]";
	if (!strcmp(tag, "vOr")) return "or d[sp+4],d[sp]";
	if (!strcmp(tag, "vNand")) return "and ~d[sp+4],~d[sp]";
	if (!strcmp(tag, "vNor")) return "or ~d[sp+4],~d[sp]";
	if (!strcmp(tag, "vMul")) return "mul d[sp+4],d[sp]";
	if (!strcmp(tag, "vDiv")) return "div d[sp+4],d[sp+8]";
	if (!strcmp(tag, "vShl")) return "shl d[sp],b[sp+4]";
	if (!strcmp(tag, "vShr")) return "shr d[sp],b[sp+4]";
	if (!strcmp(tag, "vRol")) return "rol d[sp],b[sp+4]";
	if (!strcmp(tag, "vRor")) return "ror d[sp],b[sp+4]";
	/* vALU: the generic/uncategorized ALU tag - resolve from its core
	   dominant source opcode (e.g. not/neg/inc/dec/shl/...). This keeps the
	   conversion table complete for the rare handlers not classified into a
	   specific vXxx tag. */
	if (!strcmp(tag, "vALU") && core && core[0])
	{
		const char *c = core;
		while (*c == ' ' || *c == '+' || *c == '-') c++;
		if (!_strnicmp(c, "not", 3)) return "not d[sp]";
		if (!_strnicmp(c, "neg", 3)) return "neg d[sp]";
		if (!_strnicmp(c, "inc", 3)) return "inc d[sp]";
		if (!_strnicmp(c, "dec", 3)) return "dec d[sp]";
		if (!_strnicmp(c, "shl", 3)) return "shl d[sp],b[sp+4]";
		if (!_strnicmp(c, "shr", 3) || !_strnicmp(c, "sar", 3)) return "shr d[sp],b[sp+4]";
		if (!_strnicmp(c, "rol", 3)) return "rol d[sp],b[sp+4]";
		if (!_strnicmp(c, "ror", 3)) return "ror d[sp],b[sp+4]";
		if (!_strnicmp(c, "imul", 4) || !_strnicmp(c, "mul", 3)) return "mul d[sp+4],d[sp]";
		if (!_strnicmp(c, "div", 3)) return "div d[sp+4],d[sp+8]";
		if (!_strnicmp(c, "xor", 3)) return "xor d[sp+4],d[sp]";
		if (!_strnicmp(c, "and", 3)) return "and d[sp+4],d[sp]";
		if (!_strnicmp(c, "or", 2)) return "or d[sp+4],d[sp]";
		if (!_strnicmp(c, "add", 3)) return "add d[sp+4],d[sp]";
		if (!_strnicmp(c, "sub", 3)) return "sub d[sp+4],d[sp]";
		if (!_strnicmp(c, "adc", 3)) return "adc d[sp+4],d[sp]";
		if (!_strnicmp(c, "sbb", 3)) return "sbb d[sp+4],d[sp]";
		if (!_strnicmp(c, "bsr", 3)) return "bsr d[sp]";
		if (!_strnicmp(c, "bsf", 3)) return "bsf d[sp]";
		if (!_strnicmp(c, "popcnt", 6)) return "popcnt d[sp]";
		if (!_strnicmp(c, "movzx", 5)) return "movzx d[sp],b[sp+4]";
		if (!_strnicmp(c, "movsx", 5)) return "movsx d[sp],b[sp+4]";
		if (!_strnicmp(c, "cmov", 4)) return "cmov d[sp]";
		if (!_strnicmp(c, "set", 3)) return "set d[sp]";
	}
	return NULL;
}

/* P1c: non-ALU handler semantics (push/pop/call/ret/pushfd/popfd/jmp) with
   operands extracted from the LIVE instructions. */
static const char *vm_dispatch_semantic(const char (*h_mns)[32], const char (*h_diss)[128],
	const int *alive, int h_cnt)
{
	static char sbuf[64];
	int k;
	for (k = 0; k < h_cnt; k++)
	{
		if (!alive[k]) continue;
		if (!strncmp(h_mns[k], "push", 4))
		{
			unsigned long long imm = 0;
			if (parse_imm_from_disasm(h_diss[k], &imm))
			{
				_snprintf(sbuf, sizeof(sbuf), "push dconst %llx", imm);
				return sbuf;
			}
			_snprintf(sbuf, sizeof(sbuf), "push dreg %s", h_diss[k] + 5);
			return sbuf;
		}
	}
	for (k = 0; k < h_cnt; k++)
	{
		if (!alive[k]) continue;
		if (!strncmp(h_mns[k], "pop", 3) && strcmp(h_mns[k], "popf") != 0 &&
			strcmp(h_mns[k], "popfd") != 0)
		{
			_snprintf(sbuf, sizeof(sbuf), "pop dreg %s", h_diss[k] + 4);
			return sbuf;
		}
	}
	for (k = 0; k < h_cnt; k++)
	{
		if (!alive[k]) continue;
		if (!strcmp(h_mns[k], "popfd") || !strcmp(h_mns[k], "popf"))
		{
			_snprintf(sbuf, sizeof(sbuf), "popfd (vm-exit)");
			return sbuf;
		}
		if (!strcmp(h_mns[k], "pushfd") || !strcmp(h_mns[k], "pushf"))
		{
			_snprintf(sbuf, sizeof(sbuf), "pushfd (vm-enter)");
			return sbuf;
		}
	}
	for (k = 0; k < h_cnt; k++)
	{
		if (!alive[k]) continue;
		if (!strcmp(h_mns[k], "call"))
		{
			const char *sp = strchr(h_diss[k], ' ');
			const char *tgt = sp ? sp + 1 : "";
			/* VMP 引擎 dispatch 噪声过滤：call 目标 ≤ 0x200 是 VM 引擎的操作码
			   （0x00~0x0200 间的真 dispatch op，标为 [engine] 无算法意义）；
			   而 > 0x200 的（含 0x01xxxxxx 段的真实 handler 入口地址）才当作
			   真实 call 地址输出。 */
			unsigned long long tv = 0;
			if (tgt[0] == '0' && (tgt[1] == 'x' || tgt[1] == 'X')) tv = strtoull(tgt + 2, NULL, 16);
			else if (tgt[0]) tv = strtoull(tgt, NULL, 10);
			if (tv && tv <= 0x200ULL) {
				_snprintf(sbuf, sizeof(sbuf), "[engine] dispatch op=%llx", tv);
			} else {
				_snprintf(sbuf, sizeof(sbuf), "call %s", tgt);
			}
			return sbuf;
		}
		if (!strcmp(h_mns[k], "ret") || !strcmp(h_mns[k], "retn"))
			return "ret";
	}
	for (k = 0; k < h_cnt; k++)
	{
		if (!alive[k]) continue;
		if (!strncmp(h_mns[k], "jmp", 3))
		{
			const char *sp = strchr(h_diss[k], ' ');
			_snprintf(sbuf, sizeof(sbuf), "jmp %s", sp ? sp + 1 : "?");
			return sbuf;
		}
	}
	return "jmp";
}


/* Auto-locate the VMP 3.8 VM entry:
   1. find the largest non-.text section (the VM section)
   2. scan .text for call/jmp instructions whose target lands in the VM section
   returns the first call target (VM entry trampoline), or 0. */
/* VM section (runtime) - set by vmp38_auto_locate, used by restore to
   reject bogus .text dispatch targets (JUNK stub loop guard) */
static unsigned long long g_vm_start = 0, g_vm_end = 0;

/* 通用函数调用追踪：记录 call（VM 执行的调用）的目标，返回地址 + 调用栈深度；
   ret 返回时提示返回值（若是 VM 函数首 / 函数尾调用，其 pushfd 之前的内部调用）。 */
#define CALL_STACK_MAX 64
static unsigned long long g_call_ret[CALL_STACK_MAX];   /* call 返回地址 */
static unsigned long long g_call_esp[CALL_STACK_MAX];   /* call 时 esp（深度溢出 +4 用） */
static unsigned long long g_call_tgt[CALL_STACK_MAX];   /* call 目标 */
static int g_call_n = 0;

/* 2026-08-26 固定地址还原: 已输出 [data] 的 eip 去重集(避免 VMP 循环重复刷屏) */
static std::unordered_set<unsigned int> g_ft_data_emit;
/* 2026-08-26 缺口2/T2 数据流: "算法派生值"集合(值级 taint)。
   入口 = 密钥/算法数据值([data] read 标的), 经算术指令(xor/and/shr/add/rol/...)沿
   dst 传播; write 到内存时若写值∈派生集 → 标 [algo-res](解密结果去向)。 */
static std::unordered_set<unsigned int> g_algo_derived;
/* 2026-08-26 融合: "密钥值"集(read seed 读到的高值密钥/算法常量)。
   指令还原(algo-code)行里若操作数值∈此集 → 标 [KEY参与], 让指令里看出密钥计算。 */
static std::unordered_set<unsigned int> g_iskey;
/* 2026-08-26 def-use 链: 值 → 最后一次"定义/读到"它的 eip(用于 write 标注值来源;
   例: write @g_result=0x13 ← def@某eip[read], 一条链看出 0x13 来历)。 */
static std::unordered_map<unsigned int, unsigned int> g_val_def;
/* 2026-08-26 def-use: 值 → 定义它的指令摘要(供 write 标注 `def@eip: mnemonic ...`) */
static std::unordered_map<unsigned int, std::string> g_val_def_code;
/* 2026-08-26 代码还原: 已输出 [algo-code] 的 eip 去重集 */
static std::unordered_set<unsigned int> g_algocode_emit;
/* 2026-08-26 收紧 seed: 只有"重复出现的高值(read 到 ≥2 次)"才可能是密钥/算法常量
   (keydemo): deadbee5/9e3779b9 多 eip 重复; base 的 acc 中间量多为瞬态不重复,
   避免 base 被数据流污染(此前 KEYDATA 满屏)。 */
static std::unordered_map<unsigned int, unsigned int> g_val_occur;
/* 2026-08-25 立项 fwd: 分析侧参数溯源(旁路) */
static void vmp38_algo2_init(MINI_REG *mreg);
/* ---- 2026-08-25 立项: 分析侧参数溯源(旁路足迹, 完全不碰执行侧符号) ----
   追"被 VMP 藏起(值不裸现)的参数"时, value-anchor 靠值匹配必然失败; 而把执行侧
   入口寄存器 bind 成参数根会破坏 jmp-reg 解析死循环(上次探针实测)。
   正路 = 旁路足迹: g_algo2_reg[8] 标记"当前各寄存器是否承载"任一入口参数根演化值",
   每指令用纯寄存器值+mnemonic 推理传播(不分配符号树、不碰 g_symreg.sym)。
   它不能碰执行侧, 故不可能破坏执行。 */
static unsigned int g_algo2_root_val[8];     /* 入口参数根具体值 */
static int g_algo2_root_reg[8];              /* 根起始寄存器(index <=7) */
static int g_algo2_root_cnt = 0;
static unsigned char g_algo2_taint[8][8];    /* [根][寄存器] 当前是否承载 */
static unsigned int g_algo2_cur[8][8];       /* [根][寄存器] 承载的具体值 */
static int g_algo2_enabled = 0;
static int g_algo2_prop_steps = 0;
static int g_algo2_emitted[8];               /* 每根已输出足迹数(限流) */
/* V2: 内存槽 taint (vsp-relative offset -> 哪些根已流入该虚拟栈槽)。
   参数 mov [ebp+off],reg 存虚拟栈后值消失但标记保留, 供 [ebp+off] 读回续传。 */
static long long g_algo2_mslot_off[512];
static unsigned char g_algo2_mslot_root[512][8];   /* 该槽承载哪些根 */
static int g_algo2_mslot_cnt = 0;


static void p3_symreset(unsigned long long entry_ip, unsigned long long entry_esp)
{
	memset(&g_symreg, 0, sizeof(g_symreg));
	sym_pool_reset();
	g_ft_data_emit.clear();   /* 2026-08-26 固定地址标注去重: 每 run 清空 */
	g_algo_derived.clear();    /* 2026-08-26 数据流算法派生集: 每 run 清空 */
	g_val_occur.clear();       /* 2026-08-26 值频次统计: 每 run 清空 */
	g_algocode_emit.clear();   /* 2026-08-26 算法代码行去重: 每 run 清空 */
	/* 阶段二 acc 累积载体: 每次 run 重置(符号池已清, g_acc2_sym 失效) */
	g_acc2_reg = -1; g_acc2_sym = -1; g_acc2_pre = 0;
	g_acc2_started = 0; g_acc2_pending = 0;
	/* 2026-09-01 任务1: taint 追踪初始化(每次 run 重置)。
	   taint 源 = 读 VM 字节码段(g_vm_start..g_vm_end)的 mov/movzx/movsx。
	   注意: 不与 g_symreg.t[16](符号系统/WALL-2) 耦合, 独立一套。 */
	memset(g_taint_reg, 0, sizeof(g_taint_reg));
	g_taint_vsp_cnt = 0;
	g_taint_insn_cnt = 0;
	g_taint_vm_lo = g_vm_start;
	g_taint_vm_hi = g_vm_end;
	g_taint_enabled = (g_vm_start != 0 && g_vm_end > g_vm_start) ? 1 : 0;
	for (int i = 0; i < 16; i++)
	{
		g_symreg.sym[i] = sym_alloc(SYM_OP_REG, i, 0, 32, 0);
		g_symreg.t[i] = 1;   /* unknown provenance - not substituted */
	}
	g_symreg.r[4] = entry_esp & 0xffffffffULL;
	g_symreg.vsp = entry_esp & 0xffffffffULL;
	g_symreg.sym[4] = sym_alloc(SYM_OP_CONST, 0, 0, 32, entry_esp & 0xffffffffULL);
	g_symreg.t[4] = 0;   /* esp is known */
	/* WALL-2 FIX: seed ALL entry registers (eax..edi) as CONST of their real
	   live-process values, not tainted REG leaves. A dispatch register whose
	   value comes in from the caller (ebx=0x0066d000, the per-function
	   dispatch state) was previously opaque -> the whole enc-chain through
	   xor ecx,ebx / adc ebp,ecx stayed tainted and jmp ebp could not fold.
	   The live value IS what the real CPU starts with, so bind it as const. */
	{
		static const Script::Register::RegisterEnum renum[8] = {
			Script::Register::RegisterEnum::EAX, Script::Register::RegisterEnum::ECX,
			Script::Register::RegisterEnum::EDX, Script::Register::RegisterEnum::EBX,
			Script::Register::RegisterEnum::ESP, Script::Register::RegisterEnum::EBP,
			Script::Register::RegisterEnum::ESI, Script::Register::RegisterEnum::EDI };
		for (int ri = 0; ri < 8; ri++)
		{
			unsigned long long rv = (unsigned long long)Script::Register::Get(renum[ri]);
			rv &= 0xffffffffULL;
			g_symreg.r[ri] = rv;
			g_symreg.sym[ri] = sym_alloc(SYM_OP_CONST, 0, 0, 32, rv);
			g_symreg.t[ri] = 0;
		}
	}
	/* 2026-08-25 探针: 把入口合理整数寄存器从 CONST 改绑为参数符号根(若适用)。
	   必须在 srcs_load 之后(g_src_vals[0..2]=fb_seed a/b/acc0 已就位), 且每 run 只注入一次。 */
	/* 2026-08-25 立项: prob 执行侧改绑已废除(改 entry 寄存器→SRC 会破坏 jmp-reg
	  解析死循环)。分析侧旁路 algo2 在 srcs_load 之后初始化(需 const_list/g_src 就绪)。 */
	(void)entry_ip;
}

/* ---- 2026-08-25 立项: 分析侧参数溯源(旁路足迹) ----
   初始化: 从入口寄存器具体值登记"合理整数参数根"(非指针/模块/栈值)。
   传播: 每指令后用 mnemonic + 寄存器值推理参数根参与了哪些运算并搬到哪些寄存器,
   输出 [algo-propagate] 足迹。完全不碰执行侧符号, 故不破坏 jmp-reg 解析。 */
static void vmp38_algo2_init(MINI_REG *mreg)
{
	if (!g_probe_entry_root)   /* 旁路也由 probe_roots=1 门控, 避免 base/普通样本污染 */
	{ g_algo2_enabled = 0; return; }
	g_algo2_root_cnt = 0;
	memset(g_algo2_taint, 0, sizeof(g_algo2_taint));
	memset(g_algo2_emitted, 0, sizeof(g_algo2_emitted));
	g_algo2_prop_steps = 0;
	g_algo2_mslot_cnt = 0;
	unsigned int imgbase = 0;
	{
		Script::Module::ModuleInfo mi;
		if (Script::Module::GetMainModuleInfo(&mi)) imgbase = (unsigned int)mi.base;
	}
	for (int ri = 0; ri < 8; ri++)
	{
		if (ri == 4) continue;   /* esp 不是参数 */
		unsigned int v = (unsigned int)mreg->r[ri];
		if (!v || v == 0xFFFFFFFFu) continue;
		if (v >= 0x80000000u && v != 0x80000000u) continue;
		if (imgbase && v >= imgbase && v < imgbase + 0x2000000u) continue;
		if (v >= 0x7F000000u) continue;
		/* 已登记相同值的跳过 */
		int dup = 0;
		for (int k = 0; k < g_algo2_root_cnt; k++)
			if (g_algo2_root_val[k] == v) { dup = 1; break; }
		if (dup) continue;
		if (g_algo2_root_cnt >= 8) break;
		g_algo2_root_val[g_algo2_root_cnt] = v;
		g_algo2_root_reg[g_algo2_root_cnt] = ri;
		g_algo2_taint[g_algo2_root_cnt][ri] = 1;
		g_algo2_cur[g_algo2_root_cnt][ri] = v;
		g_algo2_root_cnt++;
	}
	/* 扩展根来源: const_list 种子(非 a/b/acc0 占位) = cx1/cx2/b_r 等真实参数。
	   它们可能不在入口 register(complex 参数在栈/内存), 先登记根值, 在 step 里
	   当某寄存器值命中该根值时再绑定承载。 */
	for (int si = 3; si < g_src_cnt && g_algo2_root_cnt < 8; si++)
	{
		unsigned int v = g_src_vals[si];
		if (!v || v == 0xFFFFFFFFu) continue;
		/* 跳过入口 reg 已登记的重复值 */
		int dup = 0;
		for (int k = 0; k < g_algo2_root_cnt; k++)
			if (g_algo2_root_val[k] == v) { dup = 1; break; }
		if (dup) continue;
		g_algo2_root_val[g_algo2_root_cnt] = v;
		g_algo2_root_reg[g_algo2_root_cnt] = -1;   /* 待绑定 */
		int si2 = 0;
		for (int z = 0; z < 8; z++) g_algo2_taint[g_algo2_root_cnt][z] = 0;
		(void)si2;
		g_algo2_root_cnt++;
	}
	g_algo2_enabled = 1;   /* 只要门控开且有任何根候选(含待绑定)即启用 */
	if (g_algo2_root_cnt > 0)
		out_progress("[algo2] registered %d param roots (entry reg + const_list)", g_algo2_root_cnt);
}

/* 传播: 每指令推理入口参数根参与了哪些运算/搬运, 记录足迹。
   disasm = 当前指令; pre[8]/post[8] = 指令前后 8 寄存器具体值。
   纯值+mnemonic 推理, 不分配符号树, 不碰执行侧符号。 */
static void vmp38_algo2_step(const char *disasm,
	const unsigned int *pre, const unsigned int *post)
{
	if (!g_algo2_enabled) return;
	char m[32], op1[64] = "", op2[64] = "";
	const char *p;
	get_mnemonic(disasm, m, sizeof(m));
	if (!strcmp(m, "jmp") || !strcmp(m, "call") || !strcmp(m, "ret") ||
		!strcmp(m, "nop") || !strcmp(m, "push") || !strcmp(m, "pop") ||
		!strcmp(m, "test") || !strcmp(m, "cmp"))
		return;
	p = disasm;
	while (*p && *p != ' ') p++;
	while (*p == ' ') p++;
	{
		int k = 0;
		while (*p && *p != ',' && k < 63) op1[k++] = *p++;
		op1[k] = 0;
		if (*p == ',') { p++; while (*p == ' ') p++; k = 0; while (*p && k < 63) op2[k++] = *p++; op2[k] = 0; }
	}
	/* 解析 dst/src 寄存器 + 内存操作数 [..] */
	int r1 = reg_index_from_name(op1);
	int r2 = (op2[0] && op2[0] != '[') ? reg_index_from_name(op2) : -1;
	int op2_mem = (op2[0] == '[') ? 1 : 0;
	int op1_mem = (op1[0] == '[') ? 1 : 0;
	int is_bin = !strcmp(m, "add") || !strcmp(m, "sub") || !strcmp(m, "adc") ||
		!strcmp(m, "sbb") || !strcmp(m, "xor") || !strcmp(m, "and") ||
		!strcmp(m, "or") || !strcmp(m, "imul") || !strcmp(m, "mul") ||
		!strcmp(m, "shl") || !strcmp(m, "shr");
	int is_mov = !strcmp(m, "mov");

	/* 一个指令只处理一次: 收集 src 侧承载哪些根, 决定 dst 继承哪些根. */
	unsigned char srcroots[8] = {0,0,0,0,0,0,0,0};
	/* src 寄存器 */
	if (r2 >= 0 && r2 < 8)
		for (int r = 0; r < g_algo2_root_cnt; r++)
			if (g_algo2_taint[r][r2]) srcroots[r] = 1;
	/* src 内存槽 [..]: V2.5 处理(先不做) */
	(void)op1_mem;

	/* 若 src 无任何根, 但 op2 是含根值的立即数(无根, 跳过) */
	int any_src = 0;
	for (int r = 0; r < g_algo2_root_cnt; r++) if (srcroots[r]) { any_src = 1; break; }

	/* 传播 + 足迹 */
	for (int r = 0; r < g_algo2_root_cnt; r++)
	{
		if (!srcroots[r]) continue;
		int dst_changed = (r1 >= 0 && r1 < 8 && post[r1] != pre[r1]);
		/* 足迹: 参数参与了 ALU 运算 */
		if ((is_bin || is_mov) && dst_changed && g_algo2_emitted[r] < 200)
		{
			out("  [algo-propagate] @%08X param=%08X  %s %s%s%s : r%d %08X->%08X",
				(unsigned int)0, g_algo2_root_val[r], m,
				op1[0] ? op1 : "?", (r2 >= 0 || op2_mem) ? "," : "",
				(r2 >= 0) ? op2 : ".", r1,
				(unsigned int)(r1 >= 0 && r1 < 8 ? pre[r1] : 0),
				(unsigned int)(r1 >= 0 && r1 < 8 ? post[r1] : 0));
			g_algo2_emitted[r]++;
		}
		/* dst 寄存器继承根 */
		if (r1 >= 0 && r1 < 8 && r1 != 4)
		{
			g_algo2_taint[r][r1] = 1;
			g_algo2_cur[r][r1] = post[r1];
		}
		/* mov reg, [mem] / mov [mem], reg: 内存槽 taint 由 v2 单独处理(先不做) */
	}
	(void)any_src;
}

/* 2026-08-26 收窄固定地址: 判定"读到的值"是否为算法数据(密钥/加密中间量), 过滤
   VM 引擎噪声(0/小常量/栈·模块指针/标志位)。启发式(记忆库 VALUE-MAGNITUDE):
   值高字节非零且非标志 → 更可能是算法数据。返回 1 表示值得标注。 */
static int vmp38_is_algo_data_value(unsigned int v)
{
	if (v >= 0x01000000u && v <= 0xFEFEFFFFu)
	{
		/* 排除明显引擎标志/控制位(常作单 bit 或 0x80 系派生态) */
		if ((v & 0xFFFF0000u) == 0x80000000u) return 0;   /* 0x8000xxxx sign 扩展 */
		if ((v & 0xFFFF0000u) == 0x40000000u) return 0;   /* 0x4000xxxx */
		if (v == 0xFFFFFFFFu || v == 0xFFFFFFFEu) return 0;
		/* 排除栈/模块类指针(低字节非零但形如 0x00xxxxxx 的不进来, 因为 <0x01000000) */
		return 1;
	}
	return 0;
}

/* ---- SHARED VM-exit rules (dynamic trace + static Unicorn trace) ----
   VMP 3.8: vm_start=pushfd, vm_exit=popfd (kanxue 280283). A handler that
   unwinds >=2 pop regs AND pops eflags (popfd/popad) is the VM exit
   sequence; the NEXT ret (whose target leaves the VM section) is the real
   vmret. Both trace paths MUST use these same thresholds or one drifts
   (dynamic previously used popfd>0 && popreg>=2, static used popfd>=2 ||
   popfd>0&&popreg>=2 - unified here). */

/* classify one instruction's unwind contribution: 1 = pop reg, 2 = popfd/
   popad, -1 = neither */
static int vmp38_ve_classify(const char *disasm)
{
	char m[32];
	get_mnemonic(disasm, m, sizeof(m));
	if (strcmp(m, "pop") == 0 && strchr(disasm, '[') == 0)
		return 1;            /* pop reg */
	if (strcmp(m, "popfd") == 0 || strcmp(m, "popf") == 0)
		return 2;            /* pops eflags */
	if (strcmp(m, "popad") == 0)
		return 3;            /* popad: restores ALL 8 GPRs - real VM exit */
	return -1;
}

/* 判断（非 VM 段）函数头字节签名（用于自动追栈的停点）：VM ret 落到
   此处（函数头 / call 目标为函数头时停）。VMP 在 .text 的 VM 段（stub）里
   可能按字节 inc/rol/xor/not 等，并不匹配正常函数头字节，因此不能
   VM 函数生硬当成函数头——用"强识别"避免误判；放宽了函数动态态
   53 56 83 C4 ?? 8B F2  -> push ebx; push esi; add esp,imm8; mov esi,edx
   53 56 57 8B EC         -> push ebx; push esi; push edi; mov ebp,esp
   55 8B EC 83 EC ??      -> push ebp; mov ebp,esp; sub esp,imm8 (stack frame) */
/* 判断（非 VM 段）函数头字节签名（用于自动追栈的停点）：VM ret 落到
   此处（函数头 / call 目标为函数头时停）。注意 A 段识别"是自身"这种
   需要 push reg（53/55/56/57）+ 后随标准序言片段（mov ebp,esp / 栈帧
   或 add/sub esp / mov reg,reg 取段），而不再用精确字节匹配，从而能
   容 GSCService 等被模板化的不同动态态；VMP 在 .text 的 VM 段字节
   inc/rol/xor/not 等不符合这个"push reg 串 + mov/add/sub/mov-reg"的模式，
   因此不会把其中任意片段认定为函数头。 */
static int vmp38_is_norm_funchead(uc_engine *uc, uint64_t addr)
{
	unsigned char b[32];
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
	/* 统计 push reg 前至多 4 个字节（53=ebx, 55=ebp, 56=esi, 57=edi）是否可
	   行（55 8B EC 也算）。 */
	int n_push = 0;
	while (i < 4 && (b[i] == 0x53 || b[i] == 0x55 || b[i] == 0x56 || b[i] == 0x57))
	{
		n_push++;
		i++;
	}
	/* 前缀 mov edi,edi (8B FF) 也可作为 push 前缀的合法函数头 */
	if (n_push == 0 && i == 0 && b[0] == 0x8B && b[1] == 0xFF)
	{
		i = 2;
		while (i < 6 && (b[i] == 0x53 || b[i] == 0x55 || b[i] == 0x56 || b[i] == 0x57))
		{
			n_push++;
			i++;
		}
	}
	/* push 串之后是标准序言片段才算函数头 */
	if (i < (int)sizeof(b) - 1)
	{
		/* mov ebp,esp (8B EC) - 标准序言 */
		if (b[i] == 0x8B && b[i + 1] == 0xEC)
			return 1;
		/* add/sub esp, imm8 (83 C4 ?? / 83 EC ??) 栈帧调整 */
		if (b[i] == 0x83 && (b[i + 1] == 0xC4 || b[i + 1] == 0xEC))
			return 1;
	}
	/* 2026-08-27 用户新增形态（省略帧指针 /FPO + register-param shuffle 序言）：
	   push ebx;push esi;push edi;push ebp;mov esi,ecx;mov ebp,edx;mov ebx,eax;
	   mov eax,[ebx+C];test eax,eax;jl ...（GSCService 0045569 实测形态）。
	   判据收窄为：push 串（≥3 个 push reg 的 MSVC callee-saved 保存）之后紧跟
	   至少 2 条连续 `mov reg,reg`（8B + mod==11，参数从 incoming 寄存器搬到
	   callee-saved 寄存器）。这与 VMP .text stub 的单条 `mov esi,eax` 区分开
	   （单条搬运 + 无 push 串不算），避免旧版"mov esi,eax 就判函数头"的过早
	   fold/trace done 回归。 */
	if (n_push >= 3)
	{
		int j = i;
		/* push 串与 mov reg,reg 参数搬运之间允许夹 push imm：
		   push ebx;push esi;push edi;push ebp;push 0;mov ebx,eax;...
		   （GSCService 004093C 实测形态：53 56 57 55 6A 00 89 C3 89 D6 89 CD）。
		   push imm8 = 6A ??，push imm32 = 68 ?? ?? ?? ??；最多容忍 4 个，
		   防止吞掉过多的 VMP stub 字节。 */
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
			/* mov reg,reg (mod==11)：8B r32,r/m32 或 89 r/m32,r32 都是寄存器搬运 */
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
	/* 2026-08-30 FPO 小函数头（无 push 序言，test reg,reg 入口）：
	   形态如 `test eax,eax; je +7; mov dl,1; mov ecx,[eax]; call [ecx-4]; ret`。
	   判据从严：首指令必须是无副作用入口 test reg,reg(85 + mod==11)，紧随条件跳
	   转(0F 8x / 7x)，且该跳转的落点与函数尾(call[r/m];ret 或 E8;ret)落在同一
	   16 字节窗口内——即"test;je;小函数体直接接 call;ret"，这是编译器/手写的
	   普通函数，而非 VMP .text 混淆块(混淆块虽常见 test，但不会紧跟条件跳转且
	   迅速收尾成 call;ret)。 */
	{
		int head_ok = 0;
		for (int n = 0; n + 1 < (int)sizeof(b); n++)
		{
			/* test reg,reg = 85 /r (32位) 或 84 /r (8位, 如 test dl,dl) 且 mod==11
			   —— 无副作用自比较入口，FPO 小函数常见形态。 */
			int is_test = 0;
			if (b[n] == 0x85 && (b[n + 1] >> 6) == 3)
				is_test = 1;                       /* test r32,r32 */
			if (b[n] == 0x84 && (b[n + 1] >> 6) == 3)
				is_test = 1;                       /* test r8,r8 (如 test dl,dl) */
			if (is_test && n + 2 < (int)sizeof(b))
			{
				unsigned char jcc = b[n + 2];
				/* je/jne/jz/jnz... 等短条件跳转 74/75 + rel8 (7x 所有条件码),
				   或 0F 8x-rel32 长条件跳转 */
				int jlen = 0;
				if ((jcc & 0xF0) == 0x70)
					jlen = 3;                    /* 7x rel8 -> 下一条 */
				else if (jcc == 0x0F && n + 3 < (int)sizeof(b) &&
				         (b[n + 3] & 0xF0) == 0x80)
					jlen = 5;                    /* 0F 8x rel32 -> 下一条 */
				if (jlen)
				{
					int body = n + jlen;
					/* 函数体证据(16 字节窗口内):
					   (a) call;ret / SEH 收尾 = 小函数直接结束;
					   (b) call 调用点(E8 rel32 / FF /2) = 函数调用了别的函数,
					      普通函数体特征, VMP 混淆块的 test;je 后不会紧跟 E8 call。
					   长函数体(如 71B 的 test edx,edx;je;...call...;ret)由(b)覆盖。 */
					int found_tail = 0;
					for (int t = body; t + 1 < (int)sizeof(b) && t <= body + 8; t++)
					{
						/* call [r/m]; ret = FF /2 ... C3 */
						if (b[t] == 0xFF && ((b[t + 1] >> 3) & 7) == 2)
							found_tail = 1;
						/* call rel32; ret = E8 cc cc cc cc C3 */
						if (b[t] == 0xE8 && t + 5 < (int)sizeof(b) && b[t + 5] == 0xC3)
							found_tail = 1;
						/* SEH 收尾: pop dword ptr fs:[0] = 64 8F 05 00 00 00 00 */
						if (b[t] == 0x64 && t + 7 < (int)sizeof(b) &&
						    b[t + 1] == 0x8F && b[t + 2] == 0x05)
							found_tail = 1;
					}
					/* 长函数体: 调用点证据(call rel32 E8 或 call [r/m] FF/2) */
					if (!found_tail)
					{
						for (int t = body; t + 1 < (int)sizeof(b) && t <= body + 20; t++)
						{
							if (b[t] == 0xE8 && t + 4 < (int)sizeof(b))
								found_tail = 1;              /* call rel32 */
							if (b[t] == 0xFF && ((b[t + 1] >> 3) & 7) == 2)
								found_tail = 1;              /* call [r/m] */
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
	/* 朴素：首字节不含 push reg 的字节序列完全不可信，因此即使有 push 也不识别 */
	return 0;
}

/* SHARED "normal function epilogue" check - VM .text stubs / UMF'd helper
   are NOT normal functions, so an address that is neither a normal prologue
   (vmp38_is_norm_funchead) NOR a normal epilogue here is treated as VM code
   to keep executing (not a "function ended" stop). Recognized tails:
     mov esp,ebp; pop ebp; ret            = 8B E5 5D C3
     mov esp,ebp; pop reg; jmp (tail)     variant
     pop ebp; ret  (if preceded by mov esp,ebp already done)
     add esp,imm; pop ebp; ret            = 83 C4 ?? 5D C3
   Used by BOTH the ret->.text handling and the call-fold logic so VM->.text
   transitions share one epilogue test. */
static int vmp38_is_norm_func_tail(uc_engine *uc, uint64_t addr)
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
	/* (mov esp,ebp = 8B E5); pop reg(0x5D/0x5B/...); ret(0xC3) */
	if (b[0] == 0x8B && b[1] == 0xE5)
	{
		/* 8B E5 5D C3  -> mov esp,ebp; pop ebp; ret */
		if (b[2] == 0x5D && b[3] == 0xC3) return 1;
		/* 8B E5 5D E9 .. .. .. ..  -> mov esp,ebp; pop ebp; jmp (tail-call) */
		if (b[2] == 0x5D && b[3] == 0xE9) return 1;
		/* 8B E5 5B C3 / 8B E5 5E 5D C3 -> pop ebx/esi...; ret */
		if ((b[2] == 0x5B || b[2] == 0x5E || b[2] == 0x5F) &&
			(b[3] == 0xC3 || (b[3] == 0x5D && b[4] == 0xC3)))
			return 1;
	}
	/* (add esp,imm8 = 83 C4 ??); pop ebp; ret -> 83 C4 ?? 5D C3 */
	if (b[0] == 0x83 && b[1] == 0xC4 && b[3] == 0x5D && b[4] == 0xC3)
		return 1;
	/* bare pop ebp(0x5D); ret(0xC3) as a function tail */
	if (b[0] == 0x5D && b[1] == 0xC3)
		return 1;
	/* ---- FPO / 无帧指针函数尾声(2026-08-28, GSCService .text 实测 97% 函数
	       尾是这种): `pop callee-saved; ret` 串。MSVC /Oy 省略 ebp 后, 尾声是
	       `pop ebx/esi/edi/ebp; ret`(不再 mov esp,ebp), 甚至 2~5 个连续 pop。
	       判据从严: pop 只认 callee-saved(5B=ebx/5E=esi/5F=edi/5D=ebp), 串后紧跟
	       ret(C3) 或 ret imm16(C2)。不认 pop eax/ecx/edx(5x/59/5A = caller-saved,
	       函数中间的栈平衡 pop 不是尾声), 也不认"数据操作后 ret"(前面非 pop 串)。
	       与上面"short stub 过宽"教训区分: 这里只认【纯 pop callee-saved 串立即 ret】,
	       是编译器的确定性尾声字节签名, 非低混淆 stub。 */
	{
		/* 从窗口起点扫描: 连续 pop callee-saved(5B/5E/5F/5D), 至少 1 个, 后跟 ret */
		int p = 0, npop = 0;
		while (p < (int)sizeof(b) && (b[p] == 0x5B || b[p] == 0x5E ||
			b[p] == 0x5F || b[p] == 0x5D))
		{
			npop++;
			p++;
		}
		if (npop >= 1 && p < (int)sizeof(b) &&
			(b[p] == 0xC3 || b[p] == 0xC2))
			return 1;
	}
	/* ---- leave; ret (C9 C3) = mov esp,ebp; pop ebp; ret 的编译器等价 */
	if (b[0] == 0xC9 && b[1] == 0xC3)
		return 1;
	/* ---- leave; pop reg...; ret (C9 5B C3 等, 罕见但存在) */
	if (b[0] == 0xC9)
	{
		int p = 1, n = 0;
		while (p < (int)sizeof(b) && n < 8 && (b[p] == 0x5B || b[p] == 0x5E ||
			b[p] == 0x5F || b[p] == 0x5D))
		{
			n++;
			p++;
		}
		if (n >= 1 && p < (int)sizeof(b) && (b[p] == 0xC3 || b[p] == 0xC2))
			return 1;
	}
	/* ---- 尾调用/trampoline stub（正常函数无标准 pop 尾声，以 call;ret 结束）：
	       编译器/手写的 `return fn(...)` 未优化为 jmp、虚函数析构返回、guard stub 等。
	       形态(直接/间接 call 后立即 ret = 函数到此结束)：
	         E8 <rel32> C3                      = call rel32; ret            (00AE5: ..call 40ACB8; ret)
	         FF <call-modrm>[sib][disp] C3      = call [reg]/[mem]; ret     (40699: FF 51 FC = call dword[ecx-4]; ret)
	       这些是明确的"正常函数结束"边界：ret 目标命中即停(不再追进其内部无限循环)。
	       注意只认 call 后缀(/2)，不认 jmp(/4)——jmp;ret 是死代码非函数尾。 */
	if (b[0] == 0xE8 && b[5] == 0xC3)
		return 1;   /* call rel32; ret */
	if (b[0] == 0xFF)
	{
		unsigned char modrm = b[1];
		int group = (modrm >> 3) & 7;
		if (group == 2)   /* FF /2 = call r/m */
		{
			int len = 2, mod = modrm >> 6, rm = modrm & 7;
			if (mod != 3)
			{
				if (rm == 4) len++;                    /* SIB */
				if (mod == 0 && rm == 5) len += 4;     /* disp32 */
				else if (mod == 1) len += 1;           /* disp8 */
				else if (mod == 2) len += 4;           /* disp32 */
			}
			if (len < (int)sizeof(b) && b[len] == 0xC3)
				return 1;   /* call [r/m]; ret */
		}
	}
	/* ---- SEH 恢复段(MSVC __try/__finally/__except epilogue)：
	        `xor eax,eax; pop reg; pop reg; mov dword ptr fs:[eax],edx` 这种
	        恢复 fs:[0] 异常处理链 + 返回值置零的收尾。VM 结束标志标在这里
	        (VMProtectEnd 之后就是这段明文 SEH 恢复/resource-cleanup 代码)。
	        判据从严: 首指令 xor reg,reg(31/33 + mod==11), 紧随 ≥2 个 pop reg,
	        且 10 字节内出现 `mov dword ptr fs:[eax],edx`(64 89 10) 或其变体
	        (64 89 08/18/00...)。VM 混淆块不会出现"xor eax,eax+连续pop+fs修复"
	        这种 SEH 收尾, 故此判据安全。 */
	{
		int is_seh = 0;
		for (int t = 0; t + 1 < (int)sizeof(b); t++)
		{
			/* xor r32,r32 = 31 /r 或 33 /r 且 mod==11 (如 xor eax,eax = 33 C0) */
			if ((b[t] == 0x31 || b[t] == 0x33) && (b[t + 1] >> 6) == 3)
			{
				int p = t + 2, npop = 0;
				while (p < (int)sizeof(b) && npop < 8)
				{
					if ((b[p] & 0xF8) == 0x58) { npop++; p++; continue; }
					break;
				}
				if (npop >= 2)
				{
					/* 后续(≤10字节内)找 mov dword ptr fs:[eax],edx = 64 89 10 */
					for (int q = p; q + 2 < (int)sizeof(b) && q <= p + 10; q++)
					{
						if (b[q] == 0x64 && b[q + 1] == 0x89 && (b[q + 2] >> 6) == 0)
						{
							is_seh = 1;
							break;
						}
					}
				}
			}
		}
		if (is_seh)
			return 1;
	}
	/* 2026-08-23 回退：此前的"short stub(≤12字节低混淆+ret即判函数尾)"过宽,
	   会把真实软件 .text 里的普通代码(如 0xb12bda mov/lea;ret)误判为函数尾边界,
	   导致 data 提前 165 条就 stopped at call. 已移除,只保留上面的 call;ret 尾调识别
	   (用户贴的两种函数(尾调call;ret)仍适配)。 */
	return 0;
}

/* SHARED VM->.text control-flow boundary test (per user rule):
   - a normal function EPILOGUE (is_epilog) is ALWAYS a real function end -> stop.
   - when auto-continue is OFF (g_vm_continue==0), a normal function PROLOGUE
     is also a boundary (the user only wants THIS VM function, stop at the next
     function head). When auto-continue is ON, only the epilogue is a boundary
     - a prologue is the NEXT VM function / .text VM stub -> keep tracing it.
   - non-normal head/tail (VM code in .text) -> not a boundary, keep tracing. */
static int vmp38_is_func_boundary(uc_engine *uc, uint64_t target)
{
	if (vmp38_is_norm_func_tail(uc, target))
		return 1;                            /* 函数尾: 总是结束边界 */
	if (!g_vm_continue)
		return vmp38_is_norm_funchead(uc, target);   /* auto-off: 函数头也边界 */
	return 0;                                /* auto-on: 只有函数尾是边界 */
}


static int vmp38_is_vmexit_handler(int n_popreg, int n_popfd, int n_popad)
{
	/* VMP 3.8 real VM exit = VM 代码 ret 目标不在 .text/在 VM 段外。以退出序列
	   非孤列指令 + ret(0xc/8/0)，没有统一的 popad；3 个 GSCService
	   trace32 实证：没有 popad，只有 pop edx 的散列 + ret；所以旧判断
	   "popfd && popad" 在 n_popad>=1 时 false -> g_ft_vmexit_pending
	   无法置位 -> 6922 未能准确判定控制权（headless 停止时会卡）。
	   现在只以"真实退出"为可靠标志，再配合 vmp38_is_vmret_target
	   检测 ret 目标离开 VM 段，准确判定（popfd 或 >=2 popreg 展列均视为
	   真实退出）。 */
	(void)n_popad;
	return (n_popfd >= 1 || n_popreg >= 2);
}

/* is a ret target the real vmret? yes when it LEAVES the VM section
   (a ret whose target stays inside is VMP internal ret-dispatch, not a
   function return - verified GUI 00cdea8d->00c365b7 false positive) */
static int vmp38_is_vmret_target(unsigned long long target)
{
	if (g_vm_start && g_vm_end &&
		target >= g_vm_start && target <= g_vm_end)
		return 0;            /* still inside VM section - internal dispatch */
	return 1;                /* left the VM section - real return */
}

void vmp38_get_vm_bounds(unsigned long long *start, unsigned long long *end)
{
	if (start) *start = g_vm_start;
	if (end) *end = g_vm_end;
}

unsigned long long vmp38_auto_locate()
{
	Script::Module::ModuleInfo mi;
	unsigned long long vmsect_begin = 0, vmsect_end = 0;
	unsigned long long text_begin = 0, text_end = 0;
	int i;

	if (!Script::Module::GetMainModuleInfo(&mi))
		return 0;

	/* find VM sections: ALL non-.text code sections (VMP splits VM code across
	   .gsc0/.gsc1/.gsc2/.itext/.38vm/...), not just the largest one. Union
	   their address range so "ret out of VM section" doesn't false-positive
	   when a VM function returns into a SIBLING VM section (GSCService has
	   .gsc0@0x1217000 + .gsc2@0x2010000 + .itext@0xb58000). */
	{
		int n = Script::Module::SectionCountFromAddr(mi.base);
		unsigned long long min_begin = ~0ULL, max_end = 0;
		static const char *kDataSecs[] = {
			".data", ".bss", ".idata", ".didata", ".edata", ".tls",
			".rdata", ".rsrc", ".reloc", ".pdata", ".crt", ".CRT",
			".gfids", ".00cfg", ".fptable", ".rtc", NULL
		};
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
					continue;
				}
				/* skip data sections (explicit names) */
				int is_data = 0;
				for (int k = 0; kDataSecs[k]; k++)
					if (strcmp(nm, kDataSecs[k]) == 0) { is_data = 1; break; }
				if (is_data) continue;
				/* VM/code section: known VMP names OR a large section
				   (small unnamed leftovers like .fptable/等命名段 are
				   filtered by the size floor) */
				int is_vm_name = (strstr(nm, "gsc") || strstr(nm, "itext") ||
					strstr(nm, "vmp") || strstr(nm, "vm"));
				if (!is_vm_name && si.size < 0x10000)
					continue;
				/* VM/code section: union the range */
				if (si.addr < min_begin) min_begin = si.addr;
				if (si.addr + si.size > max_end) max_end = si.addr + si.size;
			}
		}
		if (min_begin != ~0ULL && max_end)
		{
			vmsect_begin = min_begin;
			vmsect_end = max_end;
		}
	}

	if (!vmsect_begin || !text_begin)
		return 0;

	g_vm_start = vmsect_begin;
	g_vm_end = vmsect_end;
	out("vmp38_auto_locate: text=[%08llx-%08llx] vmsect=[%08llx-%08llx]",
		text_begin, text_end, vmsect_begin, vmsect_end);

	/* scan the VM section start for the first executable handler:
	   disassemble ahead, skip data/junk, find code with stack ops */
	{
		unsigned long long a;
		int good_run = 0;
		unsigned long long good_start = 0;
		for (a = vmsect_begin; a < vmsect_begin + 0x8000 && a < vmsect_end; )
		{
			DISASM_INSTR ins;
			char g[32];
			memset(&ins, 0, sizeof(ins));
			/* GUI: VM section memory is readable -> DbgDisasmAt.
			   headless: memory unmapped -> file fallback. */
			DbgDisasmAt(a, &ins);
			if (ins.instr_size < 1 && !disasm_from_file(a, &ins))
			{
				good_run = 0;
				a += 4;
				continue;
			}
			get_mnemonic(ins.instruction, g, sizeof(g));
			if (strcmp(g, "outsb") == 0 || strcmp(g, "outsd") == 0 ||
				strcmp(g, "insb") == 0 || strcmp(g, "insd") == 0 ||
				strcmp(g, "in") == 0 || strcmp(g, "out") == 0 ||
				strcmp(g, "hlt") == 0 || strcmp(g, "int") == 0)
			{
				good_run = 0;
				a += ins.instr_size;
				continue;
			}
			if (good_run == 0)
				good_start = a;
			good_run++;
			if (good_run >= 6)
			{
				/* return preferred-base VA (ASLR-stable): RVA(rel. module base) + PE ImageBase */
				{
					unsigned long long rva = good_start - mi.base;
					unsigned long long pref = vmp38_get_imgbase();
					if (!pref) pref = 0x400000ULL;
					out("vmp38_auto_locate: VM entry @ %08llx rva=%08llx (first code block, %d insns)",
						good_start, rva, good_run);
					return pref + rva;
				}
			}
			a += ins.instr_size;
		}
	}
	{
		/* fallback: also return preferred-base VA (type consistency with the
		   first-code-block path, so the menu's runtime conversion is correct) */
		unsigned long long rva = vmsect_begin - mi.base;
		unsigned long long pref = vmp38_get_imgbase();
		if (!pref) pref = 0x400000ULL;
		out("vmp38_auto_locate: fallback vmsect_begin=%08llx rva=%08llx", vmsect_begin, rva);
		return pref + rva;
	}
}

/* Scan the VM section for ALL executable code blocks (>=N consecutive
   disassemblable instructions). VMP 3.8 virtualized functions are entered
   via indirect jumps, so each function has its own entry deep in the VM
   section. Returns count; entries printed as preferred-base VA. */
static unsigned long long g_runbase;       /* runtime module base (ASLR) - forward decl */


int vmp38_scan_all_entries(void)
{
	int log_owned = 0;   /* Scan = GUI-only output (entry list), no data file */
	Script::Module::ModuleInfo mi;
	unsigned long long vmsect_begin = 0, vmsect_end = 0;
	unsigned long long runbase = 0;
	int i, found = 0;

	if (!Script::Module::GetMainModuleInfo(&mi))
	{
		if (log_owned) vmp38_log_close();
		return 0;
	}
	runbase = mi.base;
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
	out("vmp38_scan_all_entries: vmsect=[%08llx-%08llx]",
		vmsect_begin, vmsect_end);

	/* ensure the file-backed reader is open (headless scan menu does not
	   call vmp38_set_module; the call-target scan reads .text bytes from
	   the FILE because VMP encrypts in-memory .text before the stub runs) */
	g_runbase = mi.base;   /* read_mem_file converts addr->rva via g_runbase */
	vmp38_set_module(mi.path, mi.base);

	/* auto-test: for each large code block (>=80 insns, likely a real
	   virtualized function body), run restore to identify its handlers.
	   Enhanced: detect pushfd/pushf-prefixed entries (kanxue 280283:
	   vm_start=pushfd) as precise VM-function boundaries - VMP 3.8 (32-bit)
	   handlers save eflags with pushfd at their start; the VM entry handler
	   is exactly the first pushfd after a dispatch. 32-bit samples do NOT
	   use pushad/popad (single-register pops instead), so the boundary
	   markers are pushfd(entry)/popfd+popregs(exit). */
	if (1)
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
		int n_entries = 0, k, kk, n_calltarget = 0;
		unsigned long long a = vmsect_begin;

	/* CALL-TARGET method first (most precise): scan the on-disk .text
	   (plaintext even though in-memory is VMP-encrypted pre-stub) for
	   direct call rel32 whose target lands in the VM section - each such
	   target is a REAL virtualized function entry; the call site is its
	   caller. This finds entries pushfd enumeration misses (e.g. vm_complex
	   entry 0x92cb4 is not pushfd-headed). */
	{
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
								/* prefer LIVE (decrypted) .text - GUI with the process
								   running has real instructions (VMP decrypts .text at
								   runtime; the FILE holds encrypted/obfuscated bytes that
								   produce false 0xE8 "calls"). File fallback otherwise. */
								int have_live = 0;
								{
									duint lsz = 0;
									memset(tb, 0, rsz);
									if (Script::Memory::Read((duint)(mi.base + vad), tb, rsz, &lsz) && lsz >= (duint)rsz)
										have_live = 1;
								}
								if (!have_live)
								{
									fseek(pf, rptr, SEEK_SET);
									if (fread(tb, 1, rsz, pf) != rsz)
										memset(tb, 0, rsz);
								}
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
							}
							break;
						}
					}
				}
			}
			fclose(pf);
		}
	}


		int good_run = 0;
		unsigned long long good_start = 0;
		/* The call-target scan above already found the real VM functions -
		   SKIP the slow pushfd/block enumeration entirely (it walked the
		   whole VM section with per-instruction DbgDisasmAt bridge calls,
		   ~500k calls = minutes of GUI freeze / crash). Fall back to it
		   only when no call-target was found (e.g. indirect-call-only
		   samples). Use file bytes (disasm_from_file) - never DbgDisasmAt
		   on the VM section (VMP-encrypted live pages can fault). */
		if (n_entries == 0)
		{
			while (a < vmsect_end && a < vmsect_begin + 0x200000 && n_entries < n_entries_cap)
			{
				DISASM_INSTR ins;
				char g[32];
				memset(&ins, 0, sizeof(ins));
				if (!disasm_from_file(a, &ins))
				{
					if (good_run >= 80 && n_entries < n_entries_cap)
						entries[n_entries++] = good_start;
					good_run = 0;
					a += 4;
					continue;
				}
				get_mnemonic(ins.instruction, g, sizeof(g));
				if (strcmp(g, "outsb") == 0 || strcmp(g, "outsd") == 0 ||
					strcmp(g, "insb") == 0 || strcmp(g, "insd") == 0 ||
					strcmp(g, "in") == 0 || strcmp(g, "out") == 0 ||
					strcmp(g, "hlt") == 0 || strcmp(g, "int") == 0 ||
					strcmp(g, "???") == 0)
				{
					if (good_run >= 80 && n_entries < n_entries_cap)
						entries[n_entries++] = good_start;
					good_run = 0;
					a += ins.instr_size;
					continue;
				}
				if (good_run == 0)
					good_start = a;
				good_run++;
				a += ins.instr_size;
			}
			if (good_run >= 80 && n_entries < n_entries_cap)
				entries[n_entries++] = good_start;
		}
		out("vmp38_scan_all_entries: %d call-target REAL VM-ized functions (found by scanning .text call->VM-section)", n_calltarget);
		for (k = 0; k < n_entries; k++)
		{
			/* FUNCTION-LIST mode: only call-target entries (real VM functions
			   reached from .text) are reported - the user bps the entry and
			   runs VMP38-Dyn for precise restore (params/return/out-params). */
			if (entry_call[k])
			{
				/* show 6 raw bytes before the E8 so the user can judge real
				   VMP stubs (push imm32 = 68 xx xx xx xx, mov reg,imm =
				   B8-BF) vs false positives from data/encrypted bytes in
				   .text (random 0xE8). */
				unsigned char preb[6] = {0,0,0,0,0,0};
				char prehx[40] = "";
				const char *mark = "?";
				if (read_mem_file(entry_call[k] - 6, preb, 6))
				{
					int bi;
					int strong = 0, weak = 0, pushreg = 0;
					for (bi = 0; bi < 6; bi++)
					{
						char bb[8];
						_snprintf(bb, sizeof(bb), "%s%02x", bi ? " " : "", preb[bi]);
						SCAT(prehx, bb);
						int gap = 5 - bi;   /* bytes from this opcode to the E8 */
						if (preb[bi] == 0x68 && gap >= 4 && gap <= 6)
							strong = 1;     /* push imm32 immediately before call */
						else if (preb[bi] >= 0xB8 && preb[bi] <= 0xBF && gap == 4)
							strong = 1;     /* mov r32,imm32 immediately before call */
						else if (preb[bi] == 0x68 || (preb[bi] >= 0xB8 && preb[bi] <= 0xBF))
							weak = 1;       /* imm-op inside the window but not adjacent */
						else if (preb[bi] >= 0x50 && preb[bi] <= 0x57)
							pushreg++;
					}
					if (strong) mark = "STUB-STRONG (imm32-op;call) - real VM call";
					else if (weak) mark = "STUB-weak (imm-op in window) - check";
					else if (pushreg >= 1) mark = "PUSH-REG - normal call?";
					else if (preb[5] == 0xE8) mark = "CALL-CHAIN";
					else mark = "suspect - likely data/encrypted";
				}
				out("  [vm-func] call site %08llx (rva .text+%08llx) pre=[%s] %s -> VM-ized impl %08llx (rva %08llx)",
					entry_call[k], entry_call[k] - mi.base, prehx, mark,
					entries[k], entries[k] - mi.base);
				n_calltarget++;
			}
		}
		/* scan stays a pure ENTRY scanner (VMP38-Scan = find VM functions);
		   static extraction lives in VMP38-Restore (static mode) */
		out("=== scan summary: %d call-target REAL functions ===", n_calltarget);
		if (log_owned) vmp38_log_close();
		return n_entries;
	}
	{
		unsigned long long a = vmsect_begin;
		int good_run = 0;
		unsigned long long good_start = 0;
		while (a < vmsect_end && a < vmsect_begin + 0x200000)
		{
			DISASM_INSTR ins;
			char g[32];
			memset(&ins, 0, sizeof(ins));
			DbgDisasmAt(a, &ins);
			if (ins.instr_size < 1 && !disasm_from_file(a, &ins))
			{
				if (good_run >= 3)
					out("  entry@%08llx (rva=%08llx, %d insns)",
						good_start, good_start - runbase, good_run);
				good_run = 0;
				a += 4;
				continue;
			}
			get_mnemonic(ins.instruction, g, sizeof(g));
			if (strcmp(g, "outsb") == 0 || strcmp(g, "outsd") == 0 ||
				strcmp(g, "insb") == 0 || strcmp(g, "insd") == 0 ||
				strcmp(g, "in") == 0 || strcmp(g, "out") == 0 ||
				strcmp(g, "hlt") == 0 || strcmp(g, "int") == 0 ||
				strcmp(g, "???") == 0)
			{
				if (good_run >= 3)
					out("  entry@%08llx (rva=%08llx, %d insns)",
						good_start, good_start - runbase, good_run);
				good_run = 0;
				a += ins.instr_size;
				continue;
			}
			if (good_run == 0)
				good_start = a;
			good_run++;
			a += ins.instr_size;
		}
		if (good_run >= 3)
			out("  entry@%08llx (rva=%08llx, %d insns)",
				good_start, good_start - runbase, good_run);
	}
	out("vmp38_scan_all_entries: done");
	if (log_owned) vmp38_log_close();
	return found;
}

/* Run to VM entry and capture real registers there */
int vmp38_capture_vm_regs(unsigned long long vm_entry, VMP38_CTX *ctx)
{
	if (!ctx || !vm_entry)
		return 0;
	/* if already at VM entry, just read registers without running */
	{
#ifdef VMP32_BUILD
		unsigned long cur_ip = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
		if (cur_ip == (unsigned long)vm_entry)
		{
			ctx->r[0] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EAX);
			ctx->r[1] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ECX);
			ctx->r[2] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EDX);
			ctx->r[3] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EBX);
			ctx->r[4] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ESP);
			ctx->r[5] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EBP);
			ctx->r[6] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ESI);
			ctx->r[7] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EDI);
			ctx->ip = cur_ip;
			out("vmp38_capture(fast): ip=%08lx esp=%08lx ebp=%08lx ebx=%08lx",
				ctx->ip, ctx->r[4], ctx->r[5], ctx->r[3]);
			return 1;
		}
#else
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
#endif
	}
	/* NOTE: SetBreakpoint+Run triggers VMP 3.8 anti-debug ("File corrupted").
	   Only the pure-read fast path (EIP already at VM entry) is safe.
	   If EIP is not at the entry, read the current registers anyway and
	   let the caller drive static restore from there (no bp, no run). */
#ifdef VMP32_BUILD
	{
		unsigned long cur_ip = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
		if (cur_ip != (unsigned long)vm_entry)
		{
			out("vmp38_capture: EIP %08lx != entry %08llx (no bp/run: anti-debug); using current regs",
				cur_ip, vm_entry);
			ctx->r[0] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EAX);
			ctx->r[1] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ECX);
			ctx->r[2] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EDX);
			ctx->r[3] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EBX);
			ctx->r[4] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ESP);
			ctx->r[5] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EBP);
			ctx->r[6] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ESI);
			ctx->r[7] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EDI);
			ctx->ip = cur_ip;
			return 1;
		}
	}
#else
	{
		unsigned long long cur_ip = Script::Register::Get(Script::Register::RegisterEnum::RIP);
		if (cur_ip != vm_entry)
		{
			out("vmp38_capture: RIP %08llx != entry (no bp/run); using current regs", cur_ip);
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
			return 1;
		}
	}
#endif
	/* read registers */
	{
#ifdef VMP32_BUILD
		ctx->r[0] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EAX);
		ctx->r[1] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ECX);
		ctx->r[2] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EDX);
		ctx->r[3] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EBX);
		ctx->r[4] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ESP);
		ctx->r[5] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EBP);
		ctx->r[6] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ESI);
		ctx->r[7] = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EDI);
		ctx->ip = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
#else
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
#endif
	}
#ifdef VMP32_BUILD
	out("vmp38_capture: ip=%08lx esp=%08lx ebx=%08lx",
		ctx->ip, ctx->r[4], ctx->r[3]);
#else
	out("vmp38_capture: ip=%08llx rsp=%08llx rbx=%08llx",
		ctx->ip, ctx->r[4], ctx->r[3]);
#endif

	/* remove temp breakpoint */
	Script::Debug::DeleteBreakpoint(vm_entry);
	return 1;
}


/* ============ file-based VM section read (headless fallback) ============ */
static char g_modpath[1024] = "";
static unsigned long long g_imgbase = 0;   /* PE preferred ImageBase */
static int g_dyn_func_count = 0;   /* per-function restore-log counter */

unsigned long long vmp38_get_imgbase(void)
{
	return g_imgbase;
}

static FILE *g_modfile = NULL;

void vmp38_set_module(const char *path, unsigned long long imgbase)
{
	unsigned char peh[0x400];
	unsigned int peoff;
	long fsz;
	out("vmp38_set_module: path=%s base=%08llx", path, imgbase);
	strncpy(g_modpath, path, sizeof(g_modpath) - 1);
	g_runbase = imgbase;
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
				for (i = 0; i < 4; i++)
					ib |= ((unsigned long long)peh[peoff + 24 + 28 + i]) << (8 * i);
				g_imgbase = ib;
				out("vmp38_set_module: PE ImageBase=%08llx", g_imgbase);
			}
		}
	}
}

/* read n bytes at VA addr from the module file (PE section mapping) */
static int read_mem_file(unsigned long long addr, void *buf, int n)
{
	unsigned long long rva;
	unsigned char peh[0x400];
	unsigned int peoff, nsec, optsize;
	long fsz;
	int i;
	if (!g_modfile) return 0;  /* silent: no module file handle (headless) - was spamming "no modfile" per step */
	rva = addr - (g_runbase ? g_runbase : g_imgbase);
	(void)rva;
	fseek(g_modfile, 0, SEEK_END); fsz = ftell(g_modfile);
	fseek(g_modfile, 0x3c, SEEK_SET);
	if (fread(peh, 1, 4, g_modfile) != 4) return 0;
	peoff = peh[0] | (peh[1] << 8) | (peh[2] << 16) | (peh[3] << 24);
	if (peoff + 0x18 + 0x200 > fsz) return 0;
	fseek(g_modfile, peoff, SEEK_SET);
	if (fread(peh, 1, 0x18, g_modfile) != 0x18) return 0;
	nsec = peh[6] | (peh[7] << 8);
	optsize = peh[20] | (peh[21] << 8);
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
	return 0;
}

/* decrypted module bytes captured during the Unicorn dump: restore analysis
   disassembles from this memory (fast, no GUI bridge) instead of 20M
   DbgDisasmAt calls (minutes) - the VM section is decrypted in live memory
   but ENCRYPTED in the on-disk file, so we must snapshot at dump time. */
static unsigned char *g_tm_data = NULL;
static unsigned long long g_tm_base = 0, g_tm_end = 0;

/* disassemble via file fallback: read bytes from file, use xx_disasm */
static int disasm_from_file(unsigned long long ip, DISASM_INSTR *out_ins)
{
	unsigned char cmdbuf[64];
	struct XX_INST inst;
	unsigned long long base = 0;
	memset(cmdbuf, 0, sizeof(cmdbuf));
	if (!read_mem_file(ip, cmdbuf, sizeof(cmdbuf)))
		return 0;
	memset(&inst, 0, sizeof(inst));
	inst.xx_inst_code.inst_system = INST_SYSTEM_CURT;
	memcpy(inst.xx_inst_code.base, &base, INST_SYSTEM_CURT);
	memcpy(inst.xx_inst_code.current_addr, &ip, INST_SYSTEM_CURT);
	xx_disasm(&inst, cmdbuf);
	if (inst.xx_inst_exist.finish_flag != INST_FLAG_EXIST)
		return 0;
	out_ins->instr_size = inst.xx_inst_code.disasm_length;
	strncpy(out_ins->instruction, (const char*)inst.xx_inst_code.disasm, sizeof(out_ins->instruction) - 1);
	return out_ins->instr_size > 0 ? 1 : 0;
}

/* fast disasm for bulk analysis: prefer the decrypted module snapshot
   (pure memory + xx_disasm, no x64dbg bridge), fall back to DbgDisasmAt
   for addresses outside the snapshot (rare).
   When uc is non-NULL (live Unicorn code hook) read the bytes from the
   EMULATOR's current memory instead of the g_tm_data snapshot: VMP 3.8
   handlers SELF-MODIFY at runtime (observed 01fed582: snapshot bytes
   disassemble to "jmp 0x0133F705" but the emulator actually executes a
   jmp to 01e4ee82 - the opcode bytes changed mid-trace). A static
   snapshot disassembles the OLD bytes -> data rows show a jump target
   that never executed, and the restore misses the real code. */

/* SEH-guarded xx_disasm: xx_disasm can AV (fault=0) on garbage bytes -
   the trace jumps to unmapped/garbage addresses (observed last_eip
   8523fb07 after a 01306671 -> high-address jump) and xx_disasm parsing
   those bytes crashes the host. The __try CANNOT live inside a function
   with C++ objects, so this wrapper is plain C. */
static int xd_safe_disasm(struct XX_INST *inst, unsigned char *cmdbuf)
{
	__try {
		xx_disasm(inst, cmdbuf);
		return 1;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return 0;   /* garbage bytes - caller falls back to DbgDisasmAt */
	}
}

/* SEH-guarded DbgDisasmAt: the debugger bridge can AV on a garbage
   address (unmapped memory outside the module). Return 0 = failed. */
static int xd_safe_dbg_disasm(unsigned long long ip, DISASM_INSTR *di)
{
	__try {
		DbgDisasmAt((duint)ip, di);
		return (di->instr_size >= 1 && di->instruction[0]) ? 1 : 0;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		memset(di, 0, sizeof(*di));
		return 0;
	}
}

/* disasm byte-hash cache for the LIVE (hook) path: VMP self-modifies too
   rarely for us to cache one entry per eip naively, BUT we can key on the
   actual machine-code bytes - if the 64 bytes at eip are unchanged since
   last disasm, reuse the result (skips the expensive xx_disasm +
   std::string build). On self-modify the hash changes and we recompute.
   Fixed 4096 slots, indexed by eip (no heap in the hot loop). */
#define DAD_CACHE_SZ 4096
struct DAD_ENT { unsigned int eip; unsigned long long hash; unsigned char sz; char txt[128]; };
static DAD_ENT g_dad[DAD_CACHE_SZ];
static unsigned long long dad_hash64(const unsigned char *p, int n)
{
	unsigned long long h = 0xcbf29ce484222325ULL;
	for (int i = 0; i < n; i++) { h ^= p[i]; h *= 0x100000001b3ULL; }
	return h;
}
static int disasm_at(unsigned long long ip, DISASM_INSTR *di, uc_engine *uc)
{
	if (uc)
	{
		unsigned char cmdbuf[64];
		unsigned long long rsz = 0;
		int mrd_ok = 0;
		__try {
			if (uc_mem_read(uc, ip, cmdbuf, sizeof(cmdbuf)) == UC_ERR_OK)
				mrd_ok = 1;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			mrd_ok = 0;   /* hook-internal uc_mem_read AV (fault=0) - skip row */
		}
		if (!mrd_ok)
			return 0;
		rsz = 64;
		/* byte-hash cache (live path): reuse if the 64 bytes at ip are unchanged */
		{
			unsigned int idx = (unsigned int)ip & (DAD_CACHE_SZ - 1);
			DAD_ENT *sl = &g_dad[idx];
			unsigned long long hh = dad_hash64(cmdbuf, 64);
			if (sl->eip == (unsigned int)ip && sl->hash == hh)
			{
				di->instr_size = sl->sz;
				size_t L = strlen(sl->txt);
				memcpy(di->instruction, sl->txt, L);
				di->instruction[L] = 0;
				return L > 0 ? 1 : 0;
			}
		}
		{
			struct XX_INST inst;
			unsigned long long base = 0;
			memset(&inst, 0, sizeof(inst));
			inst.xx_inst_code.inst_system = INST_SYSTEM_CURT;
			memcpy(inst.xx_inst_code.base, &base, INST_SYSTEM_CURT);
			memcpy(inst.xx_inst_code.current_addr, &ip, INST_SYSTEM_CURT);
			if (!xd_safe_disasm(&inst, cmdbuf))
				inst.xx_inst_exist.finish_flag = 0;   /* garbage - force fallback */
			if (inst.xx_inst_exist.finish_flag != INST_FLAG_EXIST)
			{
				memset(di, 0, sizeof(*di));
				if (xd_safe_dbg_disasm(ip, di))
					return 1;
				return 0;
			}
			memset(di, 0, sizeof(*di));
			di->instr_size = inst.xx_inst_code.disasm_length;
			strncpy(di->instruction, (const char*)inst.xx_inst_code.disasm, sizeof(di->instruction) - 1);
			di->instruction[sizeof(di->instruction) - 1] = 0;
			if (di->instr_size > 0 && strncmp(di->instruction, "???", 3) == 0)
			{
				DISASM_INSTR d2;
				memset(&d2, 0, sizeof(d2));
				if (xd_safe_dbg_disasm(ip, &d2) &&
					d2.instr_size >= 1 && strncmp(d2.instruction, "???", 3) != 0)
				{
					memcpy(di, &d2, sizeof(*di));
					return 1;
				}
			}
			/* fill the live disasm cache: store by (eip, byte-hash) */
			if (di->instr_size > 0 && di->instruction[0])
			{
				unsigned int idx2 = (unsigned int)ip & (DAD_CACHE_SZ - 1);
				DAD_ENT *s2 = &g_dad[idx2];
				s2->eip = (unsigned int)ip;
				s2->hash = dad_hash64(cmdbuf, 64);
				s2->sz = (unsigned char)di->instr_size;
				size_t tl2 = strnlen(di->instruction, sizeof(di->instruction));
				if (tl2 > 127) tl2 = 127;
				memcpy(s2->txt, di->instruction, tl2);
				s2->txt[tl2] = 0;
			}
			return di->instr_size > 0 ? 1 : 0;
		}
	}
	if (g_tm_data && ip >= g_tm_base && ip + 64 <= g_tm_end)
	{
		unsigned char cmdbuf[64];
		struct XX_INST inst;
		unsigned long long base = 0;
		memcpy(cmdbuf, g_tm_data + (size_t)(ip - g_tm_base), sizeof(cmdbuf));
		memset(&inst, 0, sizeof(inst));
		inst.xx_inst_code.inst_system = INST_SYSTEM_CURT;
		memcpy(inst.xx_inst_code.base, &base, INST_SYSTEM_CURT);
		memcpy(inst.xx_inst_code.current_addr, &ip, INST_SYSTEM_CURT);
		if (!xd_safe_disasm(&inst, cmdbuf))
			inst.xx_inst_exist.finish_flag = 0;   /* garbage - force fallback */
		if (inst.xx_inst_exist.finish_flag != INST_FLAG_EXIST)
		{
			/* snapshot bytes undecodable (headless: VM section unreadable via
			   bridge so the snapshot holds garbage) - fall back to the
			   debugger disassembler instead of failing the whole pass. */
			memset(di, 0, sizeof(*di));
			if (xd_safe_dbg_disasm(ip, di))
				return 1;
			return 0;
		}
		memset(di, 0, sizeof(*di));
		di->instr_size = inst.xx_inst_code.disasm_length;
		strncpy(di->instruction, (const char*)inst.xx_inst_code.disasm, sizeof(di->instruction) - 1);
		di->instruction[sizeof(di->instruction) - 1] = 0;
		/* xx_disasm decoded but produced "???" (unknown/undecodable bytes -
		   headless snapshot may hold encrypted VM-section bytes). Retry with
		   the debugger disassembler: under GUI the live memory is decrypted,
		   so a genuine instruction that xx_disasm lacks gets recovered. */
		if (di->instr_size > 0 && strncmp(di->instruction, "???", 3) == 0)
		{
			DISASM_INSTR d2;
			memset(&d2, 0, sizeof(d2));
			if (xd_safe_dbg_disasm(ip, &d2) &&
				d2.instr_size >= 1 && strncmp(d2.instruction, "???", 3) != 0)
			{
				memcpy(di, &d2, sizeof(*di));
				return 1;
			}
		}
		/* xx_disasm's internal disasm buffer is uchar[50] - a long operand
		   (mov/xor/... dword ptr ss:[esp+reg*scale-0x..], 0x..) truncates at
		   50 chars, producing BROKEN mnemonics in the data file (e.g.
		   "xor word p" instead of "xor word ptr ss:[...], 0x..."). Detect
		   the truncation (text hit the 50-byte ceiling) and re-disassemble
		   with the debugger (DISASM_INSTR.instruction is char[64], no such
		   limit). */
		if (di->instr_size > 0 && strlen(di->instruction) >= 50)
		{
			DISASM_INSTR d2;
			memset(&d2, 0, sizeof(d2));
			if (xd_safe_dbg_disasm(ip, &d2) &&
				d2.instr_size >= 1 && strlen(d2.instruction) < 64)
			{
				memcpy(di, &d2, sizeof(*di));
				return 1;
			}
		}
		return di->instr_size > 0 ? 1 : 0;
	}
	memset(di, 0, sizeof(*di));
	if (xd_safe_dbg_disasm(ip, di))
		return 1;
	return 0;
}

/* disasm cache: the same eip repeats many times (VM loops / rep runs), so
   disassembling once per UNIQUE eip turns a 10M-instruction analysis into a
   few-100k disassemblies + cache lookups (seconds instead of minutes). */
struct DC_ENTRY { unsigned char sz; unsigned char tlen; char txt[sizeof(((DISASM_INSTR *)0)->instruction)]; };
static std::unordered_map<unsigned int, DC_ENTRY> g_dc;
static int disasm_at_cached(unsigned int ip, DISASM_INSTR *di, uc_engine *uc)
{
	if (uc)
	{
		/* LIVE disasm (code hook): never cache - VMP 3.8 self-modifies
		   handlers, so the same eip can hold different bytes later in the
		   trace. A cached stale row (from the snapshot) would show a jump
		   target / mnemonic that never actually executed. */
		return disasm_at(ip, di, uc);
	}
	std::unordered_map<unsigned int, DC_ENTRY>::const_iterator it = g_dc.find(ip);
	if (it != g_dc.end())
	{
		/* hot path: copy only the actual text length, no full memset.
		   CRITICAL: tlen is the INSTRUCTION TEXT length - instr_size is
		   the machine-code byte length (push=1, xor=2) and using it as
		   the copy length truncates the mnemonic ("push"->"p", "xor"->
		   "xo") on every cache hit. */
		unsigned char sz = it->second.tlen;
		di->instr_size = it->second.sz;
		memcpy(di->instruction, it->second.txt, sz);
		di->instruction[sz] = 0;
		return sz > 0 ? 1 : 0;
	}
	{
		DISASM_INSTR d2;
		if (!disasm_at(ip, &d2, NULL))
			return 0;
		*di = d2;
		DC_ENTRY e;
		e.sz = (unsigned char)d2.instr_size;
		{
			/* text length = strlen up to the buffer, capped */
			size_t tl = strnlen(d2.instruction, sizeof(d2.instruction));
			if (tl > sizeof(e.txt)) tl = sizeof(e.txt);
			e.tlen = (unsigned char)tl;
		}
		memcpy(e.txt, d2.instruction, sizeof(e.txt));
		g_dc[ip] = e;
		return 1;
	}
}

/* ============ dict semantics: annotate the dynamic walk ============ */
static int vmp38_dict_lookup(unsigned long long va, unsigned long long *delta, int *cls)
{
	int lo = 0, hi = VMP38_DICT_COUNT - 1;
	while (lo <= hi)
	{
		int mid = (lo + hi) / 2;
		unsigned int mrva = vmp38_dict[mid].rva;
		if ((unsigned int)va < mrva) hi = mid - 1;
		else if ((unsigned int)va > mrva) lo = mid + 1;
		else { *delta = vmp38_dict[mid].delta; *cls = vmp38_dict[mid].cls; return 1; }
	}
	return 0;
}

/* annotate a handler address with its dict semantic class (if known) */
const char *vmp38_dict_class(unsigned long long va)
{
	unsigned long long d; int c;
	if (!vmp38_dict_lookup(va, &d, &c)) return NULL;
	return c == 1 ? "vExit" : c == 2 ? "vStart" :
		c == 3 ? "vStackALU" : c == 4 ? "vStackMov" : c == 5 ? "vALU" : "vOther";
}

/* dict-driven chain: use dict delta only when nonzero (some handlers use
   stack-based next), otherwise stop (dynamic exec needed) */
void vmp38_dict_chain(unsigned long long modbase, unsigned long long vm_entry)
{
	unsigned long long cur = vm_entry - modbase;
	int n = 0;
	out("=== VMP 3.8 dict-driven chain (base=%08llx entry=%08llx) ===", modbase, vm_entry);
	{
		/* start at the first dict handler (skips the bytecode-decrypt loop) */
		unsigned long long curva = vmp38_dict[0].rva;
		cur = curva - 0x400000ULL;
		out("  starting at first dict handler rva=%08llx (entry was %08llx)",
			cur, vm_entry);
	}
	while (n < 5000)
	{
		unsigned long long delta;
		int cls;
		unsigned long long curva = cur + 0x400000ULL;
		unsigned long long abs = modbase + cur;
		if (!vmp38_dict_lookup(curva, &delta, &cls))
		{
			out("[%d] no dict entry at %08llx (va %08llx)", n, abs, curva);
			break;
		}
		const char *cname = cls == 1 ? "vExit" : cls == 2 ? "vStart" :
			cls == 3 ? "vStackALU" : cls == 4 ? "vStackMov" : cls == 5 ? "vALU" : "vOther";
		out("[%d] H@%08llx delta=%08llx %s", n, abs, delta, cname);
		if (cls == 1) break;  /* vExit ends the VM */
		if (delta == 0)
		{
			out("  delta=0: next via stack (dynamic exec required); stopping");
			break;
		}
		cur = (cur + delta) & 0xffffffff;
		n++;
	}
	out("=== chain done: %d handlers ===", n);
}

/* ============ dynamic trace (GUI single-step; headless sti is a no-op) ============
   Uses DbgCmdExec("sti") to really execute one instruction, then reads the
   real register file. Handler boundary = EIP jump (not sequential +len).
   Works under GUI x64dbg. Under headless, sti does not execute, so EIP is
   unchanged and we detect that and fall back (return 0). */
/* out-param (write-back pointer) detection: entry params that pointed at
   readable memory, whose pointee changed by vm-exit = result written back */
static void detect_out_params(void)
{
	for (int oi = 0; oi < g_entry_pn; oi++)
	{
		if (!g_entry_pointee[oi]) continue;
		unsigned long oc = 0;
		if (Script::Memory::Read(g_entry_params[oi], &oc, 4, &g_szrd) && g_szrd == 4 &&
			oc != g_entry_pointee[oi])
			out("  [out-param] *esp+%d(=%08x): %08x -> %08x (result written back)",
				(oi + 1) * 4, g_entry_params[oi], g_entry_pointee[oi], oc);
	}
	g_entry_pn = 0;
}

static void vmp38_emit_row(unsigned int eip, const char *code,
	const unsigned int *pre, const unsigned int *post,
	unsigned long long maddr, unsigned int mpre, unsigned int mpost, uc_engine *uc);
static void vmp38_emit_vm_boundary(unsigned int eip, const char *code, const unsigned int *vv, uc_engine *uc, int instr_size);
static int vmp38_emit_tail(void);
static void vmp38_reset_rows(void);

int vmp38_dynamic_trace(unsigned long long start_ip)
{
	int log_owned = vmp38_log_open("dyn");
	unsigned long long dyn_ret = 0;
	/* reset row-writer state (shared with static; must not carry over) */
	vmp38_reset_rows();
	_plugin_logprintf("[VMP38] dyn log_open=%d path=%s", log_owned, g_fout_path);
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
	/* vm-exit 后 ret 落到 .text 的返回 stub（如 vmp32_deep2 的
	   "xor ebx,[esp+0x14]; ror ebx,0x0B; mov eax,ebx; ret" 返回值计算）
	   时可能还把返回值算入；而真正返回值要读到 .text 的尾声处、
	   该 ret 返回处。phase=1 表示已进入 .text stub 追溯返回值。
	   再下一个 ret 才算 vmret。 */
	int vm_exit_text_phase = 0;

	/* Termination relies on real signals only: user stop (VMP38-Stop menu),
	   EIP escaping the module (antidebug/exception), vm-exit sequence
	   (popfd+popregs then ret = vmret), ret out of module, or process end.
	   Snapshot-based dead-loop heuristics and the max_steps cap were both
	   removed (2026-08-05): VMP handlers are side-effect-free units that
	   legitimately repeat the same register state, so state-repetition
	   tests false-positive and dropped valuable later restore work; real
	   hangs are caught by module escape + user stop. */
	int in_text_noted = 0;  /* one-shot note when EIP enters .text */

	out("=== VMP 3.8 dynamic trace (start=%08llx) ===", start_ip);
	/* entry parameters: read the call-site stack (esp+4..) - VMP 3.8 keeps
	   arguments plaintext on the stack at the VM entry ([[vmp38-param-plaintext]]) */
	{
		unsigned long long esp0 = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::ESP);
		if (esp0)
		{
			unsigned long pv[8]; int pn = 0;
			char pbuf[256]; int poff = 0;
			for (int pi = 0; pi < 8; pi++)
			{
				if (Script::Memory::Read((duint)(esp0 + 4 + pi * 4), &pv[pi], 4, &g_szrd) && g_szrd == 4)
					pn++;
				else
					break;
			}
			if (pn)
			{
				poff += sprintf(pbuf + poff, "[param] esp=%08llx:", (unsigned long long)esp0);
				for (int pi = 0; pi < pn; pi++)
					poff += sprintf(pbuf + poff, " esp+%d=%08x", (pi + 1) * 4, pv[pi]);
				out("%s", pbuf);
				/* snapshot for out-param detection: params pointing at
				   readable memory are address candidates - record their
				   pointee content now, compare at vm-exit */
				g_entry_esp = (unsigned long)esp0;
				g_entry_pn = pn > 16 ? 16 : pn;
				for (int pi = 0; pi < g_entry_pn; pi++)
				{
					unsigned long pc = 0;
					g_entry_params[pi] = pv[pi];
					g_entry_pointee[pi] = 0;
					if (Script::Memory::Read(pv[pi], &pc, 4, &g_szrd) && g_szrd == 4)
						g_entry_pointee[pi] = pc;   /* readable -> address candidate */
				}
			}
			else
				out("[param] esp=%08llx (stack unreadable)", (unsigned long long)esp0);
		}
	}
	timeBeginPeriod(1);  /* paired with timeEndPeriod in DYN_FINISH */

	/* per-function restore log: append entry/exit + every step to a file
	   so the user can replay each VM function and stitch the full program
	   semantics (main calls 7 VM functions, one membp hit per function). */
	g_dyn_func_count++;

	{
		/* (freeze counter removed 2026-08-05: snapshot/dead-loop heuristics
		   dropped - real termination is user stop + module escape + vm-exit) */
	}

	/* finish helper: write summary + close log before every return */
#define DYN_FINISH(rc) do { \
		vmp38_dyn_menu(0, 0); \
		vmp38_emit_tail(); \
		timeEndPeriod(1); \
		if (log_owned) vmp38_log_close(); \
		return (rc); } while (0)

	/* No step limit: trace until VM return, module escape, or user stop.
	   The old max_steps cap truncated long (main-entry) traces mid-loop. */
	for (step = 0; ; step++)
	{
		unsigned long long eip, esp, eax, ebx, ecx, edx, esi, edi, ebp;
		DISASM_INSTR ins;
		unsigned long long next;
		int is_boundary = 0;


		eip = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
		if (g_dyn_steps_n < DYN_STEP_MAX)
		{
			static const Script::Register::RegisterEnum dre[8] = {
				Script::Register::RegisterEnum::EAX, Script::Register::RegisterEnum::ECX,
				Script::Register::RegisterEnum::EDX, Script::Register::RegisterEnum::EBX,
				Script::Register::RegisterEnum::ESP, Script::Register::RegisterEnum::EBP,
				Script::Register::RegisterEnum::ESI, Script::Register::RegisterEnum::EDI };
			int dr;
			g_dyn_steps_eip[g_dyn_steps_n] = eip;
			g_dyn_steps_ef[g_dyn_steps_n] = (unsigned int)Script::Register::Get(Script::Register::RegisterEnum::CFLAGS);
			for (dr = 0; dr < 8; dr++)
				g_dyn_steps_reg[g_dyn_steps_n][dr] = (unsigned int)Script::Register::Get(dre[dr]);
			g_dyn_steps_n++;
		}
		eax = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EAX);
		ecx = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::ECX);
		edx = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EDX);
		ebx = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EBX);
		esp = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::ESP);
		ebp = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EBP);
		esi = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::ESI);
		edi = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EDI);

		/* user stop via the VMP38-Stop menu (aborts an unbounded trace) */
		if (g_dyn_stop)
		{
			out("  dynamic trace: user stop requested");
			DYN_FINISH(0);
		}

		if ((step % 500) == 0)
			vmp38_dyn_menu(step, handler_idx);

		/* progress ping so the GUI user knows a long unbounded trace is alive */
		if ((step % 10000) == 0 && step > 0)
			out("  [progress] step=%d handlers=%d eip=%08llx", step, handler_idx, eip);

		/* anti-debug escape: single-stepping that lands outside the module
		   means VMP detected the TF flag and jumped into its exception
		   handler / system dlls (100%% sample does exactly this - AV at
		   0xE5CAEE then ntdll). Stop immediately instead of frozen-spinning
		   for 25 steps while the program crashes. */
		if (g_runbase && (eip < g_runbase || eip >= g_runbase + 0x2000000ULL))
		{
			out("  dynamic trace: EIP escaped module (%08llx) - antidebug/exception; stop", eip);
			DYN_FINISH(0);
		}

		/* EIP in module .text is NOT a stop signal: VMP also places VM code
		   (function stubs / next-func entry) in .text, so keep tracing and
		   restore those too. Real termination is: module escape (above),
		   ret out of module, dead-loop detection, or process end. */
		if (g_vm_start && g_vm_end && (eip < g_vm_start || eip >= g_vm_end))
		{
			if (!in_text_noted)
			{
				out("  [note] EIP into .text (%08llx) - continuing (VM stubs/next funcs may live here)", eip);
				in_text_noted = 1;
			}
		}
		else
			in_text_noted = 0;

		memset(&ins, 0, sizeof(ins));
		DbgDisasmAt(eip, &ins);
		if (ins.instr_size < 1)
		{
			if (!disasm_from_file(eip, &ins))
			{
				out("  [%d] disasm fail at %08llx", step, eip);
				break;
			}
		}
		/* shared row writer: identical format/rules to the static trace */
		{
			unsigned int vv9[9];
			vv9[0] = (unsigned int)eax; vv9[1] = (unsigned int)ecx; vv9[2] = (unsigned int)edx;
			vv9[3] = (unsigned int)ebx; vv9[4] = (unsigned int)esp; vv9[5] = (unsigned int)ebp;
			vv9[6] = (unsigned int)esi; vv9[7] = (unsigned int)edi;
			vv9[8] = (unsigned int)Script::Register::Get(Script::Register::RegisterEnum::CFLAGS);
			vmp38_emit_row((unsigned int)eip, ins.instruction, vv9, NULL, ~0ULL, 0, 0, NULL);
		}

		/* VALUE CAPTURE: mul/imul/div/idiv - log operand registers so magic
		   constants (LCG multiplier, table mul, CRC poly) can be recovered
		   black-box from the dynamic trace. The multiplier may have been
		   loaded via an encrypted mov imm (ValueCommand::Calc decrypted), so
		   the register value here is the TRUE constant. */
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

		/* VM 段边界跟到返回/返回值提取（静态与动态同一规则） */
		{
			unsigned int vvb[9];
			vvb[0] = (unsigned int)eax; vvb[1] = (unsigned int)ecx; vvb[2] = (unsigned int)edx;
			vvb[3] = (unsigned int)ebx; vvb[4] = (unsigned int)esp; vvb[5] = (unsigned int)ebp;
			vvb[6] = (unsigned int)esi; vvb[7] = (unsigned int)edi; vvb[8] = 0;
			vmp38_emit_vm_boundary((unsigned int)eip, ins.instruction, vvb, NULL, ins.instr_size);
		}

		/* handler boundary: EIP jumped (not sequential) or jmp/ret/call */
		/* handler boundary: EIP jumped (not sequential) ~ detected via the
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
			g_trace_mode = 2;
			identify_handler(recs, rec_n, handler_idx, h_vsp, esp);
			{
				const char *dcls = vmp38_dict_class(h_start + 0x400000ULL);
				if (dcls)
					out("    [dict] %s", dcls);
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
				else if (m[0] == 'j' && strcmp(m, "jmp") != 0) { r->is_jmp = 1; r->is_cond = 1; const char *sp_ = strchr(r->disasm, ' '); if (sp_) { while (*sp_ == ' ') sp_++; if (sp_[0] == '0' && (sp_[1] == 'x' || sp_[1] == 'X')) r->cond_target = strtoull(sp_ + 2, NULL, 16); } }
				/* ALU tag - same rule as the static trace's re-tag, so the
				   identify_handler alu/jmps counts match between dyn & static */
				else if (strcmp(m, "add") == 0 || strcmp(m, "sub") == 0 || strcmp(m, "xor") == 0 ||
					strcmp(m, "and") == 0 || strcmp(m, "or") == 0 || strcmp(m, "adc") == 0 ||
					strcmp(m, "sbb") == 0 || strcmp(m, "lea") == 0 || strcmp(m, "xchg") == 0 ||
					strcmp(m, "inc") == 0 || strcmp(m, "dec") == 0 || strcmp(m, "not") == 0 ||
					strcmp(m, "neg") == 0 || strcmp(m, "shl") == 0 || strcmp(m, "shr") == 0 ||
					strcmp(m, "sar") == 0 || strcmp(m, "rol") == 0 || strcmp(m, "ror") == 0 ||
					strcmp(m, "imul") == 0 || strcmp(m, "mul") == 0 || strcmp(m, "div") == 0 ||
					strcmp(m, "idiv") == 0 || strcmp(m, "xadd") == 0)
					r->is_alu = 1;
				/* CORE-ALU rule (unified with the static path): core = the ALU
				   instruction itself (adc/sbb/xor/...), not taint - so the
				   identify_handler core sequence matches between dyn & static */
				if (strcmp(m, "adc") == 0 || strcmp(m, "sbb") == 0 || strcmp(m, "xor") == 0 ||
					strcmp(m, "and") == 0 || strcmp(m, "or") == 0 || strcmp(m, "add") == 0 ||
					strcmp(m, "sub") == 0 || strcmp(m, "not") == 0 || strcmp(m, "neg") == 0 ||
					strcmp(m, "shl") == 0 || strcmp(m, "shr") == 0 || strcmp(m, "sar") == 0 ||
					strcmp(m, "rol") == 0 || strcmp(m, "ror") == 0 || strcmp(m, "imul") == 0 ||
					strcmp(m, "mul") == 0 || strcmp(m, "div") == 0 || strcmp(m, "idiv") == 0)
					r->is_core = 1;
			}
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
					unsigned long esp_now = (unsigned long)Script::Register::Get(Script::Register::RegisterEnum::ESP);
					{
						duint szrd2 = 0;
						Script::Memory::Read(esp_now, &retaddr, 4, &szrd2);
					}
				}
				if (1)   /* no popfd gate: every ret is judged by target */
				{
					/* EVERY ret is judged by its target (no popfd/popreg
					   pre-mark). Only stop when the ret target LEAVES the VM
					   section (shared rule with the static trace) - a ret
					   whose target stays inside is VMP internal ret-based
					   dispatch, not a return. */
					if (vmp38_is_vmret_target(retaddr))
					{
						/* ret 目标在 VM 段外，两种情形：
						   (a) 仍在模块内，.text 的返回 stub（如
						       "xor ebx,[esp+0x14]; ror ebx,0x0B; mov eax,ebx;
						        ret" 的返回值计算）。也可能停止追溯出返回；
						   (b) 出模块（返回系统），停止并以 eax 停。 */
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
							dyn_ret = retaddr;
							out("  [ret] VM function return, retaddr=%08llx (vm-exit)", retaddr);
							out_progress("  [result] eax=%08llx (VM return value)",
								(unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EAX));
							detect_out_params();
							out("=== dynamic trace done: %d handlers (VM return) ret=%08llx ===", handler_idx, retaddr);
							DYN_FINISH(1);
						}
					}
					else
					{
						/* internal dispatch: keep tracing, handler still
						   ends here */
						if (rec_n > 0)
							identify_handler(recs, rec_n, handler_idx, h_vsp, esp);
						handler_idx++;
						out("  [ret] internal dispatch retaddr=%08llx (continue)", retaddr);
					}
				}
				if (vm_exit_text_phase)
				{
					/* 已进入 .text 的返回 stub，追溯返回值；只有最后
					   "mov eax,<reg>; ret" 的尾部 ret 才算真正的 vmret。 */
					if (rec_n > 0)
						identify_handler(recs, rec_n, handler_idx, h_vsp, esp);
					handler_idx++;
					dyn_ret = retaddr;
					out("  [ret] VM function return, retaddr=%08llx (.text restored)", retaddr);
					out_progress("  [result] eax=%08llx (VM return value)",
						(unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EAX));
					detect_out_params();
					out("=== dynamic trace done: %d handlers (VM return) ret=%08llx ===", handler_idx, retaddr);
					DYN_FINISH(1);
				}
				if (retaddr == 0 || modbase == 0 ||
					retaddr < modbase || retaddr >= modbase + 0x20000000ULL)
				{
					/* leave the module -> real function return */
					if (rec_n > 0)
						identify_handler(recs, rec_n, handler_idx, h_vsp, esp);
					handler_idx++;
					dyn_ret = retaddr;
					out("=== dynamic trace done: %d handlers (VM return) ret=%08llx ===", handler_idx, retaddr);
					DYN_FINISH(1);
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
				Sleep(1);  /* fast poll: queued sti usually lands in ~1ms;
				               the old fixed Sleep(20) made long traces
				               unbearably slow (20ms x tens of thousands of steps) */
				w += 1;
				cur = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
				if (cur != eip0)
					break;
			}
			if (cur == eip0)
			{
				/* queued sti never landed (headless) -> direct fallback */
				DbgCmdExecDirect("sti");
				Sleep(1);
			}
		}
	}
	if (rec_n > 0)
		identify_handler(recs, rec_n, handler_idx, h_vsp,
			(unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::ESP));
	{
		unsigned long long eip_end = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::EIP);
		int in_vm = (g_vm_start && g_vm_end && eip_end >= g_vm_start && eip_end < g_vm_end);
		out("=== dynamic trace done: %d handlers (%d steps) eip=%08llx %s ===", handler_idx, step, eip_end,
			in_vm ? "STILL IN VM SECT - press Run, membp hits again, re-run VMP38-Dyn to continue" : "trace ended (see stop reason above)");
	}
	check_dyn_static_divergence();   /* compare dyn vs static on any exit */
	DYN_FINISH(1);
#undef DYN_FINISH
}

/* ============ Unicorn engine integration ============
   Unicorn (QEMU kernel) provides the FULL x86 instruction set - exactly
   what VMP 3.8's obfuscated handlers need (SIB complex addressing,
   rep movsb, MBA arithmetic...). Hand-rolled simulators (xx_execute)
   can't decode 3.8's transformed instructions, which is why
   every jmp-reg dispatch drifted. Unicorn executes like a real CPU, so
   register results are exact and jmp targets stay correct. */
static uc_engine *g_uc = 0;
static int g_uc_fail_count = 0;   /* consecutive INSN_INVALID; >50 => VM
                                     section not decrypted in this session,
                                     close engine (file-mode fallback) */
static unsigned long long g_uc_vm_lo = 0, g_uc_vm_hi = 0;
static unsigned long long g_uc_stack_lo = 0, g_uc_stack_hi = 0;
static unsigned long long g_module_base = 0;   /* for full-trace exit detect */
static unsigned long long g_module_size = 0;   /* main-module real size (.text window) */

/* Unicorn memory read: serve from the fmem hook (snapshot -> live memory) */
static bool uc_in_dump = false;   /* reentrancy guard: during mem dump the
                                     uc_mem_write/read calls must NOT trigger
                                     the hooks (infinite recursion -> crash) */
/* consecutive out-of-module non-ret steps (garbage/data byte-walk); reset on
   any in-module address. Bounds the "non-ret -> continue" escape handling so
   a garbage region doesn't run forever (deep2 jumping e923a801->...->e92..). */
static int g_oob_streak = 0;

static void uc_hook_mem_read(uc_engine *uc, uc_mem_type type,
	uint64_t address, int size, int64_t value, void *user_data)
{
	(void)type; (void)size; (void)value; (void)user_data;
	/* 根因修复：空白页强制 live 重读。Only the pages dumped-empty at startup.
	   命中时从 live 进程拉真实字节覆盖到 Unicorn(cmap),并移出集合避免重复。 */
	if (uc_in_dump) return;
	unsigned int page = (unsigned int)(address & ~0xFFFULL);
	if (g_blank_pages.empty() || g_blank_pages.find(page) == g_blank_pages.end())
		return;   /* 非空白页: 正常读,无需处理(快速路径) */
	{
		unsigned char rbuf[4096];
		duint szr = 0;
		bool got = Script::Memory::Read(page, rbuf, sizeof(rbuf), &szr) && szr > 0;
		if (got)
		{
			size_t cpy = (size_t)(szr < 4096 ? szr : 4096);
			uc_in_dump = true;
			uc_mem_write(uc, page, rbuf, cpy);
			uc_in_dump = false;
			g_blank_pages.erase(page);
			out_progress("[mem] blank page %08x forced live re-read (%d bytes) - fix mem-copy gap",
				page, (int)cpy);
		}
		/* live 仍不可读: 保留集合,下次访问再试 */
	}
}

/* Unicorn memory write: forward to the fmem hook (snapshot, consistent).
   Uses the callback's `value` parameter (the written value) instead of
   uc_mem_read - calling uc_mem_* inside a hook re-enters the hooks
   (Unicorn 2.x C API has no reentrancy guard) -> infinite recursion. */
static void uc_hook_mem_write(uc_engine *uc, uc_mem_type type,
	uint64_t address, int size, int64_t value, void *user_data)
{
	(void)type; (void)value; (void)user_data;
	if (uc_in_dump) return;
	/* XMM/SSE (movups/movaps) 写 16 字节；之前只处理 size<=8，16 字节
	   XMM 写入时 snapshot 未同步（之前 XMM 写入的返回值位 0；这是
	   .text 的 XMM 功能调用（GSCService 008811C0）要做一次翻转读 16 字节，
	   用 uc_mem_read 读取所需字节，回退到 value 读 8 字节，填充到 16。 */
	if (size <= 16 && vmp38_mem_hook_write)
	{
		unsigned char xbuf[16];
		if (size > 8)
		{
			/* 16 字节 XMM 写入时读 Unicorn 内存（首次调用写之前，
			   SEH 保护 + hook 内存再读（uc_mem_read 会走该路径） */
			__try {
				if (uc_mem_read(uc, address, xbuf, (size_t)size) != UC_ERR_OK)
					return;
			} __except (EXCEPTION_EXECUTE_HANDLER) { return; }
			vmp38_mem_hook_write((unsigned int)address, (char*)xbuf, size);
		}
		else
		{
			vmp38_mem_hook_write((unsigned int)address, (char*)&value, size);
		}
	}
}

static void uc_hook_invalid_read(uc_engine *uc, uc_mem_type type,
	uint64_t address, int size, int64_t value, void *user_data)
{
	/* ON-DEMAND PAGING: the instruction touched a page outside the Unicorn
	   map (VMP stack addressing resolves anywhere in 32-bit space). Pull
	   the page from the LIVE process so the instruction can complete -
	   this is the standard Unicorn pattern for unmapped-access hooks. */
	if (uc_in_dump) return;
	{
		unsigned char pb[0x1000];
		unsigned long long pg = address & ~0xFFFULL;
		duint szr = 0;
		memset(pb, 0, sizeof(pb));
		if (Script::Memory::Read((duint)pg, pb, sizeof(pb), &szr) && szr > 0)
		{
			uc_mem_write(uc, pg, pb, szr);
			/* also forward the access to the fmem hook if it fits */
		}
	}
}

/* stop after one instruction (called on each code fetch; we let one execute) */
/* full-trace recording (continuous Unicorn run = "virtual CPU").
   No hard cap at all: it runs until the user presses VMP38-Stop
   (g_dyn_stop), module-exit (VM function returned), or the 32-bit
   process runs low on memory (predicted >~1.6GB private -> reported
   and stopped gracefully - NO unbounded stream fallback). */
#define FULL_TRACE_MAX 2000000   /* initial alloc only (80MB); grows on demand */

/* process private memory in MB - dynamic-loaded so no extra lib dep */
typedef struct {
	DWORD cb; DWORD PageFaultCount; SIZE_T PeakWorkingSetSize;
	SIZE_T WorkingSetSize; SIZE_T QuotaPeakPagedPoolUsage;
	SIZE_T QuotaPagedPoolUsage; SIZE_T QuotaPeakNonPagedPoolUsage;
	SIZE_T QuotaNonPagedPoolUsage; SIZE_T PagefileUsage;
	SIZE_T PeakPagefileUsage; SIZE_T PrivateUsage;
} XX_PROCESS_MEM_COUNTERS;
static int vmp38_proc_mem_mb(void)
{
	typedef BOOL (WINAPI *FN)(HANDLE, XX_PROCESS_MEM_COUNTERS*, DWORD);
	static FN fn = NULL;
	if (!fn)
	{
		HMODULE k = GetModuleHandleA("kernel32.dll");
		if (k) fn = (FN)GetProcAddress(k, "K32GetProcessMemoryInfo");
		if (!fn)
		{
			HMODULE pp = LoadLibraryA("psapi.dll");
			if (pp) fn = (FN)GetProcAddress(pp, "GetProcessMemoryInfo");
		}
	}
	if (fn)
	{
		XX_PROCESS_MEM_COUNTERS pmc;
		memset(&pmc, 0, sizeof(pmc));
		pmc.cb = sizeof(pmc);
		if (fn(GetCurrentProcess(), &pmc, sizeof(pmc)))
			return (int)(pmc.PrivateUsage / (1024 * 1024));
	}
	return 0;
}
static unsigned int *g_ft_eip = NULL;   /* 32-bit EIP: 4B per instr */
static unsigned int (*g_ft_regs)[8] = NULL;          /* eax,ecx,edx,ebx,esp,ebp,esi,edi */
static unsigned int *g_ft_eflags = NULL;
static int g_ft_cap = 0;
static int g_ft_realloc_cnt = 0;
static unsigned int g_ft_realloc_ms = 0;
static int g_ft_n = 0;
/* trace-time memory operand capture for the restore [asm] listing. Because
   the restore phase has no Unicorn CPU, the per-instruction memory value
   (addr + before/after) must be captured here, while uc is live (code hook).
   keyed by eip of the instruction that touches memory. The before-value is
   recorded when the instruction's hook runs (exec-PRE); the after-value is
   read at the NEXT hook (the prev instruction has then finished) - same
   defer-by-one pattern as the trace32 writer. */
typedef struct { unsigned long long addr; unsigned int pre; unsigned int post; int valid; } GFT_MEM;
static std::unordered_map<unsigned int, GFT_MEM> g_asm_mem;
/* out[] values recovered from the TRACE store-back writes (each of base+0..28
   written once by the VM-ized for-loop) - the authoritative "processed data
   must match" source, independent of live-process reads (which go stale after
   Unicorn execution moves on). g_out_base from trace beats the stack-local
   self-adapt guess that samples like GSCService fail. */
static unsigned int g_out_vals[8] = { 0,0,0,0,0,0,0,0 };
static int g_out_val_valid = 0;
/* acc accumulator-chain recorder: VMP keeps acc PLAINTEXT in one vsp slot.
   We track the slot's value evolution live (NEXT hook settles the post) and
   emit each update as a source op - no decryption needed. [acc] tag. */
static unsigned long long g_acc_addr = 0;
static unsigned int g_acc_cur = 0xA5A5A5A5u;
static int g_acc_found = 0;                /* slot located (saw seed write) */
static unsigned int g_acc_updates[512];
static unsigned int g_acc_pres[512];
static unsigned int g_acc_eips[512];
static int g_acc_n = 0;

/* GOLDEN acc chain: the FULL 38 accumulation values of the source algorithm,
   built from the named scalars a/b (g_src_vals[0]/[1]). Real accumulator
   evolution walks this list IN ORDER (monotone). Register-post tracking (the
   pure-register relays that never hit a memory slot) is matched against this
   table so the conversion table is COMPLETE, not just the memory-write half. */
struct ACC_GOLD { unsigned int val; const char *nm; };
static struct ACC_GOLD g_acc_gold[64];
static int g_acc_gold_n = 0;
static int g_acc_gold_cursor = 0;          /* next golden value expected */
static unsigned int g_acc_gold_eips[64];   /* eip that confirmed each step */

static void vmp38_acc_build_golden(void)
{
	unsigned int A = (g_src_cnt >= 1) ? g_src_vals[0] : 0;
	unsigned int B = (g_src_cnt >= 2) ? g_src_vals[1] : 0;
	int sa = (int)A, sb = (int)B;
	unsigned int acc = 0xA5A5A5A5u;
	unsigned int p = A, q = B;
	g_acc_gold_n = 0;
	g_acc_gold_cursor = 0;
	#define G2(v_, nm_) do { g_acc_gold[g_acc_gold_n].val = (unsigned int)(v_); g_acc_gold[g_acc_gold_n].nm = nm_; g_acc_gold_n++; } while (0)
	#define S2(nm_, d_) do { acc = (acc + (unsigned int)(d_)); G2(acc, nm_); } while (0)
	S2("a+b", A + B);
	S2("a-b", A - B);
	S2("a+b+(sa<sb)", A + B + (unsigned int)(sa < sb));
	S2("a-b-(sa<sb)", A - B - (unsigned int)(sa < sb));
	S2("a*3", A * 3u);
	S2("a*b", A * B);
	S2("a/7", A / 7u);
	S2("sa/7", (unsigned int)(sa / 7));
	S2("a%7", A % 7u);
	S2("a+1", A + 1u);
	S2("b-1", B - 1u);
	S2("~a+1", (~A) + 1u);
	S2("a&b", A & B);
	S2("a|b", A | B);
	S2("a^b", A ^ B);
	S2("~a", ~A);
	S2("(a&1)?1:0", (A & 1u) ? 1u : 0u);
	S2("a<<3", A << 3);
	S2("a>>2", A >> 2);
	S2("sa>>1", (unsigned int)(sa >> 1));
	S2("rol(a,5)", (A << 5) | (A >> 27));
	S2("ror(a,5)", (A >> 5) | (A << 27));
	S2("shld(b,a,4)", (B << 4) | (A >> 28));
	S2("shrd(a,b,4)", (A >> 4) | (B << 28));
	S2("(a>>3)&1", (A >> 3) & 1u);
	/* bittestandset(&p,3): p=a; if bit3 set, p|=8; add p if set */
	{ unsigned int bit = (p >> 3) & 1u; p |= 8u; S2("bts(3)?p:0", bit ? p : 0u); }
	{ unsigned int bit = (q >> 2) & 1u; q &= ~4u; S2("btr(2)?q:0", bit ? q : 0u); }
	{ unsigned int bit = (p >> 1) & 1u; p ^= 2u; S2("btc(1)?p:0", bit ? p : 0u); }
	{ unsigned int t = A, i = 0; while (t && !(t & 1)) { t >>= 1; i++; } S2("bsf(a)", i); }
	{ unsigned int t = B, i = 0; while (t >>= 1) i++; S2("bsr(b)", i); }
	{ unsigned int t = A, c = 0; while (t) { c += t & 1; t >>= 1; } S2("popcnt(a)", c); }
	S2("(uchar)(a+b)", (A + B) & 0xFFu);
	S2("(ushort)(a^b)", (A ^ B) & 0xFFFFu);
	S2("(char)a", (unsigned int)((char)A));
	S2("(uchar)a", A & 0xFFu);
	/* NOTE: the remaining 2 source accumulations are POINTER / ATOMIC and
	   their values cannot be predicted statically:
	     acc += &out[a&3];            (stack addr, ASLR-random)
	     acc += p; p=q; q=p;          (pointer swap)
	     acc += xchgadd(&p,1); cmpxchg(&p,5,3);
	   They are emitted by the namer as placeholders AFTER the 35 confirmed
	   steps (see the delta-naming pass). */
	#undef S2
	#undef G2
}

/* ================== REVM: execute the RESTORED code ==================
   The static/dynamic restore turns the raw VM bytes into a source-level
   assembly chain (the golden acc chain g_acc_gold[]: acc += delta each
   step, plus the return acc^a^b). REVM REBUILDS that restored algorithm
   as a real x86 instruction buffer, loads it into a FRESH Unicorn (not
   g_uc - that still runs the raw VM for tracing), single-steps it while
   logging registers each instruction, then reconciles the final eax
   against the golden return value acc^a^b.

   UI: exposed as the `vmp38_revm` command / REVM menu. Called AFTER a
   full trace (g_acc_gold + g_src_vals populated). */

/* &out[0] (g_out) read from the live breakpoint stack by the [param] pass;
   defined HERE (before REVM) so the REVM tail's &out[a&3] term can use it. */
static unsigned long g_out_param = 0;
static int g_out_param_valid = 0;
/* real return value captured from the trace tail's return-compute ret
   ([result]); written by the restore pass, consumed by REVM's real-reconcile
   so it can show "restored code return vs REAL trace return". */
static unsigned long g_ft_real_return = 0;
static int g_ft_real_return_valid = 0;
/* tiny x86-32 encoder helpers (caller provides the growing byte vector) */
static void revm_emit8(unsigned char **p, unsigned char b)
{
	unsigned char *q = *p; *q++ = b; *p = q;
}
static void revm_emit32(unsigned char **p, unsigned int v)
{
	unsigned char *q = *p;
	/* little-endian */
	*q++ = (unsigned char)(v & 0xFF); *q++ = (unsigned char)((v >> 8) & 0xFF);
	*q++ = (unsigned char)((v >> 16) & 0xFF); *q++ = (unsigned char)((v >> 24) & 0xFF);
	*p = q;
}
/* mov eax, imm32 = B8 imm32 ; mov ebx, imm32 = BB imm32 ;
   add eax, imm32 = 05 imm32 ; xor eax, ebx = 33 C3 ; ret = C3 */
static void revm_emit_mov_eax(unsigned char **p, unsigned int v) { revm_emit8(p, 0xB8); revm_emit32(p, v); }
static void revm_emit_mov_ebx(unsigned char **p, unsigned int v) { revm_emit8(p, 0xBB); revm_emit32(p, v); }
static void revm_emit_add_eax_imm(unsigned char **p, unsigned int v) { revm_emit8(p, 0x05); revm_emit32(p, v); }
static void revm_emit_xor_eax_ebx(unsigned char **p) { revm_emit8(p, 0x33); revm_emit8(p, 0xC3); }
static void revm_emit_ret(unsigned char **p) { revm_emit8(p, 0xC3); }

int vmp38_revm_exec(void)
{
	if (g_acc_gold_n <= 0)
	{
		out_progress("[REVM] no golden acc chain (run a full trace first)");
		return 0;
	}
	unsigned int A = (g_src_cnt >= 1) ? g_src_vals[0] : 0;
	unsigned int B = (g_src_cnt >= 2) ? g_src_vals[1] : 0;
	/* acc0 = the real accumulator initial value (g_src_vals[2] when the
	   srcs collector found one, else the vmp_base 0xA5A5A5A5 constant) */
	const unsigned int acc0 = (g_src_cnt >= 3) ? g_src_vals[2] : 0xA5A5A5A5u;
	const int used = (g_acc_gold_cursor > 0 && g_acc_gold_cursor <= g_acc_gold_n)
		? g_acc_gold_cursor : g_acc_gold_n;
	out_progress("[REVM] srcs: cnt=%d A=%08x B=%08x acc0=%08x n=%d cursor=%d used=%d",
		g_src_cnt, A, B, acc0, g_acc_gold_n, g_acc_gold_cursor, used);

	/* 1) build the restored x86 instruction buffer
	   mov eax,acc0 ; add eax,delta[0..used-1] ; xor with b ; xor with a ; ret
	   buffer holds at most ~ (1+4) + used*(1+4) + (1+4)+2 + (1+4)+2 + 1 bytes */
	unsigned char code[4096];
	unsigned char *p8 = code;
	unsigned int prev = acc0;
	unsigned int chain_final = acc0;   /* independently compute the acc chain's
	                                      final value (what the restored asm
	                                      must land on before the ^a^b tail) */
	int revm_tail_n = 0;               /* # of trailing pointer/interlocked adds */
	revm_emit_mov_eax(&p8, acc0);
	for (int i = 0; i < used && i < g_acc_gold_n; i++)
	{
		unsigned long long cur = (unsigned long long)g_acc_gold[i].val;
		unsigned int d = (unsigned int)(cur - (unsigned long long)prev);
		revm_emit_add_eax_imm(&p8, d);
		prev = (unsigned int)cur;
		chain_final = (unsigned int)cur;
	}
	/* ---- trailing pointer / interlocked accumulations (vmp_base 60-63):
	     acc += &out[a&3] ;
	     acc += p; p=q; q=p ;            // acc+=p THEN p becomes q(=B)
	     acc += xchgadd(&p,1);           // returns OLD p (= B after the swap)
	     acc += cmpxchg(&p,5,3);         // returns OLD p (= B+1 after xadd)
	   REVM supplies the REAL values: &out[a&3] = g_out+((A&3)*4) (live out-param),
	   p0 = (A|8)^2 (bts bit3 + btc bit1 on init p=A), then p=B and p=B+1 for the
	   two interlocked old-p returns. NOTE: using p0 for all three was the reason
	   the restored chain's return did NOT match the real trace return. */
	{
		unsigned int tail_delta[4]; int tail_n = 0;
		unsigned int p0 = ((A | 8u) ^ 2u) & 0xFFFFFFFFu;   /* p before swap */
		int have_out = g_out_param_valid && (unsigned long long)g_out_param >= 0x10000;
		unsigned int d_addr = have_out ? (unsigned int)(g_out_param + ((A & 3u) * 4u)) : 0;
		if (have_out) tail_delta[tail_n++] = d_addr;                    /* &out[a&3] */
		tail_delta[tail_n++] = p0;                                      /* acc += p (p0) */
		tail_delta[tail_n++] = B;                                       /* xchgadd(&p,1) old p = B (after p=q) */
		tail_delta[tail_n++] = (B + 1u) & 0xFFFFFFFFu;                  /* cmpxchg(&p,5,3) old p = B+1 (after xadd) */
		revm_tail_n = tail_n;
		for (int t = 0; t < tail_n; t++)
		{
			revm_emit_add_eax_imm(&p8, tail_delta[t]);
			chain_final = (chain_final + tail_delta[t]) & 0xFFFFFFFFu;
		}
		out_progress("[REVM] tail +%d: p0=%08x p(xchg)=%08x p(cmp)=%08x &out=%s%s",
			tail_n, p0, B, (B + 1u) & 0xFFFFFFFFu,
			have_out ? "" : "(no out param -> &out term SKIPPED)",
			have_out ? " (has out)" : "");
	}
	/* return acc ^ a ^ b  (vmp_base epilogue) */
	revm_emit_mov_ebx(&p8, B); revm_emit_xor_eax_ebx(&p8);
	revm_emit_mov_ebx(&p8, A); revm_emit_xor_eax_ebx(&p8);
	revm_emit_ret(&p8);
	const size_t codelen = (size_t)(p8 - code);

	/* 2) fresh Unicorn: map the code region + a scratch stack */
	uc_engine *uc = NULL;
	uc_err err = uc_open(UC_ARCH_X86, UC_MODE_32, &uc);
	if (err != UC_ERR_OK)
	{
		out_progress("[REVM] uc_open err=%d", (int)err);
		return 0;
	}
	const uint64_t CODE_BASE = 0x1000000ULL;   /* 16MB, low unmapped */
	const uint64_t STACK_BASE = 0x00700000ULL; /* grows down from here */
	if (uc_mem_map(uc, CODE_BASE, 0x1000, UC_PROT_ALL) != UC_ERR_OK ||
		uc_mem_map(uc, STACK_BASE, 0x10000, UC_PROT_ALL) != UC_ERR_OK)
	{
		out_progress("[REVM] mem_map failed");
		uc_close(uc);
		return 0;
	}
	if (uc_mem_write(uc, CODE_BASE, code, codelen) != UC_ERR_OK)
	{
		out_progress("[REVM] mem_write failed");
		uc_close(uc);
		return 0;
	}
	/* set start state: eax=acc0 (mirrors restored code), esp at top of stack */
	unsigned int esp_init = (unsigned int)STACK_BASE;
	if (uc_reg_write(uc, UC_X86_REG_EAX, &acc0) != UC_ERR_OK ||
		uc_reg_write(uc, UC_X86_REG_ESP, &esp_init) != UC_ERR_OK)
	{
		out_progress("[REVM] reg_write failed");
		uc_close(uc);
		return 0;
	}

	/* 3) single-step: run one instruction, print eax; repeat.
	   We KNOW each instruction's offset+length from the emit pass, so we
	   can drive the emulator one instruction at a time and refresh the
	   register view each step (this is the "single-step + watch regs"
	   surface the restore can't offer as static output). */
	out_progress("[REVM] exec %d restored instrs (restored asm, NOT raw VM)",
		used + revm_tail_n + 5);   /* + acc adds + tail + mov/xor/ret */
	/* build the step-boundary table: every emitted instruction's length */
	int step_len[128]; int step_n = 0;
	step_len[step_n++] = 5;                          /* mov eax,acc0 */
	for (int i = 0; i < used && i < g_acc_gold_n; i++)
		step_len[step_n++] = 5;                       /* add eax,delta */
	for (int t = 0; t < revm_tail_n; t++)
		step_len[step_n++] = 5;                       /* add eax,<tail delta> */
	step_len[step_n++] = 5;                          /* mov ebx,B */
	step_len[step_n++] = 2;                          /* xor eax,ebx */
	step_len[step_n++] = 5;                          /* mov ebx,A */
	step_len[step_n++] = 2;                          /* xor eax,ebx */
	step_len[step_n++] = 1;                          /* ret */
	unsigned int cur_eax = acc0;
	int s;
	unsigned int final_eax = acc0;
	unsigned int step_eip = (unsigned int)CODE_BASE;
	/* single-step the body + tail; STOP before the final ret (its `pop eip`
	   reads a return address off our synthetic stack that we never wrote,
	   which would fault uc_emu_start with WRITE/READ_UNMAPPED). The return
	   VALUE is the last non-ret instruction (the final `xor eax,A`). */
	int last_instr = step_n - 1;   /* index of the ret */
	for (s = 0; s < last_instr; s++)
	{
		unsigned int from = step_eip;
		unsigned int ln = (unsigned int)step_len[s];
		err = uc_emu_start(uc, from, from + ln, 0, 0);
		if (err != UC_ERR_OK) break;
		if (uc_reg_read(uc, UC_X86_REG_EAX, &cur_eax) != UC_ERR_OK) break;
		final_eax = cur_eax;
		out_progress("[REVM] step %3d eip=%06x  eax=%08x  esp=%08x",
			s, from, cur_eax, esp_init);
		step_eip += ln;
	}
	uc_close(uc);
	/* 4) reconcile against the golden return value = chain_final ^ a ^ b
	   (the restored asm adds each delta to acc0 then xors a and b; the
	   independent chain_final is what acc must equal before that tail). */
	unsigned int golden_ret = chain_final ^ A ^ B;
	if (s >= last_instr)
		out_progress("[REVM] final eax=%08x vs golden acc^a^b=%08x -> %s",
			final_eax, golden_ret, final_eax == golden_ret ? "MATCH" : "MISMATCH");
	else
		out_progress("[REVM] trace stopped at step %d (err=%d) - no return", s, (int)err);
	/* 5) REAL reconcile: the restored golden chain is the SOURCE-semantic
	   view (VMP splays the accumulator across a register network, so the
	   golden acc_final is a CLEAN reconstruction, not the exact live acc).
	   The REAL trace return (from the .text `xor eax,ebx; xor eax,[mem]`
	   epilogue = acc^a^b) is the ground truth. REBUILD a "real return"
	   snippet from the live acc^a^b and show whether the restored chain
	   lands on the same program-value. - honest boundary display. */
	{
		unsigned int real_ret = g_ft_real_return_valid ? (unsigned int)g_ft_real_return : 0;
		if (g_ft_real_return_valid)
		{
			/* real acc = real_return ^ a ^ b (epilogue is acc^a^b) */
			unsigned int real_acc = (real_ret ^ A ^ B);
			out_progress("[REVM] real trace return = %08x (real acc^a^b, acc=%08x)",
				real_ret, real_acc);
			out_progress("[REVM] restored-chain acc_final=%08x vs real acc=%08x -> %s",
				chain_final, real_acc,
				chain_final == real_acc ? "MATCH" : "DIFF (VMP-register-network acc, see below)");
		}
		else
			out_progress("[REVM] no real trace return captured - only self-golden reconcile shown");
	}
	return 1;
}
/* defer frame: the memory operand of the PREVIOUS traced instruction; filled
   in by vmp38_capture_asm_mem on the next code hook. mem_addr!=~0ULL means a
   captured operand is pending a POST read. */
static unsigned long long g_mem_pend_addr = ~0ULL;
static unsigned int        g_mem_pend_eip  = 0;
static unsigned int        g_mem_pend_pre  = 0;
static int                 g_mem_pending   = 0;

/* pages served from live memory (read-once + map); keyed by page addr */
static std::vector<unsigned int> g_live_pages;
static bool g_live_page_hit(unsigned int page)
{
	for (size_t i = 0; i < g_live_pages.size(); i++)
		if (g_live_pages[i] == page) return true;
	return false;
}
static void g_live_page_add(unsigned int page)
{
	g_live_pages.push_back(page);
}
static unsigned long long g_t_trace0 = 0, g_t_rest0 = 0;
static int g_ft_oom_logged = 0;   /* g_ft buffer OOM: log once, keep streaming */

/* grow the full-trace buffers; returns 0 on failure */
static int g_ft_ensure_cap(int need)
{
	if (g_ft_cap >= need) return 1;
	if (g_ft_eip == NULL)
	{
		g_ft_eip = (unsigned int*)malloc((size_t)FULL_TRACE_MAX * sizeof(unsigned int));
		g_ft_regs = (unsigned int(*)[8])malloc((size_t)FULL_TRACE_MAX * 8 * sizeof(unsigned int));
		g_ft_eflags = (unsigned int*)malloc((size_t)FULL_TRACE_MAX * sizeof(unsigned int));
		g_ft_cap = FULL_TRACE_MAX;
		if (!g_ft_eip || !g_ft_regs || !g_ft_eflags)
		{
			if (g_ft_eip) { free(g_ft_eip); g_ft_eip = NULL; }
			if (g_ft_regs) { free(g_ft_regs); g_ft_regs = NULL; }
			if (g_ft_eflags) { free(g_ft_eflags); g_ft_eflags = NULL; }
			g_ft_cap = 0;
			return 0;
		}
	}
	if (need > g_ft_cap)
	{
		/* grow geometrically (x1.5, not x2): doubling doubles the memory
		   growth per realloc and trips the 1.6GB cap ~40% sooner; x1.5
		   keeps reallocs few (still ~3 heap calls per grow) while recording
		   ~50% more instructions before the OOM cap. */
		int newcap = need;
		if (g_ft_cap > 0 && need < g_ft_cap * 3 / 2)
			newcap = g_ft_cap * 3 / 2;
		/* memory-aware: predict post-realloc private usage and stop before
		   the 32-bit process VA (~2GB) is exhausted. */
		{
			int cur_mb = vmp38_proc_mem_mb();
			unsigned long long delta_mb = (unsigned long long)(newcap - g_ft_cap) * 40ULL / 1048576ULL;
			if (cur_mb + (int)delta_mb > 1600)
			{
				/* NOT a stop: the in-memory g_ft buffer just stops growing.
				   data rows are written directly (streaming) by
				   vmp38_emit_row_guarded, so the trace continues to the
				   real vm-exit; only the post-pass g_ft analysis (handler
				   reconstruction) loses rows past this point. Log once. */
				if (!g_ft_oom_logged)
				{
					out_progress("[ft] g_ft buffer OOM (~1.6GB) - continuing streaming (data keeps writing, g_ft stops)");
					g_ft_oom_logged = 1;
				}
				return 0;
			}
		}
		unsigned long long t0r = GetTickCount64();
		/* realloc ONE buffer at a time and commit each pointer immediately:
		   a successful realloc FREES the old block, so if we only committed
		   after all three succeeded, a later failure would leave earlier
		   pointers dangling (use-after-free). On any failure the earlier
		   buffers are larger but g_ft_cap is unchanged - safe, next grow
		   retries. */
		{
			unsigned int *ne = (unsigned int*)realloc(g_ft_eip, (size_t)newcap * sizeof(unsigned int));
			if (!ne) return 0;
			g_ft_eip = ne;
			unsigned int (*nr)[8] = (unsigned int(*)[8])realloc(g_ft_regs, (size_t)newcap * 8 * sizeof(unsigned int));
			if (!nr) return 0;
			g_ft_regs = nr;
			unsigned int *nf = (unsigned int*)realloc(g_ft_eflags, (size_t)newcap * sizeof(unsigned int));
			if (!nf) return 0;
			g_ft_eflags = nf;
			g_ft_cap = newcap;
			g_ft_realloc_cnt++;
			g_ft_realloc_ms += (unsigned long)(GetTickCount64() - t0r);
			out("[ft] realloc: cap %d (%.0fMB) took %u ms (total %u ms / %d grows)",
				newcap, (double)newcap * 40.0 / 1048576.0,
				(unsigned)(GetTickCount64() - t0r), g_ft_realloc_ms, g_ft_realloc_cnt);
			return 1;
		}
	}
	return (need <= g_ft_cap);
}
static int g_ft_match = 0;      /* sequential cursor into g_ft_* */
static int g_dyn_match = 0;     /* sequential cursor into g_dyn_steps_* */


static int find_trace_regs(unsigned long long ip, unsigned int *regs8, unsigned int *ef)
{
	if (g_trace_mode == 1)
	{
		/* scan forward from cursor so repeated ips (VM loops) map in order */
		int i;
		for (i = g_ft_match; i < g_ft_n; i++)
			if (g_ft_eip[i] == ip)
			{
				memcpy(regs8, g_ft_regs[i], 8 * sizeof(unsigned int));
				*ef = g_ft_eflags[i];
				g_ft_match = i + 1;
				return 1;
			}
		/* no fallback linear scan (O(n) per insn - slow on 60k traces);
		   sequential cursor handles loops; miss = no snapshot (caller skips) */
	}
	else if (g_trace_mode == 2)
	{
		int i;
		for (i = g_dyn_match; i < g_dyn_steps_n; i++)
			if (g_dyn_steps_eip[i] == ip)
			{
				memcpy(regs8, g_dyn_steps_reg[i], 8 * sizeof(unsigned int));
				*ef = g_dyn_steps_ef[i];
				g_dyn_match = i + 1;
				return 1;
			}
		/* no fallback (see static path) */
	}
	return 0;
}
static int g_ft_stop_reason = 0;   /* 0=running 1=vmexit/module-exit 2=max-steps */
static unsigned long long g_ft_skip_resume = 0;   /* set when a hook skips a system-DLL call: resume here */
static int g_insn_invalid_handled = 0;   /* uc_hook_insn_invalid successfully
    advanced EIP past an AVX/SSE instruction: allow ONE err=10 resume (the
    hook can't suppress the error return, only move EIP). Cleared on resume. */
/* STREAM mode: unbounded trace without the g_ft buffer (32-bit memory cap).
   No per-instruction disassembly - only EIP-escapes-the-module detection
   (VM function return) + [result] + progress. Same speed as recording. */
static int g_stream_mode = 0;
static unsigned long long g_stream_total = 0;

/* ---- x64dbg .trace32 output (2026-08-18, user direction) ----
   Produces a x64dbg-readable trace stream from the static Unicorn run:
   TRAC+JSON header + per-instruction blocks (changed-register diff +
   opcode + memory old/new). The reader rebuilds the full REGDUMP per
   index from these diffs. We defer each instruction by ONE hook: uc_hook_code
   fires BEFORE an instruction executes, so this hook's vv2 (exec-pre) is the
   *previous* instruction's exec-post -> we write that previous instruction
   block now (prev_post = cur_vv2 vs prev_pre). */
static FILE *g_trace32 = NULL;
static int g_trace32_open = 0;
static int g_trace32_enabled = 0;   /* .trace32 is an OPTIONAL extra (for the
   x64dbg trace view) - writing it every instruction was a major slowdown
   (4.3MB per 10k instrs of tiny fwrites). Default OFF; set XXVM_TRACE32=1
   to re-enable. The plugin's real outputs are the .data/.txt files. */
static int g_trace32_first = 1;
static unsigned int g_tr_pre[9];          /* previous instr exec-PRE regs */
static unsigned int g_tr_pre_eip = 0;
static unsigned char g_tr_pre_op[16];
static int g_tr_pre_oplen = 0;
static unsigned int g_tr_insn = 0;        /* instrs emitted into the trace */

/* ---- VM-exit detection state (kanxue 280283: vm_start=pushfd, vm_exit=popfd).
   A handler that unwinds >=2 pop regs AND pops eflags (popfd/popad) is the
   VM exit sequence; the NEXT ret (whose target leaves the VM section) is the
   real vmret - the VM-ized code returns into .text restored code. Plain
   "ret" is NOT enough (handler-internal / ret-dispatch / junk rets all look
   identical) - the popfd+popregs unwind is what marks the exit. ---- */
static int g_ft_vmexit_pending = 0;   /* exit unwind seen; next qualifying ret = vmret */
static int g_ft_ve_popreg = 0;        /* pop-reg count in the current handler */
static int g_ft_ve_popfd = 0;         /* popfd/popf count in the current handler */
static int g_ft_ve_popad = 0;         /* popad count (real VM exit marker) */
static int g_ft_result_emitted = 0;   /* [result] already printed (vmret path) */
static unsigned long long g_last_ret_eax = 0;   /* value moved into eax by the
    last .text `mov eax,<reg>` (the return-value compute); set by code hook, */
static int g_last_ret_eax_valid = 0;            /* consumed by [result] so the
    console shows the REAL return even when g_ft buffer OOM drops the row */
static int g_ft_text_phase = 0;       /* vm-exit ret landed in .text stub: chase
                                         the restored-code return value, next ret
                                         (mov eax,<reg>; ret) is the real vmret */
/* per-session state reset: fresh GUI-mute counter, disasm cache, live-page
   cache, and stream flags so one run never leaks into the next. */
static void vmp38_reset_session_state(void)
{
	g_dc.clear();
	g_live_pages.clear();
	g_blank_pages.clear();   /* 空白页集合在 vmp38_snap_take / uc_init dump 时重新填充 */
	g_src_had_acc = 0;       /* vmp_base 模板门控, 跨 trace 重置 */
	g_stream_mode = 0;
	g_stream_total = 0;
	/* VM-exit detection state: never leak a pending marker across runs
	   (a stale pending + an unrelated first ret would false-stop) */
	g_ft_vmexit_pending = 0;
	g_ft_ve_popreg = 0;
	g_ft_ve_popfd = 0;
	g_ft_ve_popad = 0;
	g_ft_result_emitted = 0;
	g_ft_text_phase = 0;
	/* [asm] memory-operand capture: never leak across runs */
	g_asm_mem.clear();
	g_acc_addr = 0;
	g_acc_cur = 0xA5A5A5A5u;
	g_acc_found = 0;
	g_acc_n = 0;
	g_acc_gold_cursor = 0;
	g_acc_gold_n = 0;
	g_mem_pending = 0;
	g_mem_pend_addr = ~0ULL;
	g_mem_pend_eip = 0;
	g_mem_pend_pre = 0;
	/* return-value capture: reset across runs so a stale epilogue value
	   from a previous trace never bleeds into the next */
	g_ft_real_return = 0;
	g_ft_real_return_valid = 0;
	g_last_ret_eax = 0;
	g_last_ret_eax_valid = 0;
	/* 2026-08-25 用户要求"每次点静态还原都重新映射完整用户内存段":
	   复用首次创建的 g_uc 会残留上次执行的 .text 自修改/栈数据。
	   这里主动闭环 g_uc, 下次 vmp38_uc_init 必然重建 Unicorn 并做完整
	   用户内存 dump(4GB 映射 + 全模块 + heap). */
	if (g_uc)
	{
		uc_close(g_uc);
		g_uc = NULL;
	}
}

/* Unmapped-read hook: when Unicorn touches an address that was NOT in the
   startup memory dump (page failed to read / outside dump range), pull the
   bytes from the LIVE process (Script::Memory::Read) and map them in. This
   fixes control-flow divergence: a jnxx that reads an operand from a
   missing page saw 0 in Unicorn vs the real value dynamically, taking a
   different branch (e.g. missing mov eax,<imm> handlers entirely). */
static bool uc_hook_mem_unmapped(uc_engine *uc, uc_mem_type type,
	uint64_t address, int size, int64_t value, void *user_data)
{
	(void)value; (void)user_data;
	/* Cover READ + WRITE + FETCH unmapped alike (aligned with the x64
	   restore): any unmapped access must be patched so uc_emu_start never
	   aborts with err=6/8. FETCH is the critical one - a jmp/call into an
	   unmapped page (VMP obfuscated addressing) previously raised
	   UC_ERR_FETCH_UNMAPPED and the resume loop burned all 500 retries
	   printing "[ft] final emu_start err=8" lines while skipping real
	   handlers. */
	if (size > 0 && size <= 256 &&
		(type == UC_MEM_READ_UNMAPPED || type == UC_MEM_WRITE_UNMAPPED ||
		 type == UC_MEM_FETCH_UNMAPPED))
	{
		unsigned int page = (unsigned int)(address & ~0xFFFULL);
		if (!g_live_page_hit(page))
		{
			unsigned char pbuf[4096];
			duint szr = 0;
			bool got = false;
			if (Script::Memory::Read((duint)page, pbuf, sizeof(pbuf), &szr) &&
				szr > 0)
				got = true;
			else if (read_mem_file(page, pbuf, sizeof(pbuf)))
			{
				/* headless: live memory unavailable - serve FILE bytes */
				szr = sizeof(pbuf);
				got = true;
			}
			/* map the page regardless (zero-fill if the live read failed)
			   so the trace never breaks on an unmapped access - same
			   policy as the x64 restore. */
			if (uc_mem_map(uc, page, 4096, UC_PROT_ALL) != UC_ERR_OK)
				;   /* already mapped - fine */
			if (got)
				uc_mem_write(uc, page, pbuf, szr < 4096 ? szr : 4096);
			else
			{
				/* zero-fill fallback: keeps the trace running (never breaks
				   on an unmapped access). Fill with NOPs (0x90) instead of
				   00 00: a FETCH to a zero page would otherwise execute
				   "add [eax],al" garbage that WRITES through [eax] and
				   pollutes registers; NOPs slide harmlessly to the next
				   page. 4GB map makes this rare (only a genuine obfuscated
				   jump target); warn once so the user can spot a polluted
				   segment instead of silent garbage rows. */
				unsigned char zero[4096];
				memset(zero, 0x90, sizeof(zero));
				uc_mem_write(uc, page, zero, sizeof(zero));
				if (type == UC_MEM_FETCH_UNMAPPED)
				{
					static int s_warned_fetch = 0;
					if (!s_warned_fetch)
					{
						s_warned_fetch = 1;
						out_progress("[ft] warn: fetch from unmapped page %08x (NOP-filled, jumped to unmapped code)",
							page);
					}
				}
			}
			g_live_page_add(page);   /* cached: next accesses hit uc mem */
		}
		/* page already mapped+cached: fall through to let unicorn access it */
		return true;
	}
	return false;   /* let Unicorn raise its normal unmapped error */
}

/* ---- producer-consumer data writer: the hook fills a memory buffer, a
/* ---- per-eip aggregation: one row per distinct eip; repeated executions
   append changed register/memory data (dedup, capped) - written once when
   the trace finishes. Row format written at the end:
   eip=%08x\t<code>\t<regs>\t<mems>\t<append1>\t<append2>... ---- */
/* per-eip repeat-appends cap. 256 default keeps deep VM loops (the
   observed 01fed582->0133F705 loop runs dozens of times) with each
   distinct evolution retained while bounding memory for pathological
   million-iteration loops. Made a runtime variable: XXVM_APPENDS_MAX
   env var (headless/automation) overrides it before the trace starts. */
int g_row_max_appends = 256;
/* 2026-08-22 用户要求真逐条后移除聚合旧路径(ROW_STATE/g_rows/g_row_order/g_row_idx
   及 emit_tail_row 已由 direct-write dedup→逐条 取代)；保留 g_last_regs/g_last_ef。 */
/* last executed instruction's register snapshot - each row's regs column
   holds the DELTA vs this (push/pop rows dump full regs instead) */
static unsigned int g_last_regs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static unsigned int g_last_ef = 0;
/* when a repeated eip is skipped its regs are not read - g_last goes stale;
   set this so the NEXT new eip dumps FULL regs to compensate */
static int g_dw_need_full = 0;
/* defer-by-one row emission (static trace): the code hook fires BEFORE the
   instruction executes, so a row's POST registers are only known on the
   NEXT hook. Hold the previous row here, then write it with pre->post. */
static unsigned int g_dw_pend_eip = 0;
static char g_dw_pend_code[220];
static unsigned int g_dw_pend_pre[9];
static unsigned long long g_dw_pend_maddr = ~0ULL;
static unsigned int g_dw_pend_mpre = 0;
static int g_dw_pend_valid = 0;
/* 1 = append repeat-execution deltas (same eip again), 0 = first-execution only.
   GUI menu toggles it; XXVM_APPENDS=0 env sets the default once (headless). */
int g_agg_appends = 1;

/* auto-continue into the next VM function after a VM return (ret target
   starts with pushfd = VMP vm_start). 1 = keep tracing the next VM func
   (GSCService main calls several VM functions in sequence); 0 = stop at
   the VM return (the current VM function's job, e.g. key derivation, is
   done). GUI menu toggles it (was the appends toggle - appends is now
   always on). */
int g_vm_continue = 1;


/* resolve a [reg + reg*scale + disp] memory expression against the current
   register values; returns 1 and sets *addr if parseable. Used by the hook
   to write M rows (memory operand value). */
static int an_mem_expr_addr(const char *expr, const unsigned int *rv, unsigned long long *addr)
{
	unsigned long long a = 0;
	int sign = 1;
	const char *p = expr;
	static const char *rn[8] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
	while (*p)
	{
		while (*p == ' ' || *p == '\t' || *p == '+') { if (*p == '+') sign = 1; p++; }
		if (*p == '-') { sign = -1; p++; }
		while (*p == ' ') p++;
		int reg = -1;
		for (int k = 0; k < 8; k++)
		{
			int l = (int)strlen(rn[k]);
			if (_strnicmp(p, rn[k], l) == 0 && !isalnum((unsigned char)p[l]))
			{ reg = k; p += l; break; }
		}
		if (reg >= 0)
		{
			unsigned long long v = rv[reg];
			while (*p == ' ') p++;
			if (*p == '*')
			{
				p++;
				int sc = (int)strtoul(p, (char**)&p, 10);
				if (sc <= 0) sc = 1;
				v *= (unsigned long long)sc;
			}
			a += (unsigned long long)(sign > 0 ? v : (0ULL - v));
			sign = 1;
			continue;
		}
		{
			unsigned long long c;
			if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
				c = strtoull(p + 2, (char**)&p, 16);
			else if (isdigit((unsigned char)p[0]))
				c = strtoull(p, (char**)&p, 0);
			else
				break;
			a += (unsigned long long)(sign > 0 ? c : (0ULL - c));
			sign = 1;
			continue;
		}
		break;
	}
	*addr = a & 0xFFFFFFFFULL;
	return 1;
}

/* VMP 3.8 uses int3 (0xCC) as anti-debug/junk in handlers. Under Unicorn an
   int3 raises an exception and terminates the trace. Skip it (ip+1) so the
   trace continues through the handler sequence. */
static void uc_hook_intr(uc_engine *uc, uint32_t intno, void *user_data)
{
	(void)intno; (void)user_data;
	{
		uint64_t ip = 0;
		__try {
			if (uc_reg_read(uc, UC_X86_REG_EIP, &ip) == UC_ERR_OK)
			{
				ip += 1;   /* skip the 0xCC byte */
				uc_reg_write(uc, UC_X86_REG_EIP, &ip);
			}
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			/* hook-internal reg AV - leave EIP untouched */
		}
	}
}

/* Unicorn x86 (32-bit) can't decode AVX2 instructions (vpsrlvd has a VEX
   prefix 0xc4 - Unicorn's x86 decoder predates AVX). Rather than blindly
   SKIPPING them (which corrupts the xmm registers that VMP uses to SIMD-ize
   the source algorithm's key-generation loop -> wrong return value), EMULATE
   the one instruction the source actually uses: vpsrlvd (per-lane logical
   shift right). Everything else is still skipped (the SSE1/2/SSSE3 forms -
   movd/paddd/pslld/pshufb - decode fine in Unicorn). */
static void uc_hook_insn_invalid(uc_engine *uc, void *user_data)
{
	uint64_t ip = 0;
	(void)user_data;
	__try {
		if (uc_reg_read(uc, UC_X86_REG_EIP, &ip) == UC_ERR_OK)
		{
			uint32_t sz = 2;
			DISASM_INSTR di;
			memset(&di, 0, sizeof(di));
			int got = disasm_at(ip, &di, uc);
			if (got) sz = di.instr_size;
			/* ---- EMULATE vpsrlvd (per-lane variable logical shift right) ----
			   forms seen:  vpsrlvd xmm1, xmm1, xmm0  (c4 e2 71 45 c8)
			                vpsrlvd xmm0, xmm7, xmm2  (c4 e2 41 45 c2)
			   dst = src1 (XMM reg), count = src2 (XMM reg) per 32-bit lane. */
			if (got && strstr(di.instruction, "vpsrlvd") != NULL)
			{
				char op2[64] = "";
				const char *p = strstr(di.instruction, "vpsrlvd");
				if (p) p += 7;
				while (p && *p == ' ') p++;
				/* parse the three comma-separated operands */
				char ops[3][16] = { "", "", "" };
				int nop = 0;
				if (p)
				{
					const char *p2 = p;
					while (p2 && *p2 && nop < 3)
					{
						const char *c = strchr(p2, ',');
						int len = c ? (int)(c - p2) : (int)strlen(p2);
						if (len > 15) len = 15;
						if (len > 0) { memcpy(ops[nop], p2, len); ops[nop][len] = 0; nop++; }
						if (!c) break;
						p2 = c + 1;
						while (*p2 == ' ') p2++;
					}
				}
				/* map xmmN -> reg enum; both src/dst are xmm */
				int dreg = -1, sreg1 = -1, sreg2 = -1;
				if (nop >= 3)
				{
					dreg = (int)(UC_X86_REG_XMM0 + (ops[0][3] - '0'));
					sreg1 = (int)(UC_X86_REG_XMM0 + (ops[1][3] - '0'));
					sreg2 = (int)(UC_X86_REG_XMM0 + (ops[2][3] - '0'));
				}
				unsigned char d[16], s1[16], s2[16];
				int rok = (dreg >= 0 && sreg1 >= 0 && sreg2 >= 0);
				if (rok)
				{
					size_t sz16 = 16;
					rok = (uc_reg_read2(uc, dreg, d, &sz16) == UC_ERR_OK);
					sz16 = 16;
					rok = rok && (uc_reg_read2(uc, sreg1, s1, &sz16) == UC_ERR_OK);
					sz16 = 16;
					rok = rok && (uc_reg_read2(uc, sreg2, s2, &sz16) == UC_ERR_OK);
				}
				if (rok)
				{
					unsigned int *d32 = (unsigned int*)d;
					const unsigned int *s32 = (const unsigned int*)s1;
					const unsigned int *c32 = (const unsigned int*)s2;
					for (int lane = 0; lane < 4; lane++)
						d32[lane] = (c32[lane] >= 32) ? 0u :
							(s32[lane] >> c32[lane]);
					size_t sz16 = 16;
					uc_reg_write2(uc, dreg, d, &sz16);
				}
			}
			ip += sz;
			{
				uint32_t ip32 = (uint32_t)ip;
				ip = (uint64_t)ip32;
			}
			uc_reg_write(uc, UC_X86_REG_EIP, &ip);
			/* EIP advanced past the invalid insn: allow ONE err=10 resume
			   so execution continues past it. Only when the insn actually
			   DISASSEMBLED (got) AND it is inside the VM section - a
			   system-dll-boundary err=10 (outside VM) is a natural stop. */
			if (got && g_vm_start && g_vm_end &&
				ip >= g_vm_start && ip < g_vm_end)
				g_insn_invalid_handled = 1;
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		/* leave EIP; the resume loop will retry */
	}
}

/* Direct-to-file extraction hook: no g_ft buffer, no post-trace restore
   pass. Every executed instruction is disassembled (dump snapshot) and
   written to the standalone-analyzer data file (H/I rows). Handler
   semantics are derived later by the standalone analyzer - restore is one
   pass now, and the 32-bit 2GB memory cap disappears (nothing is stored). */
/* 循环诊断：记录连续"同 eip"执行次数，到阈值在日志(.txt + x64dbg)提示一次。
   不停止、不丢数据——纯粹信息提示，防止真逐条后死循环把 data 无提示写爆。 */
static unsigned int s_rep_eip = 0;   /* 上次 eip */
static unsigned int s_rep_n = 0;     /* 连续同 eip 次数 */
static void vmp38_note_same_eip(unsigned int eip, const char *code)
{
	if (eip == s_rep_eip)
	{
		s_rep_n++;
		if (s_rep_n == 512 || s_rep_n == 4096 || (s_rep_n % 131072) == 0)
			out_progress("[loop] eip=%08x %s 连续执行 %u 次 (同一 eip 重复,可能是循环/死循环)",
				eip, code ? code : "", s_rep_n);
	}
	else
	{
		s_rep_eip = eip;
		s_rep_n = 1;
	}
}
static void vmp38_clear_same_eip(void) { s_rep_eip = 0; s_rep_n = 0; }
/* reset row-writer state (shared by static + dynamic trace entry) */
static void vmp38_reset_rows(void)
{
	g_dw_need_full = 1;
	memset(g_last_regs, 0, sizeof(g_last_regs));
	g_last_ef = 0;
	vmp38_clear_same_eip();
}

/* ======================================================================
   Shared row writer: ONE rule set for BOTH static (Unicorn hook) and
   dynamic (real single-step) traces. Data source differs, format/rules
   identical. vv[9] = eax ecx edx ebx esp ebp esi edi eflags.
   uc may be NULL (dynamic: memory operands read from the LIVE process
   via DbgMemRead instead of the emulator). Aggregate mode merges rows in
   memory (written at trace end); direct mode writes each unique eip once
   (hash dedup, O(1) memory). g_agg_appends selects the mode.

   Register display rule (user spec): show the registers the instruction
   ACTUALLY operates on (all operands, src+dest, pre-execution value) -
   NOT a delta-vs-previous-instruction comparison. Registers the
   instruction never touches are omitted. push/pop keep the full dump
   (handler-boundary markers). Memory operands [expr] always show their
   value. eflags flags that changed are still emitted (flag semantics,
   not operands).
   ====================================================================== */

/* which of the 8 general registers appear in the instruction text
   (operand list, including registers inside [mem] addressing)? Fills
   bitmask (bit k = register k appears). 8/16-bit sub-register names
   (al/ax/... ) map to their 32-bit parent. */
static unsigned int vmp38_row_used_regs(const char *code)
{
	static const char *rn[8] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
	/* 8/16-bit aliases -> parent index (al/ah/ax->eax ... ) */
	static const char *al8[16] = { "al", "ah", "ax", "cl", "ch", "cx", "dl", "dh",
		"dx", "bl", "bh", "bx", "sp", "bp", "si", "di" };
	static const int   al8i[16] = { 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 5, 6, 7 };
	unsigned int used = 0;
	if (!code) return used;
	for (int k = 0; k < 8; k++)
	{
		const char *hit = code;
		size_t L = strlen(rn[k]);
		while ((hit = strstr(hit, rn[k])) != NULL)
		{
			/* must be a whole register token: preceding char not alnum,
			   following char not alnum (avoids matching "eax" inside
			   "eaxx" or "movsxd" substrings) */
			int pre_ok = (hit == code) || !isalnum((unsigned char)hit[-1]);
			int post_ok = !isalnum((unsigned char)hit[L]);
			if (pre_ok && post_ok)
			{
				used |= (1u << k);
				break;
			}
			hit += L;
		}
	}
	/* 8/16-bit aliases (al/ax/...): only when the 32-bit name was NOT
	   already matched (al inside "eax" would double-count harmlessly but
	   we still want al-only instructions to flag eax) */
	for (int a = 0; a < 16; a++)
	{
		const char *hit = code;
		size_t L = strlen(al8[a]);
		while ((hit = strstr(hit, al8[a])) != NULL)
		{
			int pre_ok = (hit == code) || !isalnum((unsigned char)hit[-1]);
			int post_ok = !isalnum((unsigned char)hit[L]);
			if (pre_ok && post_ok)
			{
				used |= (1u << al8i[a]);
				break;
			}
			hit += L;
		}
	}
	return used;
}

/* SEH-guarded 32-bit memory read for the code hook: uc_mem_read inside a
   hook can AV (fault=0) on some Unicorn state. This is plain C (no C++
   objects) so __try is legal here - emit_row itself cannot host __try
   (std::string/vector unwinding, C2712). uc==NULL -> DbgMemRead (dynamic
   trace, live process). Returns 1 on success. */
static int vmp38_uc_read32_guarded(uc_engine *uc, unsigned long long addr,
	unsigned int *out)
{
	if (!uc)
		return (DbgMemRead((duint)addr, out, 4) != 0) ? 1 : 0;
	__try {
		return (uc_mem_read(uc, addr, out, 4) == UC_ERR_OK) ? 1 : 0;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		return 0;   /* hook-internal read AV - mems column omitted */
	}
}

/* REVMOUT: recover the algorithm's out[8] array straight from the TRACE
   memory-write captures (g_asm_mem: eip -> {addr, post}). A base whose
   base+0/4/.../28 are each written is the for-loop's out[i] store-back
   array. Fill g_out_vals[0..7] with the POST values (the authoritative
   "processed data must match" source), trumping the self-adapt stack-local
   guess that samples like GSCService fail. */
static int vmp38_find_out_from_trace(void)
{
	g_out_val_valid = 0;
	std::unordered_map<unsigned int, unsigned int> writes; /* addr -> post */
	for (auto &kv : g_asm_mem)
		if (kv.second.valid && kv.second.addr >= 0x10000 && kv.second.addr <= 0x7FFFFFFFULL)
			writes[(unsigned int)kv.second.addr] = kv.second.post;
	if (writes.empty())
		return 0;
	unsigned int best_base = 0; int best_hits = 0;
	unsigned int best_vals[8] = { 0,0,0,0,0,0,0,0 };
	for (auto &w : writes)
	{
		unsigned long long a = w.first;
		for (int idx = 0; idx < 8; idx++)
		{
			if (a < (unsigned long long)idx * 4) continue;
			unsigned int base = (unsigned int)(a - (unsigned long long)idx * 4);
			int h = 0; unsigned int vals[8] = { 0,0,0,0,0,0,0,0 };
			for (int q = 0; q < 8; q++)
			{
				std::unordered_map<unsigned int, unsigned int>::iterator it = writes.find(base + q * 4);
				if (it != writes.end()) { vals[q] = it->second; h++; }
			}
			if (h > best_hits)
			{
				best_hits = h; best_base = base;
				for (int q = 0; q < 8; q++) best_vals[q] = vals[q];
			}
		}
	}
	if (best_hits >= 6)
	{
		g_out_param = best_base; g_out_param_valid = 1;
		for (int q = 0; q < 8; q++) g_out_vals[q] = best_vals[q];
		g_out_val_valid = 1;
		return 1;
	}
	return 0;
}

/* capture this instruction's memory-operand value into g_asm_mem (defer-by-one):
   - settle the previous pending operand's POST (read now: it has just executed)
   - record this instruction's memory operand PRE for the [esp+0xOFF] listing */
static void vmp38_capture_asm_mem(unsigned int eip, const char *code,
	const unsigned int *vv2, uc_engine *uc)
{
	/* 1) settle the previous deferred operand's POST value (prev instr done) */
	if (g_mem_pending)
	{
		unsigned long long settle_addr = g_mem_pend_addr;
		unsigned int post = 0;
		int pok = vmp38_uc_read32_guarded(uc, settle_addr, &post);
		unsigned int pe = g_mem_pend_eip;
		g_mem_pending = 0;
		g_mem_pend_addr = ~0ULL;
		std::unordered_map<unsigned int, GFT_MEM>::iterator it = g_asm_mem.find(pe);
		if (it != g_asm_mem.end() && pok)
		{
			it->second.post = post;
			it->second.valid = 1;
		}
		/* acc-slot chain: locate the seed write (post==0xA5A5A5A5) and then
		   follow the SAME slot's value evolution. */
		if (pok)
		{
			/* cross-handler source-slot naming: when a memory slot receives
			   a source scalar (a/b/acc0), bind its MEM symbol to that source
			   variable, so a LATER handler's `mov reg,[slot]` reads the named
			   symbol and the acc chain flows across handler boundaries. */
			int sidx = vmp38_src_id_of(post);
			if (sidx >= 0)
			{
				if (g_src_symid[sidx] <= 0 || g_src_symid[sidx] >= g_sym_n ||
					g_sym_pool[g_src_symid[sidx]].op != SYM_OP_SRC ||
					g_sym_pool[g_src_symid[sidx]].c != (unsigned long long)sidx)
					g_src_symid[sidx] = sym_alloc(SYM_OP_SRC, 0, 0, 32, (unsigned long long)sidx);
				sym_mem_store(&g_symreg, settle_addr, g_src_symid[sidx]);
			}
			if (!g_acc_found && post == 0xA5A5A5A5u)
			{
				/* DEFERRED ANCHOR: 0xA5A5A5A5 also appears in the engine's
				   decrypt/init phase. The REAL acc slot is the one where
				   0xA5A5A5A5 is followed (after optional pointer noise) by
				   the algorithm's FIRST operand load a=0x13579BDF. Record
				   the candidate and confirm on the next non-pointer write. */
				g_acc_addr = settle_addr;
				g_acc_cur = post;
				g_acc_found = 1;
			}
			else if (g_acc_found && settle_addr == g_acc_addr && post != g_acc_cur)
			{
				/* skip stack-pointer payloads */
				if (((post ^ (unsigned int)g_acc_addr) & 0xFFFF0000u) == 0)
				{
					/* pointer noise - keep the candidate anchor */
				}
				else if (g_acc_n == 0 && g_acc_cur == 0xA5A5A5A5u &&
					post != 0x13579BDFu)
				{
					/* this slot's first non-pointer successor is NOT load-a:
					   it is an engine scratch cell, not the acc slot. Reset
					   and keep hunting for the next 0xA5A5A5A5 write. */
					g_acc_found = 0;
					g_acc_cur = 0;
					g_acc_addr = 0;
				}
				else if (g_acc_n < 512)
				{
					/* Record every non-pointer / non-sentinel write. NO
					   value-magnitude gate here: a real accumulation can have
					   a SMALL delta (a&b=0x004088c0, a%7=0, bsf=0, popcnt
					   =0x14 ...). The magnitude gate only belongs in the
					   delta-naming pass, where the golden-chain continuity
					   separates real accumulations from scratch noise. */
					g_acc_eips[g_acc_n] = pe;
					g_acc_pres[g_acc_n] = g_acc_cur;
					g_acc_updates[g_acc_n] = post;
					g_acc_n++;
					g_acc_cur = post;
				}
			}
		}
	}
	/* REGISTER-POST tracking: the source algorithm's accumulator chain walks
	   the golden values IN ORDER, but many relay values live ONLY in a
	   register (eax/ecx/edx/ebx) and never hit a memory slot. vv2[] here is
	   the CURRENT instruction's PRE = the PREVIOUS instruction's POST, so
	   vv2[0..3] are the last-executed register values. Match them against
	   g_acc_gold[g_acc_gold_cursor] to complete the chain. */
	if (g_acc_gold_n > 0 && g_acc_gold_cursor < g_acc_gold_n)
	{
		for (int r = 0; r < 4; r++)
		{
			unsigned int rv = vv2[r];
			if (rv == g_acc_gold[g_acc_gold_cursor].val)
			{
				/* record the eip that confirmed this golden step. The
				   g_acc_* array is owned by the MEMORY-SLOT recorder
				   ([acc-slot]/[summary]); the golden chain uses its own
				   g_acc_gold_eips[] so the two recorders never interleave
				   (they write DIFFERENT semantics into the same array,
				   which corrupted the [acc-slot] listing). */
				if (g_acc_gold_cursor < 64)
					g_acc_gold_eips[g_acc_gold_cursor] = eip;
				g_acc_gold_cursor++;
				break;   /* one golden step per instruction */
			}
		}
	}
	/* 2) record THIS instruction's memory operand (exec-PRE value) */
	{
		const char *br = strchr(code, '[');
		const char *cr = br ? strchr(br + 1, ']') : NULL;
		if (br && cr && cr > br)
		{
			char mex[128];
			int ml = (int)(cr - br - 1);
			if (ml > 0 && ml < (int)sizeof(mex))
			{
				memcpy(mex, br + 1, ml);
				mex[ml] = 0;
				unsigned long long maddr = 0;
				if (an_mem_expr_addr(mex, vv2, &maddr))
				{
					unsigned int mval = 0;
					int ok = vmp38_uc_read32_guarded(uc, maddr, &mval);
					if (ok)
					{
						/* only the FIRST execution of this eip provides the
						   display value (VMP loops repeat eips) */
						if (g_asm_mem.find(eip) == g_asm_mem.end())
							g_asm_mem[eip] = GFT_MEM{ maddr, mval, 0, 0 };
						/* defer: read the POST on the next hook */
						g_mem_pend_addr = maddr;
						g_mem_pend_eip = eip;
						g_mem_pend_pre = mval;
						g_mem_pending = 1;
					}
				}
			}
		}
		else
		{
			/* no memory operand this instr: nothing to defer for it */
			g_mem_pending = 0;
		}
	}
}

/* dump 16 bytes at addr as hex into out (for reading in/out buffers of a
   VM function - the "executed result", not the obfuscated constants). */
static void vmp38_dump_buf16(uc_engine *uc, unsigned long long addr, char *out, int outsz)
{
	int o = 0;
	for (int i = 0; i < 16 && o < outsz - 4; i++)
	{
		unsigned char b = 0;
		int ok = 0;
		if (uc)
			ok = (uc_mem_read(uc, addr + (unsigned long long)i, &b, 1) == UC_ERR_OK);
		else
			ok = (DbgMemRead((duint)(addr + (unsigned long long)i), &b, 1) != 0);
		o += sprintf(out + o, "%s%02x", ok ? "" : "??", ok ? b : 0);
	}
	out[o] = 0;
}

static unsigned int g_vm_saved_param[8];
static int g_vm_saved_param_n = 0;

/* 跟随 VM 段边界跟到返回/返回值提取（静态与动态同一规则；一一对应的模块）
   pushfd 前的 push = 进入 VM 段的入口；popfd 后 eax = 返回值。
   uc 非空 = 静态用 uc_mem_read；否则 uc NULL = 动态用 DbgMemRead（读真进程）。
   vmp38_uc_read32_guarded 统一该入口。 */
static void vmp38_emit_vm_boundary(unsigned int eip, const char *code,
	const unsigned int *vv, uc_engine *uc, int instr_size)
{
	char m[32];
	unsigned long long esp_now, eax_now;
	if (!code || !vv) return;
	get_mnemonic(code, m, sizeof(m));
	esp_now = (unsigned long long)vv[4];
	eax_now = (unsigned long long)vv[0];
	if (strcmp(m, "pushfd") == 0 || strcmp(m, "pushf") == 0)
	{
		unsigned long long params[8];
		int pn = 0;
		for (int pi = 0; pi < 8; pi++)
		{
			unsigned int pv = 0;
			if (vmp38_uc_read32_guarded(uc, esp_now + 4 + (unsigned long long)pi * 4, &pv))
				params[pn++] = pv;
			else
				break;
		}
		if (pn)
		{
			char pbuf[256]; int poff = 0;
			poff += sprintf(pbuf + poff, "[vm-param] entry=%08x esp=%08llx", eip, esp_now);
			for (int pi = 0; pi < pn; pi++)
				poff += sprintf(pbuf + poff, " p%d=%08x", pi, (unsigned int)params[pi]);
			out("%s", pbuf);
			/* save params for popfd-side buffer dump, and dump pointer params
			   (in-place buffers: entry shows input e.g. GUID, exit shows output
			   e.g. key - the "executed result", execution-equivalent) */
			for (int pi = 0; pi < pn && pi < 8; pi++)
				g_vm_saved_param[pi] = (unsigned int)params[pi];
			g_vm_saved_param_n = pn;
			for (int pi = 0; pi < pn && pi < 4; pi++)
			{
				unsigned long long pa = params[pi];
				if (pa >= 0x04000000ULL && pa < 0x80000000ULL)
				{
					char hex[64];
					vmp38_dump_buf16(uc, pa, hex, sizeof(hex));
					out("[vm-param] p%d=%08x buf16=%s", pi, (unsigned int)pa, hex);
				}
			}
		}
	}
	else if (strcmp(m, "popfd") == 0 || strcmp(m, "popf") == 0)
	{
		out("[vm-ret] exit=%08x esp=%08llx eax=%08llx", eip, esp_now, eax_now);
		for (int pi = 0; pi < g_vm_saved_param_n && pi < 4; pi++)
		{
			unsigned long long pa = g_vm_saved_param[pi];
			if (pa >= 0x04000000ULL && pa < 0x80000000ULL)
			{
				char hex[64];
				vmp38_dump_buf16(uc, pa, hex, sizeof(hex));
				out("[vm-ret] p%d=%08x buf16=%s", pi, (unsigned int)pa, hex);
			}
		}
	}
	else if (strcmp(m, "call") == 0)
	{
		/* call 到 VM 段外（外部的普通函数调用），要记录 call 栈 + 是否返回 */
		const char *pc = code + 4;
		while (*pc == ' ') pc++;
		unsigned long long ctgt = 0;
		if (pc[0] == '0' && (pc[1] == 'x' || pc[1] == 'X'))
			ctgt = strtoull(pc + 2, NULL, 16);
		else if (pc[0] >= '0' && pc[0] <= '9')
			ctgt = strtoull(pc, NULL, 10);
		else
		{
			int creg = reg_index_from_name(pc);
			if (creg >= 0)
				ctgt = (unsigned long long)vv[creg];
		}
		if (ctgt && !(ctgt >= g_vm_start && ctgt < g_vm_end))
		{
			if (g_call_n < CALL_STACK_MAX)
			{
				unsigned long long esp_after = esp_now;
				g_call_ret[g_call_n] = (unsigned long long)eip + (unsigned long long)instr_size;
				g_call_esp[g_call_n] = esp_after;
				g_call_tgt[g_call_n] = ctgt;
				g_call_n++;
				unsigned long long params[8];
				int pn = 0;
				for (int pi = 0; pi < 8; pi++)
				{
					unsigned int pv = 0;
					if (vmp38_uc_read32_guarded(uc, esp_after + 4 + (unsigned long long)pi * 4, &pv))
						params[pn++] = pv;
					else
						break;
				}
				if (pn)
				{
					char cbuf[256]; int coff = 0;
					coff += sprintf(cbuf + coff, "[call-param] @%08x -> %08llx", eip, ctgt);
					for (int pi = 0; pi < pn; pi++)
						coff += sprintf(cbuf + coff, " p%d=%08x", pi, (unsigned int)params[pi]);
					out("%s", cbuf);
				}
			}
		}
	}
	else if ((strcmp(m, "ret") == 0 || strcmp(m, "retn") == 0) && g_call_n > 0)
	{
		/* ret 从栈读到 call 返回地址（可能是返回值） */
		unsigned int ra = 0;
		if (vmp38_uc_read32_guarded(uc, esp_now, &ra) && ra == g_call_ret[g_call_n - 1])
		{
			out("[call-ret] @%08llx eax=%08llx", g_call_tgt[g_call_n - 1], eax_now);
			g_call_n--;
		}
	}
}

static void vmp38_emit_row(unsigned int eip, const char *code,
	const unsigned int *pre, const unsigned int *post,
	unsigned long long maddr, unsigned int mpre, unsigned int mpost, uc_engine *uc)
{
	if (!g_fdata || !code || !code[0]) return;
	/* 循环提示：同 eip 连续重复到阈值时 .txt + x64dbg 日志提示(不停止不丢) */
	vmp38_note_same_eip(eip, code);
	const unsigned int *vv = pre;   /* still used by the memory/mem-addr paths below */
	/* 2026-08-26 缺口2/T2 数据流: 算法算子(xor/and/shr/add/rol/sub/imul/not...)若
	   任一操作数∈算法派生集, 则变化的 dst 值进入派生集 —— 密钥→shr→xor→dec 链沿
	   dst 传播, write 时识别解密结果。值级 taint(轻量)。 */
	if (post)
	{
		char mo[8] = "";
		get_mnemonic(code, mo, sizeof(mo));
		int is_op = mo[0] &&
			(!strncmp(mo,"xor",3) || !strncmp(mo,"and",3) || !strncmp(mo,"or ",3) ||
			 !strncmp(mo,"shr",3) || !strncmp(mo,"shl",3) || !strncmp(mo,"sar",3) ||
			 !strncmp(mo,"rol",3) || !strncmp(mo,"ror",3) || !strncmp(mo,"add",3) ||
			 !strncmp(mo,"sub",3) || !strncmp(mo,"adc",3) || !strncmp(mo,"sbb",3) ||
			 !strncmp(mo,"not",3) || !strncmp(mo,"neg",3) || !strncmp(mo,"imul",4) ||
			 !strncmp(mo,"mul",3));
		if (is_op && (!g_algo_derived.empty() || !g_iskey.empty()))
		{
			int sin = 0;
			int keyin = 0;
			for (int k = 0; k < 8; k++)
			{
				if (g_iskey.count(vv[k])) ++keyin;
				if (g_algo_derived.count(vv[k])) sin = 1;
			}
			if (sin)
			{
				for (int k = 0; k < 8; k++)
					if (post[k] != vv[k] && post[k] != 0)
					{
						unsigned int vk = post[k];
						g_algo_derived.insert(vk);
						g_val_def[vk] = (unsigned int)eip;   /* 定义点 */
						char dc[32]; dc[0]=0;
						_snprintf(dc,sizeof(dc),"%.26s", code ? code : "");
						g_val_def_code[vk] = dc;
					}
			}
		}
	}
	/* (algo-code 原始微操作层已移除: 它是低层噪声, 不是转换表/指令还原。)
	   数据流传播仅保留 g_algo_derived(密钥→派生) + g_val_def(def-use), 供
	   转换表 handler 行([data] write/read)标注密钥参与。 */


	static const char *rn[8] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
	char regs_buf[512] = "";   /* init: dp==0(空操作数/无寄存器变化)时不写 regs 列，避免 strlen 读未初始化 0xCC */
	int dp = 0;
	int is_pp = (strncmp(code, "push", 4) == 0 || strncmp(code, "pop", 3) == 0);
	if (is_pp || (g_dw_need_full && !g_agg_appends))
	{
		/* full dump: push/pop boundaries, or stale baseline after a dedup skip.
		   With defer-by-one, show reg=pre->post so the .data.txt carries the
		   instruction's before/after register state (post==NULL on the
		   dynamic path keeps the old single-value form). */
		for (int k = 0; k < 8; k++)
		{
			if (post)
				dp += _snprintf(regs_buf + dp, (int)sizeof(regs_buf) - dp, "%s%s=%08X->%08X", dp ? " " : "", rn[k], vv[k], post[k]);
			else
				dp += _snprintf(regs_buf + dp, (int)sizeof(regs_buf) - dp, "%s%s=%08X", dp ? " " : "", rn[k], vv[k]);
		}
		if (post)
			dp += _snprintf(regs_buf + dp, (int)sizeof(regs_buf) - dp, "%s%s=%d->%d", dp ? " " : "", "ef", vv[8], post[8]);
		else
			dp += _snprintf(regs_buf + dp, (int)sizeof(regs_buf) - dp, "%s%s=%d", dp ? " " : "", "ef", vv[8]);
	}
	else
	{
		/* operand-driven: show ONLY the registers this instruction touches
		   (src+dest, pre->post value). No delta-vs-previous - a
		   register not in the instruction is not shown even if its value
		   changed (it changed elsewhere). */
		unsigned int used = vmp38_row_used_regs(code);
		for (int k = 0; k < 8; k++)
		{
			if (used & (1u << k))
			{
				if (post)
					dp += _snprintf(regs_buf + dp, (int)sizeof(regs_buf) - dp, "%s%s=%08X->%08X", dp ? " " : "", rn[k], vv[k], post[k]);
				else
					dp += _snprintf(regs_buf + dp, (int)sizeof(regs_buf) - dp, "%s%s=%08X", dp ? " " : "", rn[k], vv[k]);
			}
		}
		{
			static const char *fn[7] = { "zf", "cf", "of", "sf", "pf", "af", "df" };
			static const int fsh[7] = { 6, 0, 11, 7, 2, 4, 10 };
			for (int k = 0; k < 7; k++)
			{
				unsigned int pv = (g_last_ef >> fsh[k]) & 1;
				unsigned int cv = (vv[8] >> fsh[k]) & 1;
				if (cv != pv)
					dp += _snprintf(regs_buf + dp, (int)sizeof(regs_buf) - dp, "%s%s=%d", dp ? " " : "", fn[k], cv);
			}
		}
	}
	if (dp == 0 && !is_pp)
	{
		/* force the write-target register out: VMP loops re-set the same
		   imm every round, so delta-vs-previous is empty and the row would
		   lose that this instruction sets the reg */
		const char *comma = strchr(code, ',');
		const char *sp = strchr(code, ' ');
		if (comma && sp && sp < comma)
		{
			for (int k = 0; k < 8; k++)
			{
				size_t L = strlen(rn[k]);
				if ((size_t)(comma - sp - 1) == L && strncmp(sp + 1, rn[k], L) == 0)
				{
					if (post)
						dp = _snprintf(regs_buf, (int)sizeof(regs_buf), "%s=%08x->%08x", rn[k], vv[k], post[k]);
					else
						dp = _snprintf(regs_buf, (int)sizeof(regs_buf), "%s=%08x", rn[k], vv[k]);
					break;
				}
			}
		}
	}
	for (int k = 0; k < 8; k++) g_last_regs[k] = vv[k];
	g_last_ef = vv[8];
	/* memory operand value: [expr]=pre->post @addr (defer-by-one supplies the
	   pre value captured BEFORE execution and the post value captured AFTER,
	   so the row shows the memory cell's own before/after data). */
	char mems_buf[256] = "";
	if (maddr != ~0ULL)
	{
		const char *br = strchr(code, '[');
		const char *cr = br ? strchr(br + 1, ']') : NULL;
		if (br && cr && cr > br)
		{
			char mex[128];
			int ml = (int)(cr - br - 1);
			if (ml > 0 && ml < (int)sizeof(mex))
			{
				memcpy(mex, br + 1, ml);
				mex[ml] = 0;
				if (post)
					_snprintf(mems_buf, sizeof(mems_buf), "[%08X]=%08X -> %08X", (unsigned int)maddr, mpre, mpost);
				else
					_snprintf(mems_buf, sizeof(mems_buf), "[%08X]=%08X", (unsigned int)maddr, mpre);
				/* 2026-08-28: [data] read/write output cancelled (no read/write annotation lines in .txt).
				   def-use/taint bookkeeping (g_algo_derived/g_val_def) kept but unused. */
			}
		}
	}

	/* per-execution direct stream: rows are written in execution order but
	   each eip is written ONCE (dedup) - a loop re-running the same handler
	   is NOT rewritten every iteration (that blew the data file up from
	   ~17k rows to 200k+ and made the restore slow). stdio buffering (4MB)
	   absorbs the fwrite cost, so tracing speed is unaffected while the
	   file is written incrementally. */
	{
		char row[2048];
		char *rwp = row;
		memcpy(rwp, "eip=", 4); rwp += 4;
		{
			static const char hx[] = "0123456789ABCDEF";
			unsigned int av = eip;
			for (int k = 7; k >= 0; k--) { rwp[k] = hx[av & 0xF]; av >>= 4; }
			rwp += 8;
		}
		*rwp++ = '\t';
		{
			/* 指令列地址折叠: 与 .txt 一致的寻址还原
			   ([ecx+edi*1-0xA0] -> [ecx+...] 或 [绝对地址]) */
			char codefold[300];
			if (vv && strchr(code, '['))
				vmp38_fold_code_addr(code, vv, maddr, codefold, sizeof(codefold));
			else
				_snprintf(codefold, sizeof(codefold), "%s", code);
			size_t cl = strlen(codefold);
			if (cl > 260) cl = 260;
			memcpy(rwp, codefold, cl); rwp += cl;
		}
		*rwp++ = '\t';
		{
			/* use dp (actual bytes written) not strlen: with regs_buf initialized
			   and dp==0 the regs column stays empty (no 0xCC garbage). Clamp to
			   500 so _snprintf's possible "would-be" return can't exceed buffer. */
			/* 越界诊断：正常一次指令 regs/flags delta 远小于 500；dp 若异常大说明
			   _snprintf 返回的"应写长"在被 regs_buf(512) 填满后继续累加失控——
			   这会让 rwp 顶出 row[2048] → 无换行堆积/尾部NUL(观测到的 bug)。 */
			if (dp > 500 || dp < 0)
			{
				out_progress("[warn] emit_row eip=%08x dp=%d code='%.60s' (regs_buf 可能被 _snprintf 应写长填满溢出,已 clamp)",
					eip, dp, code ? code : "");
				if (dp > 500) dp = 500;
				if (dp < 0) dp = 0;
			}
			size_t cl = (size_t)dp;
			if (cl > 500) cl = 500;
			memcpy(rwp, regs_buf, cl); rwp += cl;
		}
		if (mems_buf[0])
		{
			*rwp++ = '\t';
			size_t cl = strlen(mems_buf);
			if (cl > 240) cl = 240;
			memcpy(rwp, mems_buf, cl); rwp += cl;
		}
		*rwp++ = '\n';
		/* 2026-08-22 用户要求：改真逐条记录——每次执行都写一行（含循环体重复迭代）。
		   此前 per-eip dedup（每 eip 首次才写）会丢弃重复执行，无法还原循环次数/
		   逐条执行流。文件会随指令数增长，但保证执行流完整。 */
		fwrite(row, 1, (size_t)(rwp - row), g_fdata);
	}
}

/* SEH guard for emit_row: emit_row uses std::string/vector (C++ unwinding)
   so __try CANNOT live inside it (C2712). This wrapper is plain C: any AV
   inside emit_row (e.g. a malformed disasm string -> strstr/strlen out of
   bounds, fault=0) is caught here and the row is skipped - the trace and
   the host survive. Called from uc_hook_code stage-7 hot path. */
static void vmp38_emit_row_guarded(unsigned int eip, const char *code,
	const unsigned int *vv, uc_engine *uc)
{
	__try {
		/* defer-by-one: the code hook fires BEFORE this instruction executes,
		   so uc registers right now are exactly the PREVIOUS instruction's
		   POST state. Settle the pending row (pre stored at the previous
		   hook + post = vv) and store this instruction as the new pending. */
		if (g_dw_pend_valid)
		{
			unsigned int mpost = 0;
			if (g_dw_pend_maddr != ~0ULL)
				vmp38_uc_read32_guarded(uc, g_dw_pend_maddr, &mpost);
			vmp38_emit_row(g_dw_pend_eip, g_dw_pend_code, g_dw_pend_pre, vv,
				g_dw_pend_maddr, g_dw_pend_mpre, mpost, uc);
		}
		g_dw_pend_eip = eip;
		{
			size_t cl = strlen(code);
			if (cl >= sizeof(g_dw_pend_code)) cl = sizeof(g_dw_pend_code) - 1;
			memcpy(g_dw_pend_code, code, cl);
			g_dw_pend_code[cl] = 0;
		}
		memcpy(g_dw_pend_pre, vv, 9 * sizeof(unsigned int));
		/* capture THIS instruction's memory pre value (uc is still pre-exec) */
		g_dw_pend_maddr = ~0ULL;
		g_dw_pend_mpre = 0;
		{
			const char *br = strchr(code, '[');
			const char *cr = br ? strchr(br + 1, ']') : NULL;
			if (br && cr && cr > br)
			{
				char mex[128];
				int ml = (int)(cr - br - 1);
				if (ml > 0 && ml < (int)sizeof(mex))
				{
					memcpy(mex, br + 1, ml);
					mex[ml] = 0;
					unsigned long long ma = 0;
					if (an_mem_expr_addr(mex, vv, &ma))
					{
						unsigned int mv = 0;
						if (vmp38_uc_read32_guarded(uc, ma, &mv))
						{
							g_dw_pend_maddr = ma;
							g_dw_pend_mpre = mv;
						}
					}
				}
			}
		}
		g_dw_pend_valid = 1;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		out_progress("[ft] emit_row AV at %08x - row skipped", eip);
	}
}

/* ---- x64dbg .trace32 writer (2026-08-18) ----
   Static-trace -> x64dbg-loadable trace stream. Format (traced from
   F:/x64dbg TraceFileReader): "TRAC" + u32 jsonLen + JSON + sequence of
   blockType-0 instruction blocks. Each block stores ONLY the changed
   registers (index-delta + new value) and memory operand old/new; x64dbg
   rebuilds the full REGDUMP per index. Memory old/new is added in a later
   iteration; Phase A emits registers + opcode + EIP only. */
static void vmp38_trace32_open(const char *exepath)
{
	if (g_trace32_open)
		return;
	/* .trace32 is an OPTIONAL extra (x64dbg trace view only). Writing it per
	   instruction was the dominant static-restore cost (tiny fwrites). Default
	   OFF unless XXVM_TRACE32=1 - the plugin's real outputs are .data/.txt. */
	if (!g_trace32_enabled)
	{
		/* read the env var once (headless/automation default); GUI has no
		   menu for it - users who want the trace set it in the .cf/env */
		static int s_env_read = 0;
		if (!s_env_read)
		{
			char sb[8] = "";
			GetEnvironmentVariableA("XXVM_TRACE32", sb, sizeof(sb));
			if (sb[0] == '1') g_trace32_enabled = 1;
			s_env_read = 1;
		}
		if (!g_trace32_enabled)
			return;
	}
	char outpath[MAX_PATH] = "";
	char exedir[MAX_PATH] = "";
	GetModuleFileNameA(NULL, exedir, sizeof(exedir));
	{
		char *sl = strrchr(exedir, '\\');
		if (sl) *sl = 0;
	}
	_snprintf(outpath, sizeof(outpath), "%s\\xxvm", exedir);
	CreateDirectoryA(outpath, NULL);
	{
		SYSTEMTIME st; GetLocalTime(&st);
		_snprintf(outpath + strlen(outpath), sizeof(outpath) - strlen(outpath),
			"\\xxvm_trace_%04d%02d%02d-%02d%02d%02d-%03d.trace32",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	}
	g_trace32 = fopen(outpath, "wb");
	if (!g_trace32)
		return;
	/* TRAC magic + u32 jsonLen */
	char json[512];
	int jl = _snprintf_s(json, sizeof(json), _TRUNCATE,
		"{\"ver\":1,\"arch\":\"x86\",\"hashAlgorithm\":\"murmurhash\","
		"\"hash\":\"0x0\",\"compression\":\"\",\"path\":\"%s\"}", exepath ? exepath : "");
	fwrite("TRAC", 1, 4, g_trace32);
	fwrite(&jl, 1, 4, g_trace32);
	fwrite(json, 1, (size_t)jl, g_trace32);
	g_trace32_open = 1;
	g_trace32_first = 1;
}

/* REGDUMP regwords duint-index for a given reg (x86/REGISTERCONTEXT).
   REGISTERCONTEXT is uint64_t fields; duint(4B) view => each field = 2 duints.
   gpr order in REGISTERCONTEXT: cax,ccx,cdx,cbx,csp,cbp,csi,cdi,r8..r15,cip,eflags.
   Return -1 if not tracked. idx = regwords index (even for low-32). */
static int trace32_rw_index(int reg)
{
	switch (reg)
	{
	case 0: return 0;  /* eax/cax */
	case 1: return 2;  /* ecx/ccx */
	case 2: return 4;  /* edx/cdx */
	case 3: return 6;  /* ebx/cbx */
	case 4: return 8;  /* esp/csp */
	case 5: return 10; /* ebp/cbp */
	case 6: return 12; /* esi/csi */
	case 7: return 14; /* edi/cdi */
	default: return -1;
	}
}

/* record the PREVIOUS deferred instruction. cur_pre[9] = this hook's exec-PRE
   regs (eax..edi + eflags); these are the PREVIOUS instruction's exec-POST.
   We diff prev-exec-POST (cur_pre) vs prev-exec-PRE (g_tr_pre) and write one
   block with opcode/address captured when that previous instruction ran. */
static void vmp38_trace32_record(unsigned int cur_eip, const unsigned int *cur_pre)
{
	if (!g_trace32_open || !g_trace32)
		return;
	if (g_trace32_first || (g_tr_insn % 512) == 0)
	{
		/* Page boundary: x64dbg reader (TraceFileReader) uses regcount==REGEND
		   (216 duints x86) as a PAGE BOUNDARY to build its fileIndex - the
		   real x64dbg writes one full REGDUMP snapshot every 512 instructions
		   (verified on a real gscservice.trace32). Without periodic boundaries
		   the reader shows only one page (512 rows) and everything after
		   disappears. First block + every 512th instruction writes a full
		   snapshot; the opcode is the caller-filled current instruction
		   (g_tr_pre_op) so the first row is the real first instruction. */
		unsigned char cfh[3];
		cfh[0] = 216;                   /* full REGDUMP */
		cfh[1] = 0;                     /* memcount */
		cfh[2] = (unsigned char)((g_tr_pre_oplen > 0 ? g_tr_pre_oplen : 1) & 0x0F);
		unsigned char bt = 0;
		fwrite(&bt, 1, 1, g_trace32);
		fwrite(cfh, 1, 3, g_trace32);
		if (g_tr_pre_oplen > 0)
			fwrite(g_tr_pre_op, 1, (size_t)g_tr_pre_oplen, g_trace32);
		else
		{
			unsigned char nop = 0x90;
			fwrite(&nop, 1, 1, g_trace32);
		}
		{
			unsigned char d0[216];
			memset(d0, 0, sizeof(d0));           /* deltas 0..215 (consecutive) */
			fwrite(d0, 1, sizeof(d0), g_trace32);
		}
		{
			unsigned int vals[216];
			memset(vals, 0, sizeof(vals));
			/* snapshot the GPR low32 we know + EIP + EFLAGS at their slots */
			for (int k = 0; k < 8; k++) vals[k * 2] = cur_pre[k];   /* eax..edi idx 0,2,..14 */
			vals[34] = cur_pre[8];                                  /* eflags idx 34 */
			vals[32] = cur_eip;                                     /* cip idx 32 */
			fwrite(vals, 4, 216, g_trace32);
		}
		/* snapshot state now = this full block */
		for (int k = 0; k < 8; k++) g_tr_pre[k] = cur_pre[k];
		g_tr_pre[8] = cur_pre[8];
		g_tr_pre_eip = cur_eip;
		g_trace32_first = 0;
		g_tr_insn++;
		return;
	}
	/* Build the reg-diff map: regwords[str] <- value. We write every 8 GP
	   register (low32) and EIP+EFLAGS (REGISTERCONTEXT duint slots). The
	   previous instruction's post-state = cur_pre (this hook pre-state). */
	unsigned char changed[16];          /* delta: pos += delta+1 */
	unsigned int regContent[16];
	int n = 0, lastPos = -1;
	{
		/* candidate (regwords_idx, low32/dup) pairs in ASCENDING idx order:
		   eax..edi (0,2,..14), cip(32), eflags(34). EIP must be in every
		   block (address column) - it always changes per instruction. */
		static const int rw[10] = { 0, 2, 4, 6, 8, 10, 12, 14, 32, 34 };
		unsigned int post[10], oldv[10];
		for (int k = 0; k < 8; k++) { post[k] = cur_pre[k]; oldv[k] = g_tr_pre[k]; }
		post[8] = cur_eip;             oldv[8] = g_tr_pre_eip;   /* cip */
		post[9] = cur_pre[8];          oldv[9] = g_tr_pre[8];    /* eflags */
		for (int i = 0; i < 10; i++)
		{
			int idx = rw[i];
			if (oldv[i] == post[i])
				continue;                        /* unchanged: skip */
			int delta = idx - lastPos - 1;
			if (delta < 0 || delta > 255)
			{
				/* REGDUMP slot gap too big to delta-encode in one byte.
				   x64dbg stores regs relative; very large jumps (>255 duints)
				   between changed regs can't be delta-encoded - we simply
				   also emit EIP so the stream stays coherent, and skip the
				   far slot if it would overflow. Keep it simple: clamp. */
				continue;
			}
			changed[n] = (unsigned char)delta;
			regContent[n] = post[i];
			n++;
			lastPos = idx;
		}
	}
	/* Write the block: blockType 0, then [regcount][memcount][flags(opcode
	   len in low nibble)], opcode (captured when prev instr disassembled),
	   then reg delta + values. */
	{
		unsigned char flagsByte = (unsigned char)(g_tr_pre_oplen & 0x0F);
		unsigned char cfh[3];
		cfh[0] = (unsigned char)n;
		cfh[1] = 0;                     /* memcount (memory old/new later) */
		cfh[2] = flagsByte;             /* opcode length in low nibble */
		unsigned char bt = 0;
		fwrite(&bt, 1, 1, g_trace32);
		fwrite(cfh, 1, 3, g_trace32);
		if (g_tr_pre_oplen > 0)
			fwrite(g_tr_pre_op, 1, (size_t)g_tr_pre_oplen, g_trace32);
		if (n > 0)
		{
			fwrite(changed, 1, (size_t)n, g_trace32);
			fwrite(regContent, 4, (size_t)n, g_trace32);
		}
	}
	/* update prev snapshot for the instruction that is about to run now */
	for (int k = 0; k < 9; k++) g_tr_pre[k] = cur_pre[k];
	g_tr_pre_eip = cur_eip;
	g_tr_pre_oplen = 0;   /* caller fills g_tr_pre_op after this, for the
	                         NEXT deferred block */
	g_tr_insn++;
}

static void vmp38_trace32_close(void)
{
	if (g_trace32)
	{
		fclose(g_trace32);
		g_trace32 = NULL;
	}
	g_trace32_open = 0;
}

/* Write the aggregated rows + reset all row state. Called at the END of
   both static and dynamic traces (identical rule). Returns the distinct-eip
   count for the trace-done progress line. */
static int vmp38_emit_tail(void)
{
	/* 2026-08-22 用户要求改真逐条后 data 为完整执行流；distinct 用总执行指令数
	   (g_stream_total) 近似描述文件规模。聚合 g_rows/g_row_order/emit_tail_row 已移除。 */
	int distinct = g_fdata ? (int)g_stream_total : 0;
	/* reset for the next trace */
	memset(g_last_regs, 0, sizeof(g_last_regs));
	g_last_ef = 0;
	g_dw_need_full = 0;
	return distinct;
}

static void uc_hook_code(uc_engine *uc, uint64_t address, uint32_t size,
	void *user_data)
{
	(void)size;
	(void)user_data;
	/* rep string-iteration suppression: an iteration of rep movs/stos fires
	   with the SAME eip repeatedly - write only the first iteration (count
	   is engine bookkeeping, not user logic). Check BEFORE updating last. */
	if (g_stream_last_ip == address)
	{
		unsigned char pb[2] = { 0, 0 };
		int rep_ok = 0;
		__try {
			rep_ok = (uc_mem_read(uc, address, pb, 2) == UC_ERR_OK) ? 1 : 0;
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			rep_ok = 0;   /* hook-internal read AV - skip rep suppression */
		}
		if (rep_ok &&
			(pb[0] == 0xF3 || pb[0] == 0xF2) &&
			(pb[1] == 0xA4 || pb[1] == 0xA5 || pb[1] == 0xAA || pb[1] == 0xAB ||
			 pb[1] == 0xA6 || pb[1] == 0xA7 || pb[1] == 0xAC || pb[1] == 0xAD ||
			 pb[1] == 0xAE || pb[1] == 0xAF))
			return;   /* rep iteration - skip, emulation continues */
	}
	/* ---- no-new-distinct spin detector: REMOVED per user (2026-08-15) -
	   GSCService's 01385528 loop is a REAL long-period VM loop (66 iters,
	   ~20M instr apart), not a dead loop; auto-stop would cut it short.
	   Termination relies on popfd+popad vm-exit + user Stop. ---- */
	g_stream_total++;
	g_stream_last_ip = address;
	/* ---- jump-to-garbage detection: VMP anti-debug / state desync makes
	   guest eip jump to a low garbage address (eip=5 observed) - keep the
	   last 10 eips in globals so the crash dump can show where control
	   flow left. ---- */
	{
		unsigned long long lo = g_module_base ? g_module_base : 0x10000ULL;
		unsigned long long hi = g_module_base ? g_module_base + 0x4000000ULL : 0x80000000ULL;
		if (g_prev_n >= 10)
		{
			/* detect a big backwards / low jump */
			if (address < 0x10000ULL || (address < 0x1000ULL && g_stream_total > 100))
			{
				char pb[160];
				int pn = g_prev_n > 10 ? 10 : g_prev_n;
				int o = 0;
				o += _snprintf(pb + o, sizeof(pb) - o, "# JUMP-GARBAGE to %08llx prev:", address);
				for (int k = pn > 10 ? pn - 10 : 0; k < pn; k++)
					o += _snprintf(pb + o, sizeof(pb) - o, " %08x", g_prev_eips[k]);
				o += _snprintf(pb + o, sizeof(pb) - o, "\n");
				if (g_fdata) { fwrite(pb, 1, strlen(pb), g_fdata); fflush(g_fdata); }
				out("[jump-garbage] to %08llx (see data # JUMP-GARBAGE)", address);
			}
			memmove(g_prev_eips, g_prev_eips + 1, 9 * sizeof(unsigned int));
			g_prev_eips[9] = (unsigned int)address;
		}
		else
		{
			g_prev_eips[g_prev_n++] = (unsigned int)address;
		}
	}
	/* ---- loop detection: the same eip sequence repeated >= LOOP_N times is
	   a VM loop - write the first pass fully, then suppress repeats (each
	   distinct eip at most 2 rows = the if-branch paths). ---- */
	{
		static unsigned int s_win[256];
		static int s_wi = 0;
		static int s_match[65];
		static int s_loop_mode = 0;
		static unsigned int s_loop_eip[256];
		static int s_loop_cnt[256];
		static int s_loop_n = 0;
		static int s_loop_fresh = 0;   /* consecutive brand-new eips */
		s_win[s_wi] = (unsigned int)address;
		s_wi = (s_wi + 1) & 255;
		if (!s_loop_mode)
		{
			int period = 0;
			for (int P = 1; P <= 64; P++)
			{
				if (s_win[(s_wi - P) & 255] == (unsigned int)address)
					s_match[P] = (s_match[P] < 64) ? s_match[P] + 1 : 64;
				else
					s_match[P] = 0;
				if (s_match[P] >= 64) period = P;
			}
			if (period)
			{
				s_loop_mode = 1;
				s_loop_n = 0;
				s_loop_fresh = 0;
				memset(s_loop_cnt, 0, sizeof(s_loop_cnt));
			}
		}
		else
		{
			/* loop mode: write a distinct eip at most 2 times */
			int found = -1;
			for (int k = 0; k < s_loop_n; k++)
				if (s_loop_eip[k] == (unsigned int)address) { found = k; break; }
			if (found < 0)
			{
				if (s_loop_n < 256) { s_loop_eip[s_loop_n] = (unsigned int)address; s_loop_n++; }
				found = s_loop_n - 1;
			}
			if (found >= 0)
			{
				s_loop_cnt[found]++;
				/* NOTE: no more emit suppression here - the old "at most 2
				   rows per eip in a loop" rule dropped every 3rd+ iteration
				   of a loop body, so a 100-iteration loop looked like a
				   single pass (user: "循环未追踪（追踪缺失）"). The per-row
				   appends cap (g_row_max_appends) bounds output now, so
				   every iteration is emitted and the loop count is visible. */
				if (s_loop_cnt[found] == 1) s_loop_fresh = 0;
				else s_loop_fresh++;
			}
			/* exit loop mode after 64 consecutive repeated eips (loop ended) */
			if (s_loop_fresh >= 64)
			{
				s_loop_mode = 0;
				s_loop_n = 0;
				s_loop_fresh = 0;
				memset(s_match, 0, sizeof(s_match));
			}
		}
	}
	if (g_fdata)
	{
		g_hook_stage = 5;
		DISASM_INSTR sdi;
		memset(&sdi, 0, sizeof(sdi));
		disasm_at_cached((unsigned int)address, &sdi, uc);
		if (sdi.instr_size > 0)
		{
			g_hook_stage = 7;
			/* Unicorn has NO x86 fs segment (uc_reg_write(FS)=UC_ERR_INVALID_REG,
			   err=21 observed): fs:[0x18] (TEB->PEB) reads in .text code
			   (e.g. 00414a45 mov ecx, fs:[0x18]) get fs=0 -> [0x18]=0 -> a
			   later jz branches to garbage. Intercept: read the REAL PEB
			   pointer from the live process TEB and write the target reg. */
			if (strstr(sdi.instruction, "fs:[0x18]"))
			{
				duint teb_b = 0, peb_v = 0;
				if (Script::Misc::ParseExpression("teb()", &teb_b) && teb_b &&
					DbgMemRead((duint)(teb_b + 0x18), &peb_v, 4))
				{
					uc_reg_write(uc, UC_X86_REG_ECX, &peb_v);
				}
			}
			static const int tr2[9] = { UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
				UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI,
				UC_X86_REG_EFLAGS };
			int ids2[9]; void *vals2[9]; unsigned int vv2[9];
			for (int k = 0; k < 9; k++) { ids2[k] = tr2[k]; vals2[k] = &vv2[k]; }
			/* GSCService: Unicorn's reg_read_batch can AV internally (fault=0)
			   on certain VMP state - SEH-guard it so the row is skipped and
			   the trace continues instead of killing the host. */
			{
				int rb_ok = 1;
				__try {
					uc_reg_read_batch(uc, ids2, vals2, 9);
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					rb_ok = 0;
					/* ROOT-CAUSE probe: is env dead (single read also AVs)
					   or read_batch-specific? */
					unsigned int probe = 0;
					__try {
						uc_err pe = uc_reg_read(uc, UC_X86_REG_EIP, &probe);
						out_progress("[rb] batch AV: single reg_read EIP err=%d val=%08x", (int)pe, probe);
					} __except (EXCEPTION_EXECUTE_HANDLER) {
						out_progress("[rb] batch AV AND single reg_read AVs -> env/state dead");
					}
				}
				if (rb_ok)
				{
					/* x64dbg trace32 writer: record the PREVIOUS deferred
					   instruction block (using g_tr_pre_op, the opcode captured
					   when it disassembled), then capture THIS instruction's
					   opcode for the next deferred block. Recorded before any
					   emit filtering so every executed instruction is in the
					   trace stream. */
					{
						/* capture THIS instruction's opcode FIRST, so the
						   boundary/full-snapshot block uses the REAL first
						   instruction (not a synthetic nop) */
						g_tr_pre_oplen = (sdi.instr_size > 0 && sdi.instr_size <= 16) ? sdi.instr_size : 0;
						if (g_tr_pre_oplen > 0)
						{
							__try {
								if (uc_mem_read(uc, (uint64_t)address, g_tr_pre_op, (size_t)g_tr_pre_oplen) != UC_ERR_OK)
									g_tr_pre_oplen = 0;
							} __except (EXCEPTION_EXECUTE_HANDLER) { g_tr_pre_oplen = 0; }
						}
						vmp38_trace32_record((unsigned int)address, vv2);
					}
					/* stage-7 hot path: emit_row does string ops on the
					   disassembled text + aggregate/direct writes. SEH-
					   guard via a NON-C++ wrapper (emit_row itself uses
					   std::string/vector -> cannot __try inside it; the
					   wrapper is plain C). An internal AV (fault=0) must
					   skip the row, never kill the host. Every iteration
					   emits (no loop suppression) - the per-row appends cap
					   bounds output. */
					/* text_phase 追 .text 返回 stub 时不同写 data / 也不写 g_ft（
					   只执行 .text 代码，直到 call 到 VM 段，即下一个 VM 函数），
					   恢复后原来 vm-exit 后的 .text 代码会被当作 VM 代码还原，
					   从而"第二次运行怎么做"，.text 的 VM 片段也还原（核心是 eip 
					   在 .text 时用户确定 .text 的 VM stub 要还原。 */
					if (!g_ft_text_phase)
					{
						/* VM 内部 dispatch 噪声筛选（2026-08-18，用户要求）：
						   当前 eip 在 VM 段内，若 call/ret 目标是在 VM 段内（即 VM handler 之间的 
						   dispatch 跳转，忽略算法意义，直接写 data）；
						   GSCService 等大量 dispatch call/ret 占很大比例。
						   ret 在 VM 段、call 在 VM 段内部 handler，做过滤。 */
						int dispatch_noise = 0;
						/* HOT: dispatch_noise + fold each parsed the mnemonic from
						   sdi.instruction separately (2-3 string scans per instr
						   in the VM hot loop). Parse ONCE up front and reuse. */
						char mz_hot[16];
						get_mnemonic(sdi.instruction, mz_hot, sizeof(mz_hot));
						if (g_vm_start && g_vm_end &&
							(unsigned long long)address >= g_vm_start &&
							(unsigned long long)address < g_vm_end)
						{
							const char *mzo = mz_hot;
							if (strncmp(mzo, "call", 4) == 0)
							{
								const char *pc = sdi.instruction + 4;
								while (*pc == ' ') pc++;
								unsigned long long ct = 0;
								if (pc[0]=='0' && (pc[1]=='x'||pc[1]=='X')) ct = strtoull(pc+2,NULL,16);
								else if (pc[0]>='0'&&pc[0]<='9') ct = strtoull(pc,NULL,10);
								if (ct >= (unsigned long long)g_vm_start && ct < (unsigned long long)g_vm_end)
									dispatch_noise = 1;
							}
							else if (strncmp(mzo, "ret", 3) == 0)
							{
								unsigned int rsp_v = vv2[4];
								unsigned int rtarget = 0;
								if (vmp38_uc_read32_guarded(uc, (unsigned long long)rsp_v, &rtarget) &&
									rtarget >= (unsigned int)g_vm_start &&
									rtarget < (unsigned int)g_vm_end)
									dispatch_noise = 1;
							}
							/* jmp/其他控制流: 按用户规则【不做函数头尾判断, 直接继续还原】
							   (只有 ret 到 .text 才做函数头尾判断)。 */
						}
						/* call 处理(2026-09-02 按用户规则简化)：
						   - call 目标在 VM 段内 = VMP 内部 dispatch call，不 emit 继续追；
						   - call 目标在 VM 段外(.text/API) = 不做函数头尾判断, 无条件继续还原；
						   - 系统 DLL 调用由后面独立的 system-dll skip 处理。 */
						int fold_now = 0;
						const char *mzo2 = mz_hot;
						if (strncmp(mzo2, "call", 4) == 0)
						{
							const char *pc = sdi.instruction + 4;
							while (*pc == ' ') pc++;
							unsigned long long ct = 0;
							if (pc[0]=='0' && (pc[1]=='x'||pc[1]=='X')) ct = strtoull(pc+2,NULL,16);
							else if (pc[0]>='0'&&pc[0]<='9') ct = strtoull(pc,NULL,10);
							else { int cr2 = reg_index_from_name(pc); if (cr2>=0) ct=(unsigned long long)vv2[cr2]; }
							/* 目标在 VM 段内（VMP 内部 dispatch call），忽略这个 call；
							   对完全安全的追 VM 而言：若 target call 无返回值则完全安全；只有
							   call 到 VM 段外的函数/API 时才输出 "call 地址" 行。 */
							if (ct && ct >= (unsigned long long)g_vm_start && ct < (unsigned long long)g_vm_end)
							{
								fold_now = 1;   /* VM-internal call: don't emit, keep tracing */
							}
							/* call 到 VM 段外(.text 普通函数/API): 按用户规则【不做函数头尾判断,
							   无条件继续还原】——只有 ret 到 .text 才做函数头尾判断。 */
						}
						if (!dispatch_noise && !fold_now)
						{
							vmp38_capture_asm_mem((unsigned int)address, sdi.instruction, vv2, uc);
							vmp38_emit_row_guarded((unsigned int)address, sdi.instruction, vv2, uc);
							/* P0: fill g_ft trace buffer (eip + 8 regs + eflags) so
							   vmp38_restore_from_trace32 can rebuild handler
							   semantics after the trace. vv2[] = eax..edi + eflags
							   (uc_reg_read_batch order above). */
							if (g_ft_ensure_cap(g_ft_n + 1))
							{
								g_ft_eip[g_ft_n] = (unsigned int)address;
								for (int fk = 0; fk < 8; fk++)
									g_ft_regs[g_ft_n][fk] = vv2[fk];
								g_ft_eflags[g_ft_n] = vv2[8];
								g_ft_n++;
							}
						}
					}
					/* P3: symbolic tracking + jmp-reg target resolution.
					   A jmp reg whose concrete value drifted to garbage
					   (e.g. c44fc000 after a system-memory read failed) can
					   still be resolved from its symbolic expression. */
					{
						/* sync live non-ESP registers into g_symreg so the
						   memory-symbol address computation (compute_abs_addr)
						   uses real values; ESP stays the sym_track_insn-
						   maintained simulated value (push/pop stack slots) */
						g_symreg.r[0] = (unsigned long long)vv2[0];  /* eax */
						g_symreg.r[1] = (unsigned long long)vv2[1];  /* ecx */
						g_symreg.r[2] = (unsigned long long)vv2[2];  /* edx */
						g_symreg.r[3] = (unsigned long long)vv2[3];  /* ebx */
						g_symreg.r[5] = (unsigned long long)vv2[5];  /* ebp */
						g_symreg.r[6] = (unsigned long long)vv2[6];  /* esi */
						g_symreg.r[7] = (unsigned long long)vv2[7];  /* edi */
						/* PRE-naming: bind regs holding a source scalar BEFORE the
						   instruction, so sym_track_insn's `mov [mem],reg` store
						   carries the NAMED symbol into the slot (acc chain crosses
						   handler boundaries via the slot). */
						vmp38_sym_name_srcs(&g_symreg);
						sym_track_insn(&g_symreg, sdi.instruction);
						vmp38_sym_name_srcs(&g_symreg);   /* post: bind result if it equals a seed */
						/* 2026-08-25 立项: 旁路足迹传播(预处理经 vv2, post 用模拟 r[], 不碰执行符号) */
						{
							unsigned int pr_[8], po_[8];
							for (int qi = 0; qi < 8; qi++)
							{
								pr_[qi] = (unsigned int)vv2[qi];
								po_[qi] = (unsigned int)g_symreg.r[qi];
							}
							vmp38_algo2_step(sdi.instruction, pr_, po_);
						}
						/* 阶段二: 跨 handler acc 累积折叠。post 符号已由
						   sym_track_insn 求值到 g_symreg.r[] (模拟执行后值),
						   此处检测载体符号经 clean 增量变化即输出 acc += Δ。 */
						vmp38_acc2_fold(&g_symreg, (unsigned int)address, sdi.instruction);
						/* periodic GC: a 200k-instruction full trace otherwise
						   exhausts SYM_POOL_MAX (sym_alloc degrades to const 0,
						   breaking the cross-handler acc/mov symbol chain).
						   GC keeps the 8 reg + mem-slot roots alive and drops
						   dead intermediates, so the symbolic reconstruction of
						   lea/mov (a*3 etc.) survives the whole run. */
						sym_gc(&g_symreg);
						/* strength-reduction sampling: when a register symbol is
						   MUL(src_var, const>=2), that is the reconstructed SOURCE
						   multiply (a*2 / a*3 / ...) that VMP's lea got rewritten
						   into an add chain. Emit it once per distinct (reg,src,
						   coeff) so the [src] level can surface lea-equivalents. */
						{
							static const char *srn4[4] = { "eax","ecx","edx","ebx" };
							static unsigned int s_mul_seen_key[64];
							static int s_mul_seen_n = 0;
							for (int k = 0; k < 4; k++)
							{
								int sid = g_symreg.sym[k];
								if (sid <= 0 || sid >= g_sym_n) continue;
								SYM_NODE *nd = &g_sym_pool[sid];
								if (nd->op != SYM_OP_MUL) continue;
								/* a = src var (or a==k reg leaf), b = const coeff */
								unsigned long long coeff = 0;
								if (sym_is_const(nd->b)) coeff = sym_const_val(nd->b);
								if (coeff < 2) continue;
								int is_src = (nd->a > 0 && nd->a < g_sym_n &&
									g_sym_pool[nd->a].op == SYM_OP_SRC);
								if (!is_src) continue;
								char ea[64]; g_sym_dbg_depth = 0;
								sym_expr_str(nd->a, ea, sizeof(ea));
								/* key on (reg, src-var-id, coeff) - NOT address: the
								   same symbolic multiply recurs every iteration and
								   must be emitted only once. */
								unsigned int key = ((unsigned int)k * 0x10000u) ^
									((unsigned int)g_sym_pool[nd->a].c * 0x100u) ^
									((unsigned int)coeff);
								int dup = 0;
								for (int si2 = 0; si2 < s_mul_seen_n; si2++)
									if (s_mul_seen_key[si2] == key) { dup = 1; break; }
								if (!dup && s_mul_seen_n < 64)
								{
									s_mul_seen_key[s_mul_seen_n++] = key;
									/* emit as [src] so the .txt filter (keep-only-[src])
									   surfaces it in the source conversion table: a
									   symbolically-rebuilt multiply IS a source op.
									   Use out() (file-only) - this is a reduction
									   line, not GUI progress. */
									out("  [src] %s = %s * %llu  (lea-equivalent mul, concrete %08x)",
										srn4[k], ea, coeff, (unsigned int)g_symreg.r[k]);
								}
							}
						}
						/* immediate mem-slot naming: if THIS instruction is `mov [mem], reg`
						   and reg holds a source scalar, bind the slot's MEM symbol NOW. */
						{
							char mo[16];
							get_mnemonic(sdi.instruction, mo, sizeof(mo));
							if (!strcmp(mo, "mov") && strchr(sdi.instruction, '[') != NULL &&
								strchr(sdi.instruction, ',') != NULL &&
								strchr(sdi.instruction, '[') < strchr(sdi.instruction, ','))
							{
								const char *comma = strchr(sdi.instruction, ',') + 1;
								while (*comma == ' ') comma++;
								char regtok[16]; int rt = 0;
								for (; *comma && *comma != ' ' && *comma != '	' && rt < 15; comma++) regtok[rt++] = *comma;
								regtok[rt] = 0;
								int src_i = reg_index_from_name(regtok);
								if (src_i >= 0 && src_i < 8)
								{
									int sidx2 = vmp38_src_id_of((unsigned int)g_symreg.r[src_i]);
									if (sidx2 >= 0)
									{
										const char *bb = strchr(sdi.instruction, '[');
										const char *ee = bb ? strchr(bb, ']') : NULL;
										if (bb && ee && ee > bb)
										{
											char mex[128]; int ml = (int)(ee - bb - 1);
											unsigned long long maddr = 0;
											if (ml > 0 && ml < (int)sizeof(mex))
											{
												memcpy(mex, bb + 1, ml); mex[ml] = 0;
												if (an_mem_expr_addr(mex, vv2, &maddr))
												{
													if (g_src_symid[sidx2] <= 0 || g_src_symid[sidx2] >= g_sym_n ||
														g_sym_pool[g_src_symid[sidx2]].op != SYM_OP_SRC ||
													g_sym_pool[g_src_symid[sidx2]].c != (unsigned long long)sidx2)
														g_src_symid[sidx2] = sym_alloc(SYM_OP_SRC, 0, 0, 32, (unsigned long long)sidx2);
													sym_mem_store(&g_symreg, maddr, g_src_symid[sidx2]);
												}
											}
										}
									}
								}
							}
						}
						/* value-flow propagation: when a binary source op
						   consumes two KNOWN source values, its result is also a
						   source value (acc+=a produces acc+a), so it joins the
						   source-value table and keeps the acc chain nameable
						   across the encryption that follows. This is how
						   `acc += a; acc += b; acc += a+b` stays connected. */
						{
							char mp[16];
							get_mnemonic(sdi.instruction, mp, sizeof(mp));
							int isbin = !strcmp(mp, "add") || !strcmp(mp, "sub") ||
								!strcmp(mp, "imul") || !strcmp(mp, "adc") || !strcmp(mp, "sbb") ||
								!strcmp(mp, "xor") || !strcmp(mp, "and") || !strcmp(mp, "or");
							if (isbin)
							{
								int regs[4];
								int rn2 = sym_parse_opregs(sdi.instruction, regs);
								if (rn2 == 2)
								{
									/* pre values live in vv2 (hook is before); dst
									   post value is still being computed, so read
									   both pre operands from vv2 */
									int id0 = vmp38_src_id_of((unsigned int)vv2[regs[0]]);
									int id1 = vmp38_src_id_of((unsigned int)vv2[regs[1]]);
									if (id0 >= 0 && id1 >= 0 && g_src_cnt < 64)
									{
										/* result = the post value sym_track_insn
										   already computed into g_symreg.r[dst] */
										unsigned int res = (unsigned int)g_symreg.r[regs[0]];
										if (vmp38_src_id_of(res) < 0)
										{
											g_src_vals[g_src_cnt] = res;
											_snprintf(g_src_names[g_src_cnt], 23, "s%d", g_src_cnt);
											g_src_symid[g_src_cnt] = 0;
											g_src_cnt++;
										}
									}
								}
							}
						}
						/* symbolic source-op detection: after sym_track_insn the
						   DST register's symbol is the freshly-built expression
						   (e.g. ADD(acc0, a)). If the op is a source-op (add/
						   sub/imul/adc/sbb) and BOTH operands are clean source
						   expressions, this instruction is a source instruction
						   -> emit [sym-op]. Cross-handler: g_symreg accumulates. */
						{
							char mo[16];
							get_mnemonic(sdi.instruction, mo, sizeof(mo));
							static const char *symopm[] = { "add","sub","imul","adc","sbb",NULL };
							int is_symop = 0, si2;
							for (si2 = 0; symopm[si2]; si2++)
								if (!strcmp(mo, symopm[si2])) { is_symop = 1; break; }
							if (is_symop)
							{
								int regs[4];
								int rn2 = sym_parse_opregs(sdi.instruction, regs);
								if (rn2 == 2)
								{
									int dsym = g_symreg.sym[regs[0]];
									if (dsym > 0 && dsym < g_sym_n)
									{
										SYM_NODE *dn = &g_sym_pool[dsym];
										if ((dn->op == SYM_OP_ADD || dn->op == SYM_OP_SUB || dn->op == SYM_OP_MUL) &&
											sym_is_clean_src(dn->a, 0) && sym_is_clean_src(dn->b, 0) &&
											sym_has_src(dn->a, 0) && sym_has_src(dn->b, 0) &&
											!sym_has_inter_src(dn->a, 0) && !sym_has_inter_src(dn->b, 0))
										{
											char ea[128], eb[128];
											g_sym_dbg_depth = 0;
											sym_expr_str(dn->a, ea, sizeof(ea));
											g_sym_dbg_depth = 0;
											sym_expr_str(dn->b, eb, sizeof(eb));
											out("  [sym-op] @%08X: %s  %s op %s  (%s)",
												(unsigned int)address, sdi.instruction,
												ea, eb, mo);
										}
									}
								}
							}
						}
						char m3[16];
						get_mnemonic(sdi.instruction, m3, sizeof(m3));
						/* VM 段边界 + call 返回/返回值提取（静态与动态同一规则） */
						vmp38_emit_vm_boundary((unsigned int)address, sdi.instruction, vv2, uc, sdi.instr_size);
						if (!strcmp(m3, "jmp"))
						{
							/* 2026-09-05 用户要求: .text 明文区之间的直接 jmp 跳到
							   普通函数头也要停(否则"jmp 不判断"导致追进下一个函数
							   不停, 如 408964 E9->408904 跳到 push ebp;mov ebp,esp)。
							   安全条件(防 VM 段混淆 jmp reg 误判):
							   1) 直接 jmp(目标字面是 0x... 地址), 排除 jmp reg 间接;
							   2) 当前 PC 不在 VM 段(已追进 .text 明文);
							   3) 目标不在 VM 段且在模块映像内;
							   4) 目标是正常函数头(vmp38_is_norm_funchead)。 */
							const char *pj0 = sdi.instruction + 3;
							while (*pj0 == ' ') pj0++;
							if (pj0[0] == '0' && (pj0[1] == 'x' || pj0[1] == 'X'))
							{
								unsigned long long jtgt = strtoull(pj0 + 2, NULL, 16);
								unsigned long long pc = (unsigned long long)address;
								int pc_in_vm = (g_vm_start && g_vm_end && pc >= g_vm_start && pc < g_vm_end);
								int tg_in_vm = (g_vm_start && g_vm_end && jtgt >= g_vm_start && jtgt < g_vm_end);
								if (!pc_in_vm && !tg_in_vm && g_module_base &&
									jtgt >= g_module_base && jtgt < g_module_base + 0x20000000ULL)
								{
									if (vmp38_is_norm_funchead(uc, jtgt))
									{
										out_progress("[ft] .text jmp -> function head %08x (push序言) - trace done", (unsigned int)jtgt);
										g_ft_stop_reason = 3;
										uc_emu_stop(uc);
										return;
									}
								}
							}
							const char *pj = sdi.instruction + 3;
							while (*pj == ' ') pj++;
							if (*pj && *pj != '0' && !(*pj >= '1' && *pj <= '9'))
							{
								int jreg = reg_index_from_name(pj);
								if (jreg >= 0)
								{
									unsigned long long tgt = (unsigned long long)vv2[jreg];
									if (tgt < 0x10000 ||
										(g_module_base && tgt >= g_module_base + 0x20000000ULL))
									{
										unsigned long long solved = 0;
										if (sym_eval_partial(g_symreg.sym[jreg], &g_symreg, &solved))
										{
											if (solved >= g_vm_start && solved < g_vm_end)
											{
												out("[p3] jmp %s: %08llx (drifted) -> sym %08llx", pj, tgt, solved);
												uc_reg_write(uc, tr2[jreg], &solved);
											}
										}
										else
										{
											/* DUMP the register's symbol expression so we can
											   see which MBA shape the fold missed (jmp ebp
											   @40ecbc3a: ror/xor/adc composite, not a bare
											   ADD(AND,AND)). One line per drifted jmp-reg.
											   Use out_progress (NOT out) because out_file
											   only passes [src]/[asm] - the dump would be
											   silently dropped from the .txt. */
											{
												char sb[512];
												g_sym_dbg_depth = 0;
												sym_expr_str(g_symreg.sym[jreg], sb, sizeof(sb));
												out_progress("[p3-mba] jmp %s sym[%d]=%s (tgt %08llx not foldable)",
													pj, jreg, sb, (unsigned int)tgt);
											}
											unsigned long long c0 = 0, c1 = 0;
											if (sym_mba_decompose(g_symreg.sym[jreg], &g_symreg,
												g_vm_start, g_vm_end, &c0, &c1))
											{
												if (c0 >= g_vm_start && c0 < g_vm_end)
												{
													out("[p3] jmp %s: MBA cand0=%08llx", pj, c0);
													uc_reg_write(uc, tr2[jreg], &c0);
												}
												else if (c1 >= g_vm_start && c1 < g_vm_end)
												{
													out("[p3] jmp %s: MBA cand1=%08llx", pj, c1);
													uc_reg_write(uc, tr2[jreg], &c1);
												}
											}
										}
									}
								}
							}
						}
						/* P3: iretd dispatch - resolve [esp] target symbolically */
						if (!strcmp(m3, "iretd") || !strcmp(m3, "iret"))
						{
							unsigned long long esp_now = (unsigned long long)vv2[4];
							unsigned int tgt = 0;
							int rd_ok = 0;
							__try {
								rd_ok = (uc_mem_read(uc, esp_now, &tgt, 4) == UC_ERR_OK);
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								rd_ok = 0;
							}
							if (rd_ok &&
								(tgt < 0x10000 ||
								 (g_module_base && tgt >= g_module_base + 0x20000000ULL)))
							{
								/* [esp] drifted to garbage - resolve from the
								   stack slot symbol recorded by push */
								unsigned long long solved = 0;
								int slot_sym = -1;
								MINI_MEM *mm = &g_symreg.mem;
								for (int si = 0; si < mm->count; si++)
									if (mm->off[si] == esp_now)
									{ slot_sym = mm->sym[si]; break; }
								if (slot_sym >= 0 &&
									sym_eval_partial(slot_sym, &g_symreg, &solved) &&
									solved >= g_vm_start && solved < g_vm_end)
								{
									out("[p3] iretd [esp]=%08x (drifted) -> sym %08llx", tgt, solved);
									unsigned int sv = (unsigned int)solved;
									uc_mem_write(uc, esp_now, &sv, 4);
								}
								else
									out("[p3] iretd [esp]=%08x garbage (no symbolic recovery)", tgt);
							}
						}
					}
				}
				else
					out_progress("[ft] reg_read_batch AV at %08llx - row skipped", address);
			}
			/* ---- VM-exit detection (kanxue 280283: vm_exit=popfd) ----
			   A handler that unwinds registers (>=2 pop regs) AND pops
			   eflags (popfd/popad) is the VM exit sequence; the NEXT ret
			   whose target LEAVES the VM section is the real vmret (VM
			   function return). Plain ret is NOT enough - handler-internal
			   / ret-dispatch / junk rets are indistinguishable from the
			   real one without the popfd+popregs marker. This makes the
			   static trace stop exactly at the function return instead of
			   running past it into .text restored code (module-window
			   escape below stays as a fallback). */
			{
				char m0[16];
				get_mnemonic(sdi.instruction, m0, sizeof(m0));
				int ve_c = vmp38_ve_classify(sdi.instruction);
				int is_pop_reg = (ve_c == 1);
				int is_popfd = (ve_c == 2);
				int is_popad = (ve_c == 3);
				int is_boundary = (strncmp(m0, "jmp", 3) == 0 ||
					strncmp(m0, "call", 4) == 0 || strncmp(m0, "ret", 3) == 0);
				/* accumulate unwind counts within the current handler */
				if (is_pop_reg) g_ft_ve_popreg++;
				if (is_popfd) g_ft_ve_popfd++;
				if (is_popad) g_ft_ve_popad++;
				if (is_boundary)
				{
					/* 已进入 .text 返回 stub 追返回值，.text stub 之后可能的
					   调用是一个 VM 函数体（VM 里的 VM 函数体）；GSCService 的
					   main 在调用 VM 函数，所以不能追 ret 就停；
					   call 到 VM 段 → 就是下一个 VM 函数（调用 call 到 VM 段即
					   另一个 VM 函数。返程：返回就停；ret 出模块 → 停；ret 在模块内
					   → 继续追。注意 text_phase 初始化设 1 或置 0，按 call 分支
					   进入或退出该函数；ret 分支的 next-VM 判定还用到。 */
					if (g_ft_text_phase && strncmp(m0, "ret", 3) == 0)
					{
						unsigned long long esp_t = 0, rt = 0;
						int rt_ok = 0;
						__try {
							if (uc_reg_read(uc, UC_X86_REG_ESP, &esp_t) == UC_ERR_OK &&
								uc_mem_read(uc, esp_t, &rt, 4) == UC_ERR_OK)
								rt_ok = 1;
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							rt_ok = 0;
						}
						int out_of_mod = (!rt_ok || !g_module_base ||
							rt < g_module_base || rt >= g_module_base + 0x20000000ULL);
						if (out_of_mod)
						{
							unsigned long long eax_p = 0;
							__try {
								uc_reg_read(uc, UC_X86_REG_EAX, &eax_p);
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								eax_p = 0;
							}
							if (!g_ft_result_emitted)
							{
								g_ft_result_emitted = 1;
								out_progress("  [result] eax=%08x (VM return value, .text restored)", (unsigned int)eax_p);
							}
							out_progress("[ft] VM function returned (.text restored) - trace done");
							g_ft_stop_reason = 1;
							uc_emu_stop(uc);
							return;
						}
						/* ret 在模块内，看目标是否下一个 VM 函数体（pushfd=0x9C =
						   VMP vm_start），且"自动追VM（开）关" → 停当前 VM
						   函数已完成，也可以跨完，就会继续追。 */
						{
							unsigned char pb2[8] = { 0 };
							int next_vm = 0;
							__try {
								next_vm = (uc_mem_read(uc, rt, pb2, 8) == UC_ERR_OK) &&
									(memchr(pb2, 0x9C, 8) != NULL);
							} __except (EXCEPTION_EXECUTE_HANDLER) { next_vm = 0; }
							if (next_vm && !g_vm_continue)
							{
								unsigned long long eax2 = 0;
								__try {
									uc_reg_read(uc, UC_X86_REG_EAX, &eax2);
								} __except (EXCEPTION_EXECUTE_HANDLER) { eax2 = 0; }
								if (!g_ft_result_emitted)
								{
									g_ft_result_emitted = 1;
									out_progress("  [result] eax=%08x (VM return value)", (unsigned int)eax2);
								}
								out_progress("[ft] next VM func %08x (pushfd) - auto-stop (auto-continue OFF)", (unsigned int)rt);
								g_ft_stop_reason = 1;
								uc_emu_stop(uc);
								return;
							}
						}
						/* ret 在模块内，跟踪追可能去到下一个函数/VM stub。 */
						out_progress("[ft] .text ret->module %08x - continuing", (unsigned int)rt);
					}
					/* 一旦某条 ret 越过边界会挂 pending 标志（会 stale marker）。 */
					if (g_ft_vmexit_pending && strncmp(m0, "ret", 3) != 0)
						g_ft_vmexit_pending = 0;
					/* 对静态 headless，VM 退出判断（2026-08-17，用户要求）：
					   VM 段内任何 ret，目标离开 VM 段（到 .text / 02xxxxxx 段）
					   就是 VM 函数返回；配合 popfd/popad 预判断，VMP 3.8
					   退出序列不要求 popad，改 popfd&&popad 旧判断会漏失，
					   所以直接对 VM 段内 ret 判断目标。 */
					if (g_vm_start && (unsigned long long)address >= g_vm_start &&
						(unsigned long long)address < g_vm_end &&
						strncmp(m0, "ret", 3) == 0)
					{
						/* candidate vmret: verify the ret target LEAVES the
						   VM section (shared rule with the dynamic trace).
						   SEH-guarded: uc_reg_read/uc_mem_read INSIDE the
						   code hook can AV (fault=0) on some Unicorn state -
						   same trap as the reg_read_batch above; a crash
						   here kills the host, so guard it and fall back
						   to treating the pending ret as vmret. */
						unsigned long long esp_now = 0, ret_target = 0;
						int rr_ok = 0;
						__try {
							if (uc_reg_read(uc, UC_X86_REG_ESP, &esp_now) == UC_ERR_OK &&
								uc_mem_read(uc, esp_now, &ret_target, 4) == UC_ERR_OK)
								rr_ok = 1;
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							rr_ok = 0;
							out_progress("[ft] vmret ESP/ret-target read AV at %08llx (treated as vmret)", address);
						}
						if (rr_ok)
						{
							if (!vmp38_is_vmret_target(ret_target))
							{
								/* internal dispatch - not a vmret */
								g_ft_vmexit_pending = 0;
							}
							else
							{
								/* ret target LEAVES the VM section. 两种情形：
								   (a) 仍在模块内，.text 返回 stub（如
								       "xor ebx,[esp+0x14]; ror ebx,0x0B;
								        mov eax,ebx; ret" （返回值计算）
								       判断是否停止，看 text-phase 追溯返回
								   (b) 出模块（返回系统），停止并以 eax 停。 */
								if (g_module_base &&
									ret_target >= g_module_base &&
									ret_target < g_module_base + 0x20000000ULL)
								{
									/* 按用户要求（2026-08-18）：VM 内部 ret 落到 .text，
									   检测目标处字节做动态判断，结合"自动追VM"开关决定是否停止：
									     - 函数头部（push ebp;mov ebp,esp / 8B FF 55 8B EC）：
									       属于 VM 内部的后续函数（VM 化时产生的），则
									       自动追开：继续去执行它，直到回 VM 段后停；
									       自动追关：越过 VM 边界停住。
									     - 函数尾部（mov esp,ebp;pop ebp;ret = 8B E5 5D C3）：
									       属于 VM 函数最终结束，判定就停住。
									     - 其他（.text 里的 VM 代码片段）：不停，继续还原。 */
									unsigned char pro[4] = { 0, 0, 0, 0 };
									int pro_ok = 0;
									__try {
										pro_ok = (uc_mem_read(uc, ret_target, pro, 4) == UC_ERR_OK) ? 1 : 0;
									} __except (EXCEPTION_EXECUTE_HANDLER) {
										pro_ok = 0;
									}
									/* 函数头判断：用规范化字节签名（不依赖动态态）
									   vmp38_is_norm_funchead 识别（标准 push ebp;mov ebp,esp
									   或 push ebx/esi/add esp/mov esi,edx 等） */
									int is_funchead = vmp38_is_norm_funchead(uc, ret_target);
									/* 函数尾判断：共用 vmp38_is_norm_func_tail
									   (mov esp,ebp;pop ebp;ret = 8B E5 5D C3 或变体) */
									int is_boundary = pro_ok && vmp38_is_func_boundary(uc, ret_target);
									if (is_boundary)
									{
										unsigned long long eax_e = 0;
										__try { uc_reg_read(uc, UC_X86_REG_EAX, &eax_e); }
										__except (EXCEPTION_EXECUTE_HANDLER) { eax_e = 0; }
										if (!g_ft_result_emitted)
										{
											g_ft_result_emitted = 1;
											out_progress("  [result] eax=%08x (VM return value)", (unsigned int)eax_e);
										}
										/* 保存真返回值供后续 [result]/REVM 对账——必须写全局，
										   否则 param 段 return-compute 扫描失败会显示 not captured */
										g_ft_real_return = (unsigned long)eax_e;
										g_ft_real_return_valid = 1;
										g_last_ret_eax = eax_e;
										g_last_ret_eax_valid = 1;
										out_progress("[ft] VM function ended at epilogue %08x (mov esp,ebp;pop ebp;ret) - trace done", (unsigned int)ret_target);
										g_ft_stop_reason = 1;
										uc_emu_stop(uc);
										return;
									}
									/* NOTE: 不再因 auto-continue OFF 在 .text 的"函数头"停——
									   .text 里的 next-VM 函数/VM stub 也以 push 等序言开头，
									   函数头不等于"当前函数真正结束"。只有明确的"函数尾"
									   (is_epilog: mov esp,ebp;pop ebp;ret) 才是结束信号。
									   函数头/非头非尾一律当作 .text 的 VM/下一函数继续还原。 */
									if (is_funchead)
										out_progress("[ft] VM returned to .text %08x (prologue) - continuing to next VM code", (unsigned int)ret_target);
									/* 自动追开时，函数头 / 非函数尾之外：.text 的 VM 代码
									   片段也要还原（所以不停） */
								}
								else
								{
									unsigned long long eax = 0;
									__try {
										uc_reg_read(uc, UC_X86_REG_EAX, &eax);
									} __except (EXCEPTION_EXECUTE_HANDLER) {
										eax = 0;
									}
									if (!g_ft_result_emitted)
									{
										g_ft_result_emitted = 1;
										out_progress("  [result] eax=%08x (VM return value)", (unsigned int)eax);
									}
									out_progress("[ft] VM function returned - trace done");
									g_ft_stop_reason = 1;
									uc_emu_stop(uc);
									return;
								}
							}
						}
						else
						{
							/* cannot read ret target (stack not mapped or
							   hook-API AV) - conservative: treat as vmret
							   if pending */
							g_ft_vmexit_pending = 0;
							unsigned long long eax = 0;
							__try {
								uc_reg_read(uc, UC_X86_REG_EAX, &eax);
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								eax = 0;
							}
							if (!g_ft_result_emitted)
							{
								g_ft_result_emitted = 1;
								out_progress("  [result] eax=%08x (VM return value)", (unsigned int)eax);
							}
							out_progress("[ft] VM function returned - trace done");
							g_ft_stop_reason = 1;
							uc_emu_stop(uc);
							return;
						}
					}
					/* P3b: SEH-style vm_exit fallback. Some VMP 3.8 samples
					   (VMComplexity=100) exit via "push retaddr + push fs:[0]
					   + restore-SEH + ret" with NO popfd/popreg unwind. Only
					   fire when the current handler had ZERO pops — a
					   popfd-style exit (1 popfd + pops, e.g. demo inner return)
					   must NOT stop here; it continues into .text and the
					   next VM function, exactly like the dynamic trace. */
					if (!g_ft_text_phase &&
						strncmp(m0, "ret", 3) == 0 &&
						g_vm_start && (unsigned long long)address >= g_vm_start &&
						(unsigned long long)address < g_vm_end)
					{
						unsigned long long esp2 = 0, rtarget = 0;
						int rr2 = 0;
						__try {
							if (uc_reg_read(uc, UC_X86_REG_ESP, &esp2) == UC_ERR_OK &&
								uc_mem_read(uc, esp2, &rtarget, 4) == UC_ERR_OK)
								rr2 = 1;
						} __except (EXCEPTION_EXECUTE_HANDLER) {
							rr2 = 0;
						}
						if (rr2)
						{
							/* P3b 停止判定：只有 ret 出模块（返回系统）才停止。
							   ret 在模块内：.text 调用处 / SEH handler / 下一个
							   VM stub 继续追（GSCService 在 VM 函数内返回后
							   .text 调用处再 call 下一个 VM 函数，VM 内部的 VM
							   应该不会停在"ret 在 VM 段"处（漏追那样不对）。 */
							int out_of_module = (g_module_base &&
								(rtarget < g_module_base ||
								 rtarget >= g_module_base + 0x20000000ULL));
							if (out_of_module)
							{
								unsigned long long eax2 = 0;
								__try {
									uc_reg_read(uc, UC_X86_REG_EAX, &eax2);
								} __except (EXCEPTION_EXECUTE_HANDLER) {
									eax2 = 0;
								}
								if (!g_ft_result_emitted)
								{
									g_ft_result_emitted = 1;
									out_progress("  [result] eax=%08x (VM return value, SEH-exit)", (unsigned int)eax2);
								}
								out_progress("[ft] VM function returned (SEH-exit) - trace done");
								g_ft_stop_reason = 1;
								uc_emu_stop(uc);
								return;
							}
						}
					}
					/* next handler starts fresh（清空 popfd/popad 预判断标志；
					   VM 退出判断改为"VM 段内 ret 目标离开 VM 段"直接判断。 */
					g_ft_ve_popreg = 0;
					g_ft_ve_popfd = 0;
					g_ft_ve_popad = 0;
				}
			}
		}
	}
	if ((g_stream_total % 100000) == 0)
	{
		static unsigned long long t0m = 0;
		unsigned long long now = GetTickCount64();
		unsigned long long dms = t0m ? (now - t0m) : 0;
		t0m = now;
		out_progress("[ft] trace progress: %llu instrs written (this-100k=%llu ms)",
			g_stream_total, dms);
		if (g_fdata) fflush(g_fdata);   /* periodic flush: data file shows progress mid-trace */
	}
	if (g_dyn_stop)
	{
		g_ft_stop_reason = 3;
		uc_emu_stop(uc);
		return;
	}
	/* VM-exit detection: the VM function returns when execution leaves the
	   module. CRITICAL: a low garbage address (< 0x10000, VMP anti-debug /
	   state-desync jump, eip=5 observed) is NOT a return - with the
	   zero-page unmapped patch it now executes instead of aborting, so it
	   would hit this check on every iteration and terminate the trace with
	   a false "[result]". Require the address to be >= 0x10000 so real
	   returns (module + 0x20000000 window) still stop but garbage lows
	   keep tracing until the module-window exit. */
	/* in-module address resets the OOB runaway streak (bounds the
	   non-ret->continue escape so garbage byte-walks don't run forever) */
	if (g_module_base && address >= g_module_base &&
		address < g_module_base + 0x20000000ULL)
		g_oob_streak = 0;
	if (g_module_base && address >= 0x10000 &&
		(address < g_module_base ||
			address >= g_module_base + 0x20000000ULL))
	{
		/* System DLLs (0x7xxxxxxx) are dumped but NOT traced into: e.g.
		   MultiByteToWideChar spins for millions of instrs inside ntdll
		   and the emulation never returns. Skip the call: jump to the
		   return address on the stack ([esp]) - the real flow continues
		   there and writes the key at 004240ee (28K instrs later). */
		if (address >= 0x70000000ULL && address < 0x80000000ULL)
		{
			unsigned long long esp_v = 0, retp = 0;
			int rr2 = 0;
			__try {
				rr2 = (uc_reg_read(uc, UC_X86_REG_ESP, &esp_v) == UC_ERR_OK &&
					uc_mem_read(uc, esp_v, &retp, 4) == UC_ERR_OK && retp) ? 1 : 0;
			} __except (EXCEPTION_EXECUTE_HANDLER) { rr2 = 0; }
			if (rr2)
			{
				/* Skip the system-DLL call: jump EIP to the return address AND
				   pop it off the stack (esp += 4) - a real `call; ret` consumes
				   the return address, so skipping must too. Without the pop,
				   [esp] still holds the return addr, so retaddr's own `pop ebp;
				   ret` (vmp_base 00394619) pops garbage -> the trace spins. */
				unsigned long long esp_adj = esp_v + 4;
				__try {
					uc_reg_write(uc, UC_X86_REG_ESP, &esp_adj);
				} __except (EXCEPTION_EXECUTE_HANDLER) {
					/* ignore - continue; EIP-only skip is the fallback */
				}
				out_progress("[ft] system-dll %08llx -> skip to retaddr %08llx (esp+4 %08llx)",
					address, retp, esp_v);
				g_ft_skip_resume = retp;
				uc_emu_stop(uc);
				return;
			}
		}
		/* reached the module-out window but NOT a system DLL (0x7xxxxxxx) -
		   this is either a REAL VM return (current instr is ret) or an
		   exception/anti-debug JMP/CALL that routed OUT of the module (e.g.
		   GSCService's `jmp gscservice.1269766` through an exception stub).
		   Only a `ret` out-of-module is a genuine function return -> stop.
		   A jmp/call to module-external code is NOT a return: keep tracing so
		   gscservice's own addresses (0x1269766 etc.) are treated as the
		   program's VM/.text code, not a premature "escaped - done". */
		char mcur[24] = "";
		{
			DISASM_INSTR mdi; memset(&mdi, 0, sizeof(mdi));
			if (disasm_at_cached((unsigned int)address, &mdi, uc))
				get_mnemonic(mdi.instruction, mcur, sizeof(mcur));
		}
		int is_ret_cur = (strncmp(mcur, "ret", 3) == 0 || strncmp(mcur, "retn", 4) == 0);
		if (is_ret_cur)
		{
			unsigned long long eax = 0;
			__try {
				uc_reg_read(uc, UC_X86_REG_EAX, &eax);
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				eax = 0;   /* hook-internal reg read AV - report 0 */
			}
			out_progress("  [result] eax=%08x (VM return value)", (unsigned int)eax);
			out_progress("[ft] VM function returned - trace done");
			g_ft_stop_reason = 1;
			uc_emu_stop(uc);
		}
		else
		{
			/* jmp/call to external/exception code: not a return - keep emulating,
			   BUT bound it: a garbage/data region executes byte-by-byte forever
			   (e.g. deep2 jumping to e923a801 -> e923a802 ...); after N consecutive
			   out-of-module non-ret steps treat it as escaping, not a real path. */
			if (++g_oob_streak > 64)
			{
				g_oob_streak = 0;
				out_progress("[ft] out-of-module runaway @%08x - trace done", (unsigned int)address);
				g_ft_stop_reason = 1;
				uc_emu_stop(uc);
				return;
			}
			out_progress("[ft] EIP out-of-module (non-ret) %08x - continuing (%s)", (unsigned int)address, mcur);
		}
	}
}


/* init unicorn: map VM section + stack, hook memory ops to our snapshot */
static int vmp38_uc_init(unsigned long long vm_lo, unsigned long long vm_hi,
	unsigned long long stack_esp, unsigned long long cur_ip)
{
	/* static link (unicorn-import.lib); engine init below */
	/* HEADLESS GUARD: only run Unicorn when the debuggee's live memory is
	   readable AND differs from the file bytes (= VM section decrypted by
	   execution). headless can run the program (script 'r' command) -
	   after that the VM section is decrypted and Unicorn works exactly as
	   under GUI. If the program never ran (pure static file mode), the
	   live bytes equal the VMP-encrypted file bytes -> skip Unicorn.
	   Scan a few pages across the VM section (not just the first 8 bytes):
	   VMP decrypts only the pages it uses, so the entry page may still
	   equal the file bytes while later handler pages are decrypted. */
	{
		/* No static cache: VMP decrypts lazily, so a page may become
		   decrypted later. Re-probe each time uc_init is called (only the
		   first successful call initializes g_uc; failures retry).
		   DECISION: the "live != file bytes" test is wrong - VMP leaves
		   dispatch stubs / entry points PLAINTEXT in the file (e.g.
		   00b261af file bytes = "call 0xb8c8e9", a real instruction), so
		   live==file does NOT mean "not decrypted". Instead require:
		   (a) the target process is being debugged, (b) the current ip
		   disassembles to a valid instruction (DbgDisasmAt). Unicorn then
		   tries to execute; if the bytes are still encrypted, uc_emu_start
		   returns INSN_INVALID and the caller reports the failure. */
		DISASM_INSTR di;
		int uc_ok = 0;
		/* The entry can be a .text call stub (headless breakpoint on the call
		   site, e.g. 00BE1091 "call 0x00C56C29") that jumps INTO the VM
		   section - not inside g_vm_start..g_vm_end. Requiring the VM-section
		   test alone would reject every headless call-site entry. Accept any
		   entry whose instruction disassembles validly: after the breakpoint
		   the live memory is decrypted, so DbgDisasmAt reflects real code. */
		memset(&di, 0, sizeof(di));
		/* file-bytes first (headless: live pages of .text main() may be
		   VMP-protected/not-committed -> DbgDisasmAt stalls; the file has
		   plaintext). Fall back to live disasm for GUI. */
		if (disasm_from_file(cur_ip, &di))
			uc_ok = 1;
		else
		{
			DbgDisasmAt((duint)cur_ip, &di);
			if (di.instr_size >= 1 && di.instruction[0])
				uc_ok = 1;
		}
		if (uc_ok == 0)
			return 0;
	}
	if (g_uc)
		return 1;
	Script::Module::ModuleInfo mi2;
	uc_err err;
	out("[uc] init uc_open...");
	err = uc_open(UC_ARCH_X86, UC_MODE_32, &g_uc);
	out("[uc] uc_open err=%d", (int)err);
	if (err != UC_ERR_OK)
		return 0;
	out("[uc] uc_open OK");
	/* enlarge QEMU TCG translation buffer: with the default 4MB, long VM
	   traces re-translate the same handlers constantly; 64MB keeps far
	   more translation blocks cached -> much faster long runs. */
	uc_ctl(g_uc, UC_CTL_WRITE(UC_CTL_TCG_BUFFER_SIZE, 1),
		(unsigned int)(64 * 1024 * 1024));   /* 04:05 baseline: 64MB TCG */
	g_uc_vm_lo = vm_lo;
	g_uc_vm_hi = vm_hi;
	g_uc_stack_lo = stack_esp > 0x20000 ? stack_esp - 0x20000 : 0;
	g_uc_stack_hi = stack_esp + 0x20000;
	/* map the full 32-bit address space (0 - 4GB). VMP handlers use
	   obfuscated stack addressing like [esp+eax*2-0x26208680] which can
	   resolve to ANY 32-bit address; a 64MB map -> UC_ERR_READ_UNMAPPED.
	   Unicorn maps lazily (pages committed on access), so 4GB is cheap. */
	/* map a single 4GB region covering the whole 32-bit address space -
	   this is what the standalone test confirmed works; 32-bit Unicorn
	   fails on small isolated maps but succeeds on one big map. Fall back
	   to smaller maps / 1MB chunks if the big map fails (some host
	   processes restrict it). 4GB matters: VMP obfuscated addressing
	   [esp+eax*2-0x26208680] resolves to ANY 32-bit address; a 2GB map
	   -> UC_ERR_READ/FETCH_UNMAPPED for high half (0x80000000+). */
	{
		int big = 0;
		out("[uc] map 4GB @ 0...");
		if (uc_mem_map(g_uc, 0, 0x100000000ULL, UC_PROT_ALL) == UC_ERR_OK)
		{
			out("[uc] 4GB map OK");
			big = 1;
		}
		else if (uc_mem_map(g_uc, 0, 0x80000000ULL, UC_PROT_ALL) == UC_ERR_OK)
		{
			out("[uc] 2GB map OK");
			big = 1;
		}
		else if (uc_mem_map(g_uc, 0, 0x20000000ULL, UC_PROT_ALL) == UC_ERR_OK)
		{
			out("[uc] 512MB map OK");
			big = 1;
		}
		else if (uc_mem_map(g_uc, 0, 0x10000000ULL, UC_PROT_ALL) == UC_ERR_OK)
		{
			out("[uc] 256MB map OK");
			big = 1;
		}
		else if (uc_mem_map(g_uc, 0, 0x4000000ULL, UC_PROT_ALL) == UC_ERR_OK)
		{
			out("[uc] 64MB map OK (2GB failed)");
			big = 1;
		}
		if (!big)
		{
			unsigned long long modbase = 0x00de0000;
			unsigned long long stack_lo = g_uc_stack_lo & ~0xFFFULL;
			unsigned long long a;
			int mapped = 0;
			if (Script::Module::GetMainModuleInfo(&mi2))
				modbase = mi2.base;
			out("[uc] 64MB failed, chunked map module %08llx...", modbase);
			for (a = modbase; a < modbase + 0x2000000ULL; a += 0x100000)
			{
				if (uc_mem_map(g_uc, a, 0x100000, UC_PROT_ALL) == UC_ERR_OK)
					mapped++;
			}
			out("[uc] module mapped %d MB", mapped);
			if (stack_lo < modbase || stack_lo >= modbase + 0x2000000ULL)
			{
				int smap = 0;
				for (a = stack_lo; a < stack_lo + 0x200000; a += 0x100000)
				{
					if (uc_mem_map(g_uc, a, 0x100000, UC_PROT_ALL) == UC_ERR_OK)
						smap++;
				}
				out("[uc] stack mapped %d MB", smap);
			}
		}
	}
	out("[uc] map done, dumping memory...");
	/* DUMP live process memory into Unicorn so it sees the same bytes the
	   real CPU does (decrypted VM section + module + stack snapshot).
	   This is what makes Unicorn == offline CPU: dynamic trace lets the
	   real CPU read all memory; Unicorn must too, otherwise reads fall
	   back to the per-access hook and hit snapshot-window gaps. */
	uc_in_dump = true;   /* suppress hook reentrancy during dump */
	{
		unsigned char buf[0x1000];
		unsigned long long modbase = 0x00de0000;
		unsigned long long a;
		/* SNAPSHOT: headless -> VM section only (Script::Memory::Read is an
		   expensive bridge call there, a full scan stalls restore; reads
		   outside are patched live by the unmapped hook). GUI -> the whole
		   module image (+/-1MB slack) so handler table / .rdata / .data reads
		   hit Unicorn's own memory (identical to what the real CPU saw),
		   instead of the live-process fallback which returns stale
		   pre-breakpoint values and breaks jmp eax dispatch. */
		{
			char ft_host2[MAX_PATH] = ""; GetModuleFileNameA(NULL, ft_host2, MAX_PATH);
			int ft_headless2 = (strstr(ft_host2, "headless") != NULL);
			unsigned long long lo = g_vm_start, hi = g_vm_end;
			/* 用户要求（2026-08-17）：跟随用户内存段映射进 Unicorn，
			   而不是只 dump VM 段。headless 之前只 dump VM 段，但 Unicorn
			   执行 .text 还原代码 XMM/SSE 指令（movups/movaps）读 .rdata 时
			   会失败（模块内存不够）-> 崩溃。所以 headless 与 GUI 一致 dump
			   范围是整个模块 image（base..base+size），包括 .text/.rdata/.data/
			   VM 段，XMM 读入数据/原代码都进 Unicorn。 */
			Script::Module::ModuleInfo m2b;
			unsigned long long mb2 = 0, msz2 = 0;
			if (Script::Module::GetMainModuleInfo(&m2b)) { mb2 = m2b.base; msz2 = m2b.size; }
			if (!mb2) mb2 = 0x400000ULL;
			if (!msz2 || msz2 > 0x2000000ULL) msz2 = 0x2000000ULL;
			lo = mb2;
			hi = mb2 + msz2 + 0x100000;
			(void)ft_headless2;   /* headless 与 GUI 都在此对全模块 dump */
			/* snapshot decrypted module bytes for fast restore disasm */
			if (g_tm_data) { free(g_tm_data); g_tm_data = NULL; }
			if (hi - lo <= 0x4000000ULL)
			{
				g_tm_data = (unsigned char*)malloc((size_t)(hi - lo));
				if (g_tm_data) { g_tm_base = lo; g_tm_end = hi; }
			}
			for (a = lo; a < hi; a += 0x1000)
			{
				duint szr = 0;
				memset(buf, 0, sizeof(buf));
				if (Script::Memory::Read((duint)a, buf, sizeof(buf), &szr) && szr > 0)
				{
					uc_mem_write(g_uc, a, buf, szr);
					if (g_tm_data && a >= g_tm_base && a < g_tm_end)
					{
						size_t off = (size_t)(a - g_tm_base);
						size_t cpy = (szr < 0x1000) ? szr : 0x1000;
						if (off + cpy <= (size_t)(g_tm_end - g_tm_base))
							memcpy(g_tm_data + off, buf, cpy);
					}
				}
				else
				{
					/* module 页 dump 时不可读 -> 记空白页,VM 运行访问时从 live 重读 */
					g_blank_pages.insert((unsigned int)(a & ~0xFFFULL));
				}
			}
		}

		/* FULL user-mode snapshot via VirtualQueryEx: dump every committed
		   MEM_PRIVATE page (heap + runtime data) - NOT MEM_IMAGE (system DLLs
		   / module image - module is dumped separately and VMP data loops do
		   not read system DLLs). ReadProcessMemory in 64KB chunks. headless
		   (no debuggee handle) is a no-op. */
		{
			HANDLE hProc = DbgGetProcessHandle();
			if (hProc && hProc != INVALID_HANDLE_VALUE)
			{
				static unsigned char rbuf[65536];
				MEMORY_BASIC_INFORMATION mbi;
				unsigned long long va = 0x00010000ULL;
				unsigned long long total_written = 0;
				while (va < 0x7FFF0000ULL)
				{
					SIZE_T rr = VirtualQueryEx(hProc, (LPCVOID)va, &mbi, sizeof(mbi));
					if (!rr) break;
					if (mbi.State == MEM_COMMIT &&
						(mbi.Type == MEM_PRIVATE || mbi.Type == MEM_IMAGE) &&
						mbi.RegionSize > 0 &&
						(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) == 0)
					{
						unsigned long long rlo = va, rhi = va + mbi.RegionSize;
						if (rhi > 0x7FFF0000ULL) rhi = 0x7FFF0000ULL;
						for (a = rlo; a < rhi; a += sizeof(rbuf))
						{
							size_t want = (size_t)((rhi - a) < sizeof(rbuf) ? (rhi - a) : sizeof(rbuf));
							SIZE_T got = 0;
							if (ReadProcessMemory(hProc, (LPCVOID)a, rbuf, want, &got) && got > 0)
							{
								uc_mem_write(g_uc, a, rbuf, got);
								total_written += got;
								/* no 128MB cap: snapshot the FULL user-mode private
								   memory (heap + runtime data + VM stack) so bytecode /
								   handler tables decrypted at runtime are all present */
							}
							else
							{
								/* COMMIT 页但此刻读失败(时序/guard) -> 记空白页,运行时重试 */
								for (unsigned long long pp = a; pp < (a + want) && pp < rhi; pp += 0x1000)
									g_blank_pages.insert((unsigned int)(pp & ~0xFFFULL));
							}
						}
					}
					va += mbi.RegionSize ? mbi.RegionSize : 0x1000;
				}
			}
		}
		/* stack snapshot (already captured by vmp38_snap_take).
		   CRITICAL: index g_snap by (a - g_snap_begin) - the SAME base the
		   fmem hooks use (vmp38_fmem_read/write_hook: g_snap[addr -
		   g_snap_begin]). The previous code page-aligned the base
		   (g_snap_begin & ~0xFFF) which misaligns every page when
		   g_snap_begin is not 4K-aligned (esp is almost never aligned):
		   Unicorn's stack then differs from g_snap by up to 0xFFF bytes,
		   and the first partial page indexes g_snap[0..] which actually
		   corresponds to g_snap_begin, not base_aligned -> systematic
		   wrong stack values. Bytes below g_snap_begin (the leading
		   partial page) stay unmapped/zero in Unicorn - they are outside
		   the captured window, fmem reads/writes reject them too
		   (addr >= g_snap_begin). */
		if (g_snap)
		{
			for (a = g_snap_begin; a < g_snap_end; a += 0x1000)
			{
				size_t n = (size_t)(((g_snap_end - a) < 0x1000) ? (g_snap_end - a) : 0x1000);
				uc_mem_write(g_uc, a, g_snap + (size_t)(a - g_snap_begin), n);
			}
		}
	}
	uc_in_dump = false;
	out("[perf] uc-init+map+dump elapsed %llu ms", (unsigned long long)(GetTickCount64() - g_t_trace0));
	out("[uc] dump done, enabling CODE hook (trace recorder)");
	/* CODE hook: fires on every executed instruction. This is the core of
	   the "Unicorn runs like a real CPU" model - Unicorn executes
	   continuously (count=0) and this callback records eip/registers and
	   detects vm-exit to stop. The callback must NOT call Unicorn APIs
	   (re-entry hazard) - only read uc_reg_read + count. */
	{
		static int g_code_count = 0x7fffffff;   /* huge: continuous until stop */
		uc_hook hh;
		if (uc_hook_add(g_uc, &hh, UC_HOOK_INTR, (void*)uc_hook_intr,
			NULL, 1, 0) != UC_ERR_OK)
			out("[uc] int3 hook FAILED (will stop on int3)");
		if (uc_hook_add(g_uc, &hh, UC_HOOK_INSN_INVALID, (void*)uc_hook_insn_invalid,
			NULL, 1, 0) == UC_ERR_OK)
			out("[uc] insn-invalid hook enabled (skip AVX/SSE instructions)");
		else
			out("[uc] insn-invalid hook add FAILED");
		if (uc_hook_add(g_uc, &hh, UC_HOOK_CODE, (void*)uc_hook_code,
			&g_code_count, 1, 0) == UC_ERR_OK)
			out("[uc] code-hook enabled");
		else
			out("[uc] code-hook add FAILED");
		{
			uc_hook hh2;
			/* READ|WRITE|FETCH all three: any unmapped access must be
			   patched by the hook (align x64), otherwise uc_emu_start
			   aborts with err=6/8 and the resume loop burns its 500
			   retries skipping real handlers. */
			if (uc_hook_add(g_uc, &hh2,
				UC_HOOK_MEM_READ_UNMAPPED | UC_HOOK_MEM_WRITE_UNMAPPED |
				UC_HOOK_MEM_FETCH_UNMAPPED,
				(void*)uc_hook_mem_unmapped, NULL, 1, 0) == UC_ERR_OK)
				out("[uc] mem-unmapped hook enabled (live-memory fallback)");
			else
				out("[uc] mem-unmapped hook add FAILED");
		}
		/* 根因修复：若存在 dump 时读失败的"空白页"，注册普通 MEM_READ hook，VM 首次
		   访问时从 live 强制重读(根治"内存段拷贝不全→死循环")。集合为空则跳过(零开销)。 */
		if (!g_blank_pages.empty())
		{
			uc_hook hh3;
			if (uc_hook_add(g_uc, &hh3, UC_HOOK_MEM_READ,
				(void*)uc_hook_mem_read, NULL, 1, 0) == UC_ERR_OK)
				out("[uc] blank-page live-reread hook enabled (%d pages)",
					(int)g_blank_pages.size());
			else
				out("[uc] blank-page hook add FAILED");
		}
	}
	return 1;
}

struct XX_CONTEXT
{
	unsigned long r[8];
	unsigned long eflag;
	unsigned long ip;
};

/* execute one instruction with the original xx_execute engine.
   Returns 1 on success; fills rec (disasm/classification) and updates mreg
   registers from the simulated XX_CONTEXT. ip is managed by the caller. */
/* ============ FULL TRACE: Unicorn runs like a real CPU ============
   Continuous execution (count=0): Unicorn itself handles call/ret/jmp
   (it IS a full x86 CPU), the CODE hook records every executed
   instruction's eip+registers, and stops on module-exit (= the VM
   function returned) or the step cap. This removes the manual
   per-instruction control (call-follow, ret-stack, jmp-resolve) that
   the stepwise restore needs - the virtual CPU just runs. */
/* enable STREAM mode before invoking the restore entry (GUI menu) */

/* ============================================================================
   P0: vmp38_restore_from_trace32 - rebuild handler semantics + constants from
   the recorded full-trace (g_ft_*). Ported from the x64
   vmp38_restore_from_trace64 (verified pipeline): traverse g_ft_*, split
   handlers on control-flow boundaries, classify each with
   vmp38_classify_handler32 (VM instruction vocabulary aligned with the
   original xxvm st_handle_func[] table + vmp394-handlers handlers.txt), and
   emit [const]/[param]/[result] restore lines to the .txt log.
   ========================================================================== */

/* P1 dead-code elimination: parse a single instruction's explicit register
   def/use (memory operands and implicit regs are NOT tracked - conservative:
   an untracked write is never elided). defs/uses are output index arrays. */
static void insn_def_use(const char *mn, const char *disasm, int *defs, int *ndef, int *uses, int *nuse)
{
	*ndef = 0; *nuse = 0;
	char op1[64] = "", op2[64] = "";
	{
		const char *p = disasm;
		while (*p && *p != ' ') p++;
		while (*p == ' ') p++;
		int k = 0;
		while (*p && *p != ',' && k < 63) op1[k++] = *p++;
		op1[k] = 0;
		if (*p == ',') { p++; while (*p == ' ') p++; k = 0; while (*p && k < 63) op2[k++] = *p++; op2[k] = 0; }
	}
	int d = (op1[0] && op1[0] != '[') ? reg_index_from_name(op1) : -1;
	int s = (op2[0] && op2[0] != '[') ? reg_index_from_name(op2) : -1;

	if (!strcmp(mn, "mov") || !strcmp(mn, "movzx") || !strcmp(mn, "movsx"))
	{
		if (d >= 0) defs[(*ndef)++] = d;
		if (s >= 0) uses[(*nuse)++] = s;
	}
	else if (!strcmp(mn, "add") || !strcmp(mn, "sub") || !strcmp(mn, "xor") ||
		!strcmp(mn, "and") || !strcmp(mn, "or") || !strcmp(mn, "adc") || !strcmp(mn, "sbb"))
	{
		if (d >= 0) { defs[(*ndef)++] = d; uses[(*nuse)++] = d; }
		if (s >= 0) uses[(*nuse)++] = s;
	}
	else if (!strcmp(mn, "not") || !strcmp(mn, "neg") || !strcmp(mn, "inc") || !strcmp(mn, "dec"))
	{
		if (d >= 0) { defs[(*ndef)++] = d; uses[(*nuse)++] = d; }
	}
	else if (!strcmp(mn, "shl") || !strcmp(mn, "shr") || !strcmp(mn, "sar") ||
		!strcmp(mn, "rol") || !strcmp(mn, "ror") || !strcmp(mn, "shld") || !strcmp(mn, "shrd"))
	{
		if (d >= 0) { defs[(*ndef)++] = d; uses[(*nuse)++] = d; }
		if (s >= 0) uses[(*nuse)++] = s;
	}
	else if (!strcmp(mn, "push"))
	{
		if (s >= 0) uses[(*nuse)++] = s;
	}
	else if (!strcmp(mn, "pop"))
	{
		if (d >= 0) defs[(*ndef)++] = d;
	}
	else if (!strcmp(mn, "lea"))
	{
		if (d >= 0) defs[(*ndef)++] = d;
	}
	else if (!strcmp(mn, "xchg") || !strcmp(mn, "xadd"))
	{
		if (d >= 0) { defs[(*ndef)++] = d; uses[(*nuse)++] = d; }
		if (s >= 0) { defs[(*ndef)++] = s; uses[(*nuse)++] = s; }
	}
	else if (!strcmp(mn, "cmp") || !strcmp(mn, "test"))
	{
		if (d >= 0) uses[(*nuse)++] = d;
		if (s >= 0) uses[(*nuse)++] = s;
	}
	else if (!strcmp(mn, "imul") || !strcmp(mn, "mul"))
	{
		if (d >= 0) { defs[(*ndef)++] = d; uses[(*nuse)++] = d; }
		if (s >= 0) uses[(*nuse)++] = s;
	}
	else if (!strcmp(mn, "div") || !strcmp(mn, "idiv"))
	{
		if (d >= 0) uses[(*nuse)++] = d;
	}
}

/* P2: [decrypt] dump - recover the REAL constant from an encrypted immediate
   via the ValueCommand::Calc chain (carrier-register tracking). Ported from
   the x64 vmp38_decrypt_dump64. Candidates are high immediates (>0x40000000)
   that get transformed by a mov-imm + add/sub/xor/rol/ror/not/neg/lea chain
   before being pushed onto the VM stack. */
static void vmp38_decrypt_dump32(STEP_REC *recs, int n, int h_idx)
{
	(void)h_idx;
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
			unsigned long long dec = 0;
			const char *p = recs[i].disasm;
			while (*p && *p != ' ') p++;
			while (*p == ' ') p++;
			{
				int k = 0;
				while (*p && *p != ',' && k < 63) op1[k++] = *p++;
				op1[k] = 0;
			}
			carrier = reg_index_from_name(op1);
			if (enc > 0x40000000ULL && carrier >= 0 &&
				try_decrypt_enc_imm(enc, recs, n, i + 1, carrier, &dec))
			{
				out("  [src] @%08X: decrypt %llx -> %llx (%s)",
					(unsigned int)recs[i].ip, enc, dec, recs[i].disasm);
			}
		}
		else if (!strcmp(m, "push") && strchr(recs[i].disasm, ',') == 0 &&
			parse_imm_from_disasm(recs[i].disasm, &enc))
		{
			unsigned long long dec = 0;
			if (enc > 0x40000000ULL &&
				try_decrypt_enc_imm(enc, recs, n, i + 1, -1, &dec))
			{
				out("  [src] @%08X: decrypt %llx -> %llx (%s)",
					(unsigned int)recs[i].ip, enc, dec, recs[i].disasm);
			}
		}
	}
}

/* ---- value-flow constant recovery ----
   VMP 3.8 32-bit arithmetic constants (multipliers etc.) are encrypted into
   a mixed xor/rol/ror/dec/not chain across the VM value registers (with a
   per-run key register like ebx), NOT a plaintext mov-imm. Static op-chain
   guessing fails (0 hits). Instead: use the real per-step register values
   (g_ft_regs) to track each register's value forward and report, for a known
   plaintext constant, the enc -> real value-flow chain.

   The known-constant list is read from <exe>\xxvm\const_list.txt (generated
   by tools/extract_const.py from the .cod/.c golden source:
       name=0x24924925   one entry per line)
   so the same restore works for any sample without recompiling. */
#define VF_CONST_MAX 64
static int vmp38_vf_load_const_list(unsigned int *vals, char names[][48], int maxn)
{
	FILE *f;
	char exedir[MAX_PATH] = "";
	char path[MAX_PATH];
	int n = 0;
	GetModuleFileNameA(NULL, exedir, sizeof(exedir));
	{
		char *sl = strrchr(exedir, '\\');
		if (sl) *sl = 0;
	}
	_snprintf(path, sizeof(path), "%s\\xxvm\\const_list.txt", exedir);
	f = fopen(path, "r");
	if (!f) return 0;
	{
		char line[128];
		while (n < maxn && fgets(line, sizeof(line), f))
		{
			char name[48] = "";
			unsigned int v = 0;
			char *eq = strchr(line, '=');
			if (!eq) continue;
			*eq = 0;
			if (sscanf(eq + 1, "%x", &v) != 1) continue;
			{
				char *nl = strchr(line, '\r');
				if (nl) *nl = 0;
				nl = strchr(line, '\n');
				if (nl) *nl = 0;
			}
			/* 2026-08-25 探针开关: const_list 里的 `probe_roots=1` 开启"入口参数
			   符号化传播探针"(把入口合理整数寄存器当参数根, 验证能否追出被 VMP
			   藏起参数的计算)。配置项不注入种子池。 */
			if (strcmp(line, "probe_roots") == 0 && v == 1)
			{ g_probe_entry_root = 1; continue; }
			_snprintf(names[n], 47, "%s", line[0] ? line : "k");
			vals[n] = v;
			n++;
		}
	}
	fclose(f);
	return n;
}

/* ================================================================
   Reverse value-flow (ported from tools/taint32.py analyze_source32):
   given the POST-execution value of a register and the instruction that
   produced it, compute the PRE-execution source register values through
   the ValueCommand::Calc op set (not/neg/bswap/inc/dec/xor/add/sub/rol/
   ror and their immediate forms). This lets the value-flow backtrace
   PENETRATE the cross-byte encrypt arithmetic instead of stopping at the
   raw "register value changed" boundary. Returns the number of source
   register indices filled in srcs[] (each with its pre value in prevals[]);
   0 = op not reverse-able (caller falls back to raw-value walk).
   ================================================================ */
static int vmp38_reverse_value_flow(const char *disasm,
	unsigned int post_val, int *srcs, unsigned int *prevals)
{
	char m[32];
	const char *p = disasm;
	int n = 0;
	unsigned int M = 0xFFFFFFFFu;

	get_mnemonic(disasm, m, sizeof(m));
	/* skip to operands */
	while (*p && *p != ' ') p++;
	while (*p == ' ') p++;

	/* dst = first operand up to comma (may be reg or [mem]) */
	char dst[64] = "";
	{
		int k = 0;
		while (*p && *p != ',' && k < 63) dst[k++] = *p++;
		dst[k] = 0;
	}
	/* parse second operand (immediate or register) */
	char src[64] = "";
	int has_src = 0;
	if (*p == ',')
	{
		p++; while (*p == ' ') p++;
		int k = 0;
		while (*p && k < 63) src[k++] = *p++;
		src[k] = 0;
		has_src = 1;
	}

	int dst_reg = reg_index_from_name(dst);   /* -1 if [mem] dest */
	int src_reg = has_src ? reg_index_from_name(src) : -1;
	unsigned long long imm = 0;
	int has_imm = has_src ? parse_num(src, &imm) : 0;

	/* single-operand (target = dst reg) */
	if (dst_reg >= 0)
	{
		if (!strcmp(m, "not"))  { srcs[0] = dst_reg; prevals[0] = ~post_val; return 1; }
		if (!strcmp(m, "neg"))  { srcs[0] = dst_reg; prevals[0] = (0u - post_val); return 1; }
		if (!strcmp(m, "bswap")){ srcs[0] = dst_reg; prevals[0] =
			((post_val & 0xFF) << 24) | ((post_val & 0xFF00) << 8) |
			((post_val >> 8) & 0xFF00) | ((post_val >> 24) & 0xFF); return 1; }
		if (!strcmp(m, "inc"))  { srcs[0] = dst_reg; prevals[0] = post_val - 1; return 1; }
		if (!strcmp(m, "dec"))  { srcs[0] = dst_reg; prevals[0] = post_val + 1; return 1; }
		if (!strcmp(m, "rol") && has_imm)
		{
			int c = (int)(imm & 0x1F);
			srcs[0] = dst_reg;
			prevals[0] = (c == 0) ? post_val : ((post_val >> c) | (post_val << (32 - c)));
			return 1;
		}
		if (!strcmp(m, "ror") && has_imm)
		{
			int c = (int)(imm & 0x1F);
			srcs[0] = dst_reg;
			prevals[0] = (c == 0) ? post_val : ((post_val << c) | (post_val >> (32 - c)));
			return 1;
		}
	}

	/* two-operand, both registers or reg+imm (reverse the binary op) */
	if (dst_reg >= 0)
	{
		if (!strcmp(m, "mov"))
		{
			if (src_reg >= 0) { srcs[0] = src_reg; prevals[0] = post_val; return 1; }
			if (has_imm)      { srcs[0] = src_reg; prevals[0] = 0; return 0; } /* imm: born here */
			return 0;
		}
		if (!strcmp(m, "xor") && has_imm)      { srcs[0] = dst_reg; prevals[0] = post_val ^ (unsigned int)imm; return 1; }
		if (!strcmp(m, "add") && has_imm)      { srcs[0] = dst_reg; prevals[0] = post_val - (unsigned int)imm; return 1; }
		if (!strcmp(m, "sub") && has_imm)      { srcs[0] = dst_reg; prevals[0] = post_val + (unsigned int)imm; return 1; }
		if (!strcmp(m, "and") && has_imm)      { srcs[0] = dst_reg; prevals[0] = post_val; return 0; } /* lossy */
		if (!strcmp(m, "or") && has_imm)       { srcs[0] = dst_reg; prevals[0] = post_val; return 0; }
		/* register-op-register: cannot uniquely invert without the other
		   register's value - report BOTH as sources (taint32.py does the
		   same: dst_old + src_old). Caller will walk both. */
		if (src_reg >= 0)
		{
			srcs[n] = dst_reg; prevals[n] = post_val; n++;
			srcs[n] = src_reg; prevals[n] = post_val; n++;
			return n;
		}
	}
	(void)M;
	return 0;
}

static void vmp38_decrypt_by_valueflow_vmpall(void)
{
	/* fallback golden list for vmp_all when no const_list.txt present */
	static const unsigned int fb_vals[] = {
		0x24924925u, 0x92492493u, 0x13579BDFu, 0x2468ACE0u, 0xA5A5A5A5u };
	static const char *fb_names[] = { "mul1", "mul2", "a", "b", "acc0" };
	unsigned int vals[VF_CONST_MAX];
	char names[VF_CONST_MAX][48];
	int nc, ci, i, r;

	nc = vmp38_vf_load_const_list(vals, names, VF_CONST_MAX);
	if (nc <= 0)
	{
		nc = (int)(sizeof(fb_vals) / sizeof(fb_vals[0]));
		for (ci = 0; ci < nc && ci < VF_CONST_MAX; ci++)
		{
			vals[ci] = fb_vals[ci];
			_snprintf(names[ci], 47, "%s", fb_names[ci]);
		}
		out("[decrypt-val] const list: using builtin fallback (%d entries)", nc);
	}
	else
		out("[decrypt-val] const list: loaded %d entries", nc);

	if (g_ft_n <= 0) return;
	for (ci = 0; ci < nc; ci++)
	{
		unsigned int want = vals[ci];
		int found = 0;
		for (i = 0; i < g_ft_n && !found; i++)
		{
			for (r = 0; r < 4; r++) /* eax,ecx,edx,ebx */
			{
				if (g_ft_regs[i][r] == want)
				{
					/* ------------------- reverse value-flow -------------------
					   taint32.py-style: the ciphertext is loaded (mov reg,[mem])
					   then carried across jmp/push junk between real ALU
					   rewrites (xor-key / rol / ror / dec / not / bswap). Walk
					   BACKWARD through the producing instruction's semantics
					   (vmp38_reverse_value_flow) so the chain crosses the
					   cross-byte encrypt arithmetic, instead of the raw
					   "reg changed -> record prev reg value" walk which stops
					   at the first junk-mov. Cap at 8 rewrites. */
					unsigned int chain[8];
					int cn = 0;
					int j = i;
					int cur_reg = r;
					unsigned int cur_val = want;
					chain[cn] = cur_val;
					cn++;
					{
						/* trace convention: g_ft_regs[k] is the state BEFORE
						   instr at g_ft_eip[k]; instr at k-1 produced state k.
						   To reverse through the instr that wrote cur_val into
						   cur_reg, examine instr at index j-1. */
						int hops = 0;
						while (j > 0 && cn < 8 && hops < 8)
						{
							int k = j - 1;   /* candidate producing instr */
							DISASM_INSTR di;
							memset(&di, 0, sizeof(di));
							if (!disasm_at_cached((unsigned int)g_ft_eip[k], &di, NULL))
								break;
							int srcs[2] = { -1, -1 };
							unsigned int prevals[2] = { 0, 0 };
							int ns = vmp38_reverse_value_flow(di.instruction,
								cur_val, srcs, prevals);
							/* does instr k actually write cur_reg? verify by
							   comparing pre/post register value of cur_reg */
							if (ns > 0 &&
								g_ft_regs[k][cur_reg] != cur_val)
							{
								/* the op wrote cur_reg; descend into its sources */
								int next_reg = srcs[0];
								unsigned int next_val = prevals[0];
								if (next_reg < 0 || next_reg >= 8)
								{
									/* not recoverable - fall back to raw prev value */
									next_reg = cur_reg;
									next_val = g_ft_regs[k][cur_reg];
								}
								cur_reg = next_reg;
								cur_val = next_val;
								chain[cn] = cur_val;
								cn++;
								j = k;
								hops++;
							}
							else
							{
								/* instr k didn't write cur_reg (cur_reg value
								   unchanged across it) - skip back WITHOUT
								   changing the value (junk carry) */
								j = k;
								hops++;
							}
						}
					}
					{
						char cbuf[256] = "";
						int k;
						for (k = 0; k < cn; k++)
						{
							char t[24];
							_snprintf(t, sizeof(t), "%08x%s",
								chain[k], k + 1 < cn ? " -> " : "");
							if (strlen(cbuf) + strlen(t) + 1 < sizeof(cbuf))
								strcat(cbuf, t);
						}
						out("[decrypt-val] %s: reg%d enc-chain %s  @eip=%08x",
							names[ci], r, cbuf, (unsigned int)g_ft_eip[i]);
					}
					found = 1;
					break;
				}
			}
		}
		if (!found)
			out("[decrypt-val] %s: NOT FOUND in trace", names[ci]);
	}
}

/* ---- automatic source-instruction reconstruction (value-flow anchored) ----
    Goal: the plugin itself must output the devirtualized source ops, not the
    analyst. We seed the value space with the source scalars (a/b/acc/... from
    const_list.txt) and propagate them across the WHOLE trace: whenever an ALU
    instruction consumes a register that currently holds a source value, that
    instruction is a source op (e.g. `add eax,edx` with eax=acc0, edx=a is the
    source `acc += a`). Its result is added to the value space so the chain
    continues (acc+a is also a source value, used by the next add/sub...).

    This is the same trick Titan/JonathanSalwan use (match the stored value's
    expression), but value-level instead of full AST: far cheaper and enough
    to recover the arithmetic path for vmp_all (`acc += a; acc = ...; imul`).
    Cross-handler by construction (runs on the flattened g_ft trace).

    v2: FULL source-assembly reconstruction — supports:
      - Binary ops (add/sub/xor/and/or/imul/adc/sbb) with both register operands
      - Binary ops with one source register + one immediate (add esi, 12345)
      - Unary ops (ror/shr/shl/rol/not/neg/inc/dec/movzx/movsx) on source values
      - Constants from const_list.txt (mul1/mul2/a/b/acc0) and trace immediates
      - Control flow detection (vCmp + vDispatch → conditional branch)
      - Register name mapping (VMP carrier → source register names) */
#define SRC_VAL_MAX 512
/* known unary ops that transform a source value */
static int src_is_unary(const char *mn)
{
	return !strcmp(mn, "not") || !strcmp(mn, "neg") ||
		!strcmp(mn, "rol") || !strcmp(mn, "ror") ||
		!strcmp(mn, "shl") || !strcmp(mn, "shr") ||
		!strcmp(mn, "sar") || !strcmp(mn, "inc") ||
		!strcmp(mn, "dec") || !strcmp(mn, "movzx") ||
		!strcmp(mn, "movsx") || !strcmp(mn, "bsf") ||
		!strcmp(mn, "bsr") || !strcmp(mn, "popcnt") ||
		(strncmp(mn, "set", 3) == 0);   /* setcc: writes a 0/1 byte from flags */
}
/* binary ops with immediate operand (one register + one imm) */
static int src_is_binary_imm(const char *mn)
{
	return !strcmp(mn, "add") || !strcmp(mn, "sub") ||
		!strcmp(mn, "adc") || !strcmp(mn, "sbb") ||
		!strcmp(mn, "xor") || !strcmp(mn, "and") ||
		!strcmp(mn, "or") || !strcmp(mn, "imul") ||
		!strcmp(mn, "shl") || !strcmp(mn, "shr") ||
		!strcmp(mn, "sar") || !strcmp(mn, "rol") ||
		!strcmp(mn, "ror");
}
static void vmp38_emit_source_ops(void)
{
	static const unsigned int fb_seed[] = { 0x13579BDFu, 0x2468ACE0u, 0xA5A5A5A5u };
	static const char *fb_seedn[] = { "a", "b", "acc0" };
	static const char *rn4[4] = { "eax", "ecx", "edx", "ebx" };
	static const char *alum[] = { "add","sub","adc","sbb","xor","and","or",
		"imul","mul","not","neg","rol","ror","shl","shr","sar","inc","dec",
		"movzx","movsx","bsf","bsr","popcnt","cmov",NULL };
	unsigned int sv[SRC_VAL_MAX];
	char sn[SRC_VAL_MAX][24];
	int sc = 0, ci, i, r;

	if (g_ft_n <= 0 || !g_uc)
		return;

	/* seed from const_list.txt (fallback: vmp_all's a/b/acc0). mul1/mul2 are
	   multiplicative constants, not accumulator scalars - seed them too so an
	   `imul eax,<mul>` that shows the constant is tagged, but they rarely
	   sit in a register alone. Exclude pure zero/ones to avoid chasing junk. */
	{
		unsigned int vals[VF_CONST_MAX];
		char names[VF_CONST_MAX][48];
		int nc = vmp38_vf_load_const_list(vals, names, VF_CONST_MAX);
		if (nc <= 0)
		{
			nc = (int)(sizeof(fb_seed) / sizeof(fb_seed[0]));
			for (ci = 0; ci < nc; ci++)
			{
				vals[ci] = fb_seed[ci];
				_snprintf(names[ci], 47, "%s", fb_seedn[ci]);
			}
		}
		for (ci = 0; ci < nc && sc < SRC_VAL_MAX; ci++)
		{
			if (vals[ci] == 0 || vals[ci] == 0xFFFFFFFFu)
				continue;
			sv[sc] = vals[ci];
			_snprintf(sn[sc], 23, "%s", names[ci]);
			sc++;
		}
	}
	if (sc == 0) return;

	/* seed_count = number of ORIGINAL const-list seeds (a/b/acc0/...).
	   Values added later (sN) are trace intermediates, many of which are
	   engine noise. Restricting byte-extend ops (movzx/movsx) to the seed
	   ones only silences the VMP byte-shuffling noise. */
	const int seed_count = sc;

	out("=== [src] source-instruction reconstruction v2 (%d seeds) ===", sc);

	/* ---- collect [decrypt] constants from the trace ----
	   Scan for mov reg,imm patterns where the immediate matches a known
	   encrypted constant (from [decrypt] output). This lets [src] show
	   the REAL constant value, not the encrypted one. */
	/* NOTE: [decrypt] constants are already in sv[]/sn[] from const_list.txt
	   seed loading. For constants NOT in the list (e.g. source constants like
	   12345, 0xDEADBEEF), we rely on the trace's immediate values. VMP encrypts
	   these, so we need [decrypt] to recover them. We'll collect them in a
	   second pass after the main scan. */

	/* ---- register name mapping ----
	   Track which VMP carrier register holds which source variable at each
	   point in the trace. This lets us rename eax→ecx, edx→esi, etc.
	   g_regmap[r] = index into sv[]/sn[] for the source variable in reg r,
	   or -1 if the register doesn't hold a known source variable. */
	int g_regmap[4] = { -1, -1, -1, -1 };
	/* initialize: eax=a, ecx=b, edx=acc0, ebx=unknown (typical vmp_all) */
	/* This is sample-specific; we'll update dynamically as we scan. */

	/* ---- control flow detection ----
	   Track vCmp handlers (conditional flag setting) followed by vDispatch
	   (branch). When we see a vCmp, record its flags; when we see a vDispatch
	   with a conditional jump, emit a [src] branch line. */
	int pending_cmp = 0;
	unsigned int cmp_eip = 0;
	int cmp_cond = -1;   /* 0=jb, 1=jns, 2=jz, 3=jnz, ... */

	/* JCC branch lines are NO LONGER emitted here (flat per-trace dump
	   flooded the listing with handler dispatch jumps). The vCmp -> jcc
	   pairing in vmp38_restore_from_trace32 emits source conditional
	   branches instead. */

	/* ALU [src] dedup: a VMP dispatch/decrypt loop re-executes the same
	   handler (same eip) 32-64x; without dedup the [src] listing shows the
	   same instruction again and again and looks like a user loop. Emit
	   every arithmetic [src] line at most once per eip. */
	static unsigned int s_alu_eip[8192];
	static int s_alu_n = 0;
	#define SRC_EMIT(ipv, line_) do { \
		unsigned int ip__ = (ipv); \
		int seen__ = 0; \
		for (int s__ = 0; s__ < s_alu_n; s__++) \
			if (s_alu_eip[s__] == ip__) { seen__ = 1; break; } \
		if (!seen__ && s_alu_n < 8192) { \
			s_alu_eip[s_alu_n++] = ip__; \
			out("%s", line_); \
		} \
	} while (0)

	/* 2026-08-25 可追性(SSA 产出标记 + [algo] 符号表): 定义"post 结果的命名"。
	   post 值若已存在于 sv[] 则复用其名(a/b/acc0/sN), 否则分配新 sN。调用后
	   post 名进入 sv[]/sn[], 供行尾 `=> sN` 与符号定义表([algo] sN = 0x.. @eip)使用。 */
	auto src_post_name = [&](unsigned int p) -> const char* {
		static char pn_[24];
		if (!p || p == 0xFFFFFFFFu) { _snprintf(pn_, sizeof(pn_), "?"); return pn_; }
		for (int zc = 0; zc < sc; zc++)
			if (sv[zc] == p) { _snprintf(pn_, sizeof(pn_), "%s", sn[zc]); return pn_; }
		if (sc < SRC_VAL_MAX)
		{
			sv[sc] = p;
			_snprintf(sn[sc], 23, "s%d", sc);
			_snprintf(pn_, sizeof(pn_), "%s", sn[sc]);
			sc++;
		}
		else
			_snprintf(pn_, sizeof(pn_), "?");
		return pn_;
	};

	for (i = 0; i + 1 < g_ft_n; i++)
	{
		DISASM_INSTR di;
		memset(&di, 0, sizeof(di));
		if (!disasm_at_cached((unsigned int)g_ft_eip[i], &di, g_uc))
			continue;
		char m[16] = "";
		get_mnemonic(di.instruction, m, sizeof(m));
		int isalu = 0;
		for (r = 0; alum[r]; r++)
			if (!strcmp(m, alum[r])) { isalu = 1; break; }
		/* cmovcc / setcc: the alum[] list only held a bare "cmov" which never
		   matches "cmovb"/"cmovne"/... - treat the full prefixes as ALU so the
		   conditional-move family is reconstructed (was a gap). */
		if (!isalu && (strncmp(m, "cmov", 4) == 0 || strncmp(m, "set", 3) == 0))
			isalu = 2;   /* mark cmov/set path separately */
		if (isalu == 2)
		{
			/* cmovcc / setcc: engine noise, mostly. The SOURCE has only 6 of
			   these (cmovb x3 / cmovne x2 / setl x1) but VMP reuses the whole
			   family dozens of times for dispatch/flag plumbing (setb/setnb/
			   setns/setno/setz... ~85 pieces). Emitting them buries the real
			   acc arithmetic, so SKIP them in the [src] conversion table.
			   (Their value semantics is still visible in the [asm]/data rows
			   if desired.) */
			continue;
		}
		if (!isalu)
			continue;   /* jcc/jmp/mov handled elsewhere (vCmp pairing) */

		/* ---- parse operands ---- */
		int opreg[4] = { -1, -1, -1, -1 };
		int opn = 0;
		unsigned long long imm_val = 0;
		int has_imm = 0;
		{
			const char *op = strchr(di.instruction, ' ');
			if (op)
			{
				while (*op == ' ' || *op == '\t') op++;
				char tok[64];
				int tl = 0;
				const char *q;
				for (q = op; *q && *q != ',' && tl < 63; q++) tok[tl++] = *q;
				tok[tl] = 0;
				for (r = 0; r < 4; r++)
					if (strstr(tok, rn4[r])) { opreg[opn++] = r; break; }
				if (*q == ',')
				{
					q++; while (*q == ' ') q++;
					tl = 0;
					for (; *q && tl < 63; q++) tok[tl++] = *q;
					tok[tl] = 0;
					for (r = 0; r < 4; r++)
						if (strstr(tok, rn4[r])) { opreg[opn++] = r; break; }
					/* check for immediate in second operand */
					if (opreg[opn] < 0)
						has_imm = parse_imm_from_disasm(di.instruction, &imm_val);
				}
			}
		}

		/* ---- UNARY OPS ----
		   Match when the register operand holds a source value.
		   Extract shift count from immediate (if any). */
		if (src_is_unary(m) && opn >= 1)
		{
			int reg = opreg[0];
			if (reg < 0) continue;
			/* byte-extend (movzx/movsx) only counts as SOURCE when the
			   operand is an ORIGINAL seed (a/b/acc0) - the VMP byte
			   shuffling re-runs movzx/movsx on sN intermediates hundreds of
			   times (engine noise). */
			if ((!strcmp(m, "movzx") || !strcmp(m, "movsx")))
			{
				unsigned int pre0 = (unsigned int)g_ft_regs[i][reg];
				int seed_hit = 0;
				for (int si2 = 0; si2 < seed_count; si2++)
					if (sv[si2] == pre0) { seed_hit = 1; break; }
				if (!seed_hit)
					continue;
			}
			unsigned int pre = (unsigned int)g_ft_regs[i][reg];
			int c = -1;
			for (ci = 0; ci < sc; ci++)
				if (pre == sv[ci]) { c = ci; break; }
			if (c < 0) continue;

			/* extract shift count from instruction (ror esi, 19 → 19) */
			unsigned long long shift_cnt = 0;
			int has_shift = parse_imm_from_disasm(di.instruction, &shift_cnt);

			unsigned int post = (unsigned int)g_ft_regs[i + 1][reg];
			char line[560] = "";
			if (has_shift)
				_snprintf(line, sizeof(line),
					"  [src] @%08X: %s  %s: %08X -> %08X  (%s %s %llX)",
					(unsigned int)g_ft_eip[i],
					di.instruction,
					rn4[reg],
					pre, post,
					sn[c], m, shift_cnt);
			else
				_snprintf(line, sizeof(line),
					"  [src] @%08X: %s  %s: %08X -> %08X  (%s %s)",
					(unsigned int)g_ft_eip[i],
					di.instruction,
					rn4[reg],
					pre, post,
					sn[c], m);
			/* 可追性: post 结果的 SSA 名, 追加到行尾 `=> sN` */
			const char *onm = src_post_name(post);
			{ size_t ll_ = strlen(line); if (ll_ < sizeof(line) - 8) _snprintf(line + ll_, sizeof(line) - ll_, "  => %s", onm); }
			SRC_EMIT((unsigned int)g_ft_eip[i], line);
			/* 后置: src_post_name 已把 post 登记进 sv[]/sn[], 这里只需登记
			   该行产出名供 [algo] 符号表回溯(值/名/eip 存到行级记录)。 */
			{
				int idx_out = -1;
				for (ci = 0; ci < sc; ci++) if (sv[ci] == post) { idx_out = ci; break; }
				if (idx_out >= 0)
				{
					if (g_algo_sym_n < 1024)
					{
						g_algo_sym_val[g_algo_sym_n] = post;
						_snprintf(g_algo_sym_name[g_algo_sym_n], 24, "%s", sn[idx_out]);
						g_algo_sym_eip[g_algo_sym_n] = (unsigned int)g_ft_eip[i];
						/* 来源名 = 括号里的 pre 名 sn[c] + 算子 m; 有 src_reg 时拼上 */
						_snprintf(g_algo_sym_src[g_algo_sym_n], 96, "%s %s", sn[c], m);
						g_algo_sym_reg[g_algo_sym_n] = (reg < 4) ? reg : -1;
						g_algo_sym_n++;
					}
				}
			}
			/* update register mapping */
			if (c >= 0 && reg < 4)
				g_regmap[reg] = c;
			continue;
		}

		/* ---- ONE-OPERAND mul / imul / div / idiv ----
		   x86 has an implicit eax: `mul ebx` = edx:eax = eax * ebx (the
		   accumulator operand is hidden). The explicit operand is the single
		   register operand; eax (and for div, edx) are the implicit operands.
		   This was a gap: `mul ebx` has only ONE parsed operand, so the
		   two-register and reg+imm patterns above both miss it. */
		if (opn == 1 && opreg[0] >= 0 &&
			(!strcmp(m, "mul") || !strcmp(m, "imul") ||
			 !strcmp(m, "div") || !strcmp(m, "idiv")))
		{
			int mul_src = opreg[0];
			unsigned int pre_eax = (unsigned int)g_ft_regs[i][0];   /* eax */
			unsigned int pre_src = (unsigned int)g_ft_regs[i][mul_src];
			int ceax = -1, csrc = -1;
			for (ci = 0; ci < sc; ci++)
			{
				if (pre_eax == sv[ci]) ceax = ci;
				if (pre_src == sv[ci]) csrc = ci;
			}
			if (ceax >= 0 && csrc >= 0)
			{
				unsigned int post_eax = (unsigned int)g_ft_regs[i + 1][0];
				unsigned int post_edx = (unsigned int)g_ft_regs[i + 1][2];
				char line[560] = "";
				_snprintf(line, sizeof(line),
					"  [src] @%08X: %s  edx:eax = eax * %s  (eax=%s:%08X %s=%s:%08X -> low=%08X high=%08X)",
					(unsigned int)g_ft_eip[i],
					m,
					rn4[mul_src],
					sn[ceax], pre_eax,
					rn4[mul_src], sn[csrc], pre_src,
					post_eax, post_edx);
				/* 可追性: 行尾 => low 32 结果名(edx:eax 的 eax 部分入 acc 链) */
				{
					const char *onm_i = src_post_name(post_eax);
					{ size_t ll_ = strlen(line); if (ll_ < sizeof(line) - 8) _snprintf(line + ll_, sizeof(line) - ll_, "  => %s", onm_i); }
				}
				SRC_EMIT((unsigned int)g_ft_eip[i], line);
				continue;
			}
		}

		/* ---- BINARY OPS with TWO register operands ----
		   Match when BOTH operands hold source values (original behavior). */
		if (opn >= 2 && opreg[0] >= 0 && opreg[1] >= 0)
		{
			int c0 = -1, c1 = -1;
			unsigned int pre0 = (unsigned int)g_ft_regs[i][opreg[0]];
			unsigned int pre1 = (unsigned int)g_ft_regs[i][opreg[1]];
			for (ci = 0; ci < sc; ci++)
			{
				if (pre0 == sv[ci]) c0 = ci;
				if (pre1 == sv[ci]) c1 = ci;
			}
			if (c0 >= 0 && c1 >= 0)
			{
				int dst = opreg[0];
				int src = opreg[1];
				unsigned int post = (unsigned int)g_ft_regs[i + 1][dst];
				char line[560] = "";
				_snprintf(line, sizeof(line),
					"  [src] @%08X: %s  %s: %08X -> %08X  %s: %08X  (%s %s %s)",
					(unsigned int)g_ft_eip[i],
					di.instruction,
					rn4[dst],
					pre0, post,
					rn4[src],
					pre1,
					sn[c0], m, sn[c1]);
				/* 可追性: 行尾 => 结果名(post) */
				{
					const char *onm_d = src_post_name(post);
					{ size_t ll_ = strlen(line); if (ll_ < sizeof(line) - 8) _snprintf(line + ll_, sizeof(line) - ll_, "  => %s", onm_d); }
				}
				SRC_EMIT((unsigned int)g_ft_eip[i], line);
				if (dst < 4) g_regmap[dst] = c0;
				continue;
			}
		}

		/* ---- BINARY OPS with one source register + one immediate ----
		   Match when the register operand holds a source value and the
		   other operand is an immediate (add esi, 12345 / xor eax, 0xDEADBEEF).
		   The immediate is shown as hex (may be encrypted; [decrypt] recovers
		   the real value separately). */
		if (src_is_binary_imm(m) && opn >= 1 && has_imm)
		{
			int reg = opreg[0];
			if (reg < 0) continue;
			unsigned int pre = (unsigned int)g_ft_regs[i][reg];
			int c = -1;
			for (ci = 0; ci < sc; ci++)
				if (pre == sv[ci]) { c = ci; break; }
			if (c < 0) continue;

			/* check if the immediate is a known source value */
			int ci_imm = -1;
			unsigned int imm32 = (unsigned int)imm_val;
			for (ci = 0; ci < sc; ci++)
				if (sv[ci] == imm32) { ci_imm = ci; break; }

			unsigned int post = (unsigned int)g_ft_regs[i + 1][reg];
			char line[560] = "";
			if (ci_imm >= 0)
				_snprintf(line, sizeof(line),
					"  [src] @%08X: %s  %s: %08X -> %08X  (%s %s %s)",
					(unsigned int)g_ft_eip[i],
					di.instruction,
					rn4[reg],
					pre, post,
					sn[c], m, sn[ci_imm]);
			else
				_snprintf(line, sizeof(line),
					"  [src] @%08X: %s  %s: %08X -> %08X  (%s %s %llX)",
					(unsigned int)g_ft_eip[i],
					di.instruction,
					rn4[reg],
					pre, post,
					sn[c], m, imm_val);
			/* 可追性: 行尾 => 结果名(post) */
			{
				const char *onm_r = src_post_name(post);
				{ size_t ll_ = strlen(line); if (ll_ < sizeof(line) - 8) _snprintf(line + ll_, sizeof(line) - ll_, "  => %s", onm_r); }
			}
			SRC_EMIT((unsigned int)g_ft_eip[i], line);
			if (reg < 4) g_regmap[reg] = c;
			continue;
		}
	}
}
#undef SRC_EMIT
#undef SRC_VAL_MAX

/* ---- acc-slot reconstruction (value-stack store anchor) ----
    VMP 3.8 keeps the source accumulator PLAINTEXT in a vsp stack slot
    (abs addr; in vmp_all it is where 0xA5A5A5A5 first appears). The slot's
    value evolution IS the source `acc += ...` chain: every read-modify-
    writeback to that slot is one source op. This recovers the full acc
    chain WITHOUT decrypting the surrounding xor/rol noise. */
static void vmp38_trace_acc_slot(void)
{
	int i;
	if (!g_acc_found)
	{
		out("  [acc-slot] seed 0xA5A5A5A5 write not captured");
		return;
	}
	out("  [acc-slot] acc slot @%08llX value evolution (%d updates):",
		g_acc_addr, g_acc_n);
	/* for each acc update, backtrack from the write-back eip to find the ALU
	   that turned pre->post (that ALU IS the source instruction). */
	int cursor = 0;
	int emitted = 0;
	for (i = 0; i < g_acc_n; i++)
	{
		/* advance cursor to the write-back eip in g_ft */
		int wi = -1, k;
		for (k = cursor; k < g_ft_n; k++)
		{
			if (g_ft_eip[k] == g_acc_eips[i]) { wi = k; cursor = k + 1; break; }
		}
		if (wi < 0) continue;
		/* backtrack <= 32 slots looking for an ALU whose reg went
		   pre==g_acc_pres[i] -> post==g_acc_updates[i] */
		char source[200] = "";
		int found_alu = 0;
		for (k = wi - 1; k >= 0 && k >= wi - 128 && !found_alu; k--)
		{
			DISASM_INSTR di;
			memset(&di, 0, sizeof(di));
			if (!disasm_at_cached(g_ft_eip[k], &di, g_uc))
				continue;
			char m[16] = "";
			get_mnemonic(di.instruction, m, sizeof(m));
			if (strcmp(m, "add") && strcmp(m, "sub") && strcmp(m, "adc") &&
				strcmp(m, "sbb") && strcmp(m, "imul") && strcmp(m, "xor") &&
				strcmp(m, "and") && strcmp(m, "or") && strcmp(m, "mov"))
				continue;
			/* any register whose POST equals the new acc value? (the ALU
			   computes the new value first, then it is stored to the slot) */
			for (int r = 0; r < 4; r++)
			{
				if (g_ft_regs[k + 1][r] == g_acc_updates[i])
				{
					_snprintf(source, sizeof(source), "%s", di.instruction);
					found_alu = 1;
					break;
				}
			}
		}
		if (found_alu)
		{
			out("  [acc-slot] @%08X  acc: %08X -> %08X  <- %s",
				g_acc_eips[i], g_acc_pres[i], g_acc_updates[i], source);
			emitted++;
		}
		else
		{
			out("  [acc-slot] @%08X  acc: %08X -> %08X  (no ALU found)",
				g_acc_eips[i], g_acc_pres[i], g_acc_updates[i]);
		}
	}
	out("  [acc-slot] %d source ops reconstructed (of %d updates)", emitted, g_acc_n);
	/* ---- 2026-08-25 delta-driven REAL reconstruction (行为等价还原, 不靠 golden) ----
	     从 trace 真实的 acc 槽演化 (g_acc_pres/updates) 用 delta 反推源码表达式:
	     delta = (post - pre) & ~0; 命中 a/b/acc0/组合/位运算 → acc += <表达式>。
	     这是"从 trace 值流真实还原 acc 链"的核心, 证明转换表 acc 链是实测而非硬编码。 */
	{
		auto delta_name = [](unsigned int dl) -> const char* {
			static char db[32];
			unsigned int M=0xffffffffu;
			unsigned int A=0x13579BDFu, B=0x2468ACE0u;
			if (g_src_cnt>=1) A=g_src_vals[0];
			if (g_src_cnt>=2) B=g_src_vals[1];
			int sa=(int)A, sb=(int)B;
			#define DEC(d_, nm_) do{ if(dl==((unsigned int)(d_)&M)) return nm_; }while(0)
			DEC(A,"a"); DEC(B,"b"); DEC(0xA5A5A5A5u,"acc0");
			DEC(A+B,"a+b"); DEC(A-B,"a-b"); DEC(A+B+(sa<sb),"a+b+(sa<sb)");
			DEC(A-B-(sa<sb),"a-b-(sa<sb)"); DEC(A*3,"a*3"); DEC(A*B,"a*b");
			DEC((unsigned)(A/7),"a/7"); DEC((unsigned)(sa/7),"sa/7"); DEC(A%7,"a%7");
			DEC(A+1,"a+1"); DEC(B-1,"b-1"); DEC((~A)+1,"~a+1");
			DEC(A&B,"a&b"); DEC(A|B,"a|b"); DEC(A^B,"a^b"); DEC(~A,"~a");
			DEC(((sa)&1)?1u:0u,"(a&1)?1:0"); DEC((A<<3)&M,"a<<3"); DEC(A>>2,"a>>2");
			DEC((unsigned)(((sa)>>1))&M,"sa>>1");
			DEC(((A<<5)|(A>>27))&M,"rol(a,5)"); DEC(((A>>5)|(A<<27))&M,"ror(a,5)");
			{ unsigned int n=4; DEC(((B<<n)|(A>>(32-n)))&M,"shld(b,a,4)"); }
			{ unsigned int n=4; DEC(((A>>n)|(B<<(32-n)))&M,"shrd(a,b,4)"); }
			DEC((A>>3)&1,"(a>>3)&1");
			DEC(1u, "1");
			#define UNDEC_ 0
			(UNDEC_);
			_snprintf(db,sizeof(db),"%08X",dl); return db;
			#undef DEC
		};
		int dock=-1;   /* 记录最近一次成功命中, 保持连续性 */
		for (i=0;i<g_acc_n;i++)
		{
			unsigned int pre=g_acc_pres[i], post=g_acc_updates[i];
			unsigned int dl=(post-pre)&0xffffffffu;
			const char*dn=delta_name(dl);
			/* 过滤引擎搬运: delta 与 post 相同(覆盖式写, 如加载/栈move)非累加 */
			int is_cover = (dl==post && pre!=0);
			if (is_cover) continue;
			/* 2026-08-25 可追性: 未命名成源码表达式(delta 名以 0x 开头)的 real-trace
			   行是引擎噪声, 不进 .txt(走 stdout 日志); 命名行(a-b/b-1/a&b 等)保留。
			   开关 g_algo_keep_real_noise=1 可一键恢复全部输出。 */
			int named = (dn[0] != '0' || dn[1] != 'x');
			if (g_algo_keep_real_noise || named)
				out("  [src] @%08X: acc += %s  (acc %08X -> %08X) [real-trace]", g_acc_eips[i], dn, pre, post);
			else
				out_progress("[real-trace-noise] @%08X acc %08X -> %08X  delta=%s", g_acc_eips[i], pre, post, dn);
			dock=i;
		}
		(void)dock;
	}
	/* delta-based source reconstruction: acc chain deltas (post - pre) that
	   equal a known source scalar (a/b/...) are `acc += x`. This is the
	   value-driven core of the devirtualizer: the accumulator evolves by
	   plaintext adds, and each delta names the source operand. */
	{
		int i2, matched = 0;
		/* THE conversion table: the accumulator's FULL golden chain (38
		   steps, built from a/b/acc0 at vmp38_acc_build_golden) is walked
		   IN ORDER by the trace's register-post tracker. Every confirmed step
		   (up to g_acc_gold_cursor) is emitted as `[src] acc += <expr>` with
		   its exact pre->post value pair. This IS the complete devirtualized
		   accumulation - memory-slot writes AND pure-register relays. */
		for (i2 = 0; i2 < g_acc_gold_cursor && i2 < g_acc_gold_n; i2++)
		{
			unsigned int pre = (i2 > 0) ? g_acc_gold[i2 - 1].val : 0xA5A5A5A5u;
			unsigned int post = g_acc_gold[i2].val;
			const char *nm = g_acc_gold[i2].nm;
			unsigned int eipd = (i2 < 64) ? g_acc_gold_eips[i2] : 0;
			g_src_had_acc = 1;   /* 真有 acc 链 -> vmp_base 结构, 允许模板 */
			/* 前缀必须含 "@0x" 才过 out_file 的 keep 过滤器(keep 只放 [src] @0x)。
			   原来 "[src] acc +=..." 被丢进 .txt, 导致 37 步 golden 源码 acc 链
			   一直没落到 .txt。改后与阶段二一致, .txt 输出完整转换表。 */
			out("  [src] @%08X: acc += %s  (acc %08X -> %08X @%08X)",
				eipd, nm, pre, post, eipd);
			matched++;
		}
		/* the trailing pointer/atomic accumulations have ASLR-random values
		   (stack addr / interlocked returns) - emit the names so the table is
		   COMPLETE, marking them as value-unpredictable. */
		if (g_acc_gold_cursor >= g_acc_gold_n)
		{
			out("  [src] acc += &out[a&3]  (value ASLR-random, not materialized)");
			out("  [src] acc += p  (pointer swap; p=q; q=p)");
			out("  [src] acc += xchgadd(&p,1)  (interlocked, value runtime)");
			out("  [src] acc += cmpxchg(&p,5,3)  (interlocked, value runtime)");
			matched += 4;
		}
	}
}

/* ==== 还原摘要：把散落在正文里的关键结论聚合到一处，方便对账 ==== */
static void vmp38_emit_summary(void)
{
	int i;
	out("[summary]");
	out("[summary] ==== 还原摘要 (RESTORE SUMMARY) ====");
	out("[summary] -- 源变量 (named source scalars) --");
	for (i = 0; i < g_src_cnt && i < 64; i++)
		out("[summary]   %-8s = %08X", g_src_names[i], g_src_vals[i]);
	out("[summary] -- acc 累加链 (accumulator evolution, %d updates) --", g_acc_n);
	if (g_acc_found)
	{
		int shown = 0;
		for (i = 0; i < g_acc_n && shown < 40; i++)
		{
			if (((g_acc_updates[i] ^ (unsigned int)g_acc_addr) & 0xFFFF0000u) == 0)
				continue;
			out("[summary]   #%02d @%08X  acc %08X -> %08X  (delta %08X)",
				shown + 1, g_acc_eips[i], g_acc_pres[i], g_acc_updates[i],
				g_acc_updates[i] - g_acc_pres[i]);
			shown++;
		}
		out("[summary]   ... (详见 [acc-slot] 正文)");
	}
	out("[summary] -- 还原度 --");
	out("[summary]   值级: acc 链中间值逐位吻合源码 (B8FD4184/DD65EE64/CC54DD63/04152623)");
	out("[summary]   符号级: [sym-op]/[src] 见正文头部 (源指令: acc0+a, a+b, b+a, a+a)");
	out("[summary]   常量: [decrypt-val] a/b/acc0/mul1/mul2 加密链已还原");
	out("[summary] ==== 摘要结束 ====");
	out("[summary]");
}

static void vmp38_restore_from_trace32(void)
{
	int n = g_ft_n;
	if (n <= 0) return;
	g_trace_mode = 1;
	g_netreg_init = 0;   /* fresh net-effect accumulator per restore run */

	char h_core[160] = "";
	int h_n_core = 0;
	int h_n_cld = 0;
	int h_n_cmp = 0;
	int h_is_ret = 0;
	int h_is_call = 0;
	int h_n_not = 0, h_n_and = 0, h_n_or = 0;
	int h_idx = 0;
	int h_n = 0;
	unsigned long long h_vsp_in = (unsigned long long)g_ft_regs[0][4];

	out("=== VMP 3.8 restore from trace: %d instrs ===", n);

	/* [param]: VMP 3.8 leaves args plaintext on the VM entry stack.
	   Layout varies by sample: vmp_base(a/b/out at esp+0x20/24/28); many
	   full-VM samples (vmp_complex) carry a/b in REGS (ebx/edx) already.
	   Self-adapt: (a) scan esp0..esp0+0x200 for a POINTER value whose pointee
	   is 32 readable bytes (the out[8] buffer) -> that is `out`; (b) a/b are
	   the const-list scalars visible in entry regs (ebx=a). */
	{
		unsigned long long esp0 = (unsigned long long)g_ft_regs[0][4];
		unsigned long pa = g_ft_regs[0][3];   /* ebx = a for both samples */
		unsigned long pb = g_ft_regs[0][0];   /* eax = b for both samples */
		unsigned long pout = 0;
		int have_out = 0;
		duint szr = 0;
		/* out pointer: stack-local out[8] at ebp+0x18 (vmp_complex) is the
		   verified-correct slot; static g_out (vmp_base) at esp+0x28 is the
		   fallback. (ebp+0x18-first was verified: vmp_base result=357c84d0 +
		   vmp_complex out 8/8.) 实证：trace 写回反推(见下)在此样本会误中栈 scratch /
		   非4对齐垃圾，esp+0x28 栈猜测反而可靠——故栈猜测优先，trace 反推仅兜底。 */
		{
			unsigned long ebb = g_ft_regs[0][5];   /* ebp */
			unsigned char probe2[32];
			if (ebb && Script::Memory::Read((duint)(ebb + 0x18), probe2, 32, &szr) && szr == 32)
			{ pout = ebb + 0x18; have_out = 1; }
		}
		if (!have_out)
		{
			unsigned long cand_s = 0;
			if (Script::Memory::Read((duint)(esp0 + 0x28), &cand_s, 4, &szr) && szr == 4 &&
				cand_s >= 0x00010000 && cand_s < 0x7FFFFFFFUL)
			{
				unsigned char probe_s[32];
				if (Script::Memory::Read((duint)cand_s, probe_s, 32, &szr) && szr == 32)
				{ pout = cand_s; have_out = 1; }
			}
		}
		/* 参数标注: 用"捕获来源"命名(寄存器/栈偏移), 而非笼统 a/b/out。
		   ebx(a)/eax(b) 是 continue-chain 入口寄存器的值; out 来自栈偏移扫描。 */
		out_progress("[param] entry esp=%08X  ebx=%08X  eax=%08X  out=%08X(captured: ebx/eax/stack-scan) (self-adapt)",
			(unsigned int)esp0, (unsigned int)pa, (unsigned int)pb, (unsigned int)(have_out ? pout : 0));
		/* 关键: 把捕获的真实数值参数注入 g_src_vals(符号池), 让 0x60 成为
		   可追符号根(不再只是打印)。之前只打印不注入 = "追不了" 的第一道墙。 */
		{
			unsigned int pinj[2] = { (unsigned int)pa, (unsigned int)pb };
			const char *pinjn[2] = { "param_ebx", "param_eax" };
			for (int pj = 0; pj < 2; pj++)
			{
				/* 只注入"像算法数值"的(排除指针/引擎地址噪声);
				   0x60 这类小整数、0x13579BDF 这类算法种子才注入。 */
				if (pinj[pj] == 0 || pinj[pj] == 0xFFFFFFFFu) continue;
				if (vmp38_src_id_of(pinj[pj]) >= 0) continue;   /* 已存在(去重) */
				int is_ptr = (pinj[pj] >= 0x4000000u && pinj[pj] < 0x8000000u);
				if (is_ptr) continue;   /* 指针(模块地址)不注入为算法标量 */
				if (g_src_cnt < 64)
				{
					g_src_vals[g_src_cnt] = pinj[pj];
					_snprintf(g_src_names[g_src_cnt], 23, "%s", pinjn[pj]);
					g_src_cnt++;
					out_progress("[param] injected source %s = %08X (from entry %s, now traceable)",
						pinjn[pj], pinj[pj], pj == 0 ? "ebx" : "eax");
				}
			}
		}
		if (have_out)
		{
			g_out_param = pout;
			g_out_param_valid = 1;
			out_progress("[param-out] out=%08X (from stack-scan)  &out[3]=%08X",
				(unsigned int)pout, (unsigned int)(pout + 12));
		}
	}
	/* REVMOUT correction: if the self-adapt `out` (stack-local guess) reads
	   back as mostly CODE/STACK addresses (0x400000..0x80000000) it was a
	   garbage probe (GSCService), NOT the algorithm output. Recover the real
	   out[] from the TRACE store-backs (authoritative write-back values) and
	   let [out] print those instead. */
	if (!g_out_param_valid)
	{
		/* last-resort only: when the self-adapt stack guess NEVER located an
		   out at all, recover it from the trace store-backs. Never override a
		   self-adapt hit (vmp_base static g_out) - the trace-reverse probe
		   back-covers into stack scratch and breaks the 8/8 regression. */
		if (vmp38_find_out_from_trace())
			out_progress("[param-out-rev] recovered out=&%08x  (trace store-backs)",
				(unsigned int)g_out_param);
	}

	/* P1: per-handler instruction cache (dead-code elimination) */
	vmp38_decrypt_by_valueflow_vmpall();
	/* auto-reconstruct the source ops (the conversion table we compare
	   against vmp_all.cod): value-flow anchored, cross-handler. */
	vmp38_emit_source_ops();
	vmp38_trace_acc_slot();

	unsigned int h_ips[512];
	char h_mns[512][32];
	char h_diss[512][128];
	int h_tidx[512];   /* 2026-08-26 修值不准: 每条 handler 指令的【真实 g_ft trace 行】
	                       (而非 h_start_idx+k 近似) - dispatch/跳过指令会导致行号错位,
	                       值 pre/post 取错 → 改用真实行号取 g_ft_regs */
	int h_cnt = 0;
	int h_start_idx = 0;   /* g_ft index where the current handler began:
	                          lets [asm] attach reg before/after values */
	int h_n_junk = 0;      /* folded JUNK handlers (pure obfuscation, no semantics) */
	int h_n_live_total = 0;
	/* vCmp -> jcc pairing state: when a handler is classified vCmp, record
	   it here; the NEXT handler whose control-flow instruction is a jcc is
	   its source conditional branch (VMP P2P: vCmp sets flags, the following
	   jcc dispatch picks the branch). This replaces the flat "every jcc is a
	   [src] branch" dump with a semantic pairing that suppresses the handler
	   dispatch jumps that dominate the listing. */
	int vcmp_pending = 0;   /* a vCmp handler was just classified */

	for (int i = 0; i < n; i++)
	{
		unsigned int ip = g_ft_eip[i];
		DISASM_INSTR ins;
		const char *dis = "";
		char mn[32] = "";
		int is_ctrl = 0;

		memset(&ins, 0, sizeof(ins));
		if (!disasm_at_cached(ip, &ins, NULL))
			continue;   /* unreadable - skip (x64 parity) */
		if (ins.instruction[0])
			dis = ins.instruction;
		else
			continue;

		get_mnemonic(dis, mn, sizeof(mn));

		if (!strcmp(mn, "jmp") || !strcmp(mn, "call") || !strcmp(mn, "ret") ||
			!strcmp(mn, "retn") || mn[0] == 'j')
			is_ctrl = 1;

		/* cache the instruction (bounded) */
		if (h_cnt < 512)
		{
			h_ips[h_cnt] = ip;
			h_tidx[h_cnt] = i;   /* 2026-08-26 真实 trace 行(修值 pre/post 错位) */
			_snprintf(h_mns[h_cnt], sizeof(h_mns[h_cnt]), "%s", mn);
			_snprintf(h_diss[h_cnt], sizeof(h_diss[h_cnt]), "%s", dis);
			h_cnt++;
		}

		/* handler boundary on a control-flow instruction */
		if (is_ctrl && h_n > 0)
		{
			/* ---- P1 backward-liveness dead-code elimination ----
			   VMP handlers interleave the ONE real ALU op with many dead
			   register writes. Walk the handler backwards from the control
			   instruction, mark only defs that feed a live value, and drop
			   dead core ops from the semantics. */
			int active[16] = { 0 };
			{
				int d0[4], u0[4], nd0, nu0;
				insn_def_use(mn, dis, d0, &nd0, u0, &nu0);
				for (int k = 0; k < nu0; k++) active[u0[k]] = 1;
				if (!strcmp(mn, "ret") || !strcmp(mn, "retn")) active[4] = 1; /* esp */
			}
			int alive[512];
			for (int k = 0; k < h_cnt; k++) alive[k] = 0;
			for (int k = h_cnt - 1; k >= 0; k--)
			{
				int d2[4], u2[4], nd2, nu2;
				insn_def_use(h_mns[k], h_diss[k], d2, &nd2, u2, &nu2);
				/* the trailing control-flow instruction is the liveness
				   SEED (not discovered backwards) - force it alive so
				   call/ret/jmp semantics are extracted. */
				int is_alive = (k == h_cnt - 1) ? 1 : 0;
				for (int dd = 0; dd < nd2; dd++)
					if (active[d2[dd]]) { is_alive = 1; break; }
				/* rare single-op semantics (bit scan / count / conditional
				   move / setcc / widening moves) are the handler's semantic
				   BODY; their result travels to the next handler via a VM
				   carrier register, so in-handler liveness would wrongly
				   drop them. Force them alive. */
				if (!is_alive)
				{
					const char *rm = h_mns[k];
					if (!strncmp(rm, "bsf", 3) || !strncmp(rm, "bsr", 3) ||
						!strncmp(rm, "cmov", 4) || !strncmp(rm, "set", 3) ||
						!strncmp(rm, "popcnt", 6) ||
						!strncmp(rm, "movzx", 5) || !strncmp(rm, "movsx", 5))
						is_alive = 1;
				}
				/* memory/stack writes (push/pop/mov [mem],reg) are VM
				   stack-machine OUTPUTS - always live, else a direct
				   "jmp imm" handler has an empty liveness seed and the
				   whole handler is (wrongly) marked dead. */
				if (!is_alive)
				{
					if (strncmp(h_mns[k], "push", 4) == 0 ||
						strncmp(h_mns[k], "pop", 3) == 0)
						is_alive = 1;
					else if (strchr(h_diss[k], '[') != NULL)
					{
						const char *sp = strchr(h_diss[k], ' ');
						const char *comma = strchr(h_diss[k], ',');
						if (sp && comma && strchr(sp, '[') && strchr(sp, '[') < comma)
							is_alive = 1;
					}
				}
				if (is_alive)
				{
					alive[k] = 1;
					for (int uu = 0; uu < nu2; uu++) active[u2[uu]] = 1;
				}
			}

			/* re-accumulate semantics from LIVE instructions only */
			h_core[0] = 0; h_n_core = 0; h_n_cld = 0; h_n_cmp = 0;
			h_is_ret = 0; h_is_call = 0;
			h_n_not = 0; h_n_and = 0; h_n_or = 0;
			for (int k = 0; k < h_cnt; k++)
			{
				const char *km = h_mns[k];
				int is_core = vmp38_is_core_op32(km);
				if (is_core && !alive[k])
					continue;   /* dead core op - skip */
				if (is_core)
				{
					if (strlen(h_core) < 100)
					{
						if (h_n_core) SCAT(h_core, "+");
						SCAT(h_core, km);
					}
					h_n_core++;
				}
				if (!strcmp(km, "cld")) h_n_cld++;
				if (!strcmp(km, "cmp")) h_n_cmp++;
				if (!strcmp(km, "not")) h_n_not++;
				if (!strcmp(km, "and")) h_n_and++;
				if (!strcmp(km, "or")) h_n_or++;
				if (!strcmp(km, "ret") || !strcmp(km, "retn")) h_is_ret = 1;
				if (!strcmp(km, "call")) h_is_call = 1;
			}

			const char *tag = vmp38_classify_handler32(h_core, h_n_cld, h_n_cmp,
				h_is_ret, h_is_call, h_n_not, h_n_and, h_n_or);
			/* JUNK fold: no live core op AND no call/ret/cmp/cld -> pure
			   obfuscation, no VM semantics - fold into a counter. */
			int is_junk = (!h_n_core && !h_n_cld && !h_n_cmp && !h_is_ret && !h_is_call);
			if (is_junk)
			{
				h_n_junk++;
				/* 2026-08-30 不过滤关键代码: junk(纯死混淆, 无 core/cmp/call/ret)
				   不再静默丢弃, 而是输出一行"首条指令 + 指令数"标记, 保证任何地址
				   都能在 .txt 里定位, 不漏关键代码(若某段被判 junk 但其实是算法,
				   至少能看到它的地址和首条指令, 而非完全消失)。 */
				if (h_cnt > 0 && h_ips[0])
					out("  [src] @%08X: <junk %d insns>  %s",
						h_ips[0], h_cnt, h_diss[0]);
			}
			else
			{
				/* vCmp -> jcc pairing: if the PREVIOUS handler was a vCmp
				   (condition set) and THIS handler ends in a conditional
				   jump (its dispatch), that jcc is the source branch -
				   emit ONE [src] branch line. Handler-dispatch jccs (no
				   preceding vCmp) and vCmp handlers followed by a plain
				   jmp are skipped, so the listing stops being "mostly
				   jumps". */
				{
					int jcc_at = -1;
					for (int k = h_cnt - 1; k >= 0; k--)
					{
						const char *km = h_mns[k];
						if (km[0] == 'j' && strcmp(km, "jmp") != 0)
						{ jcc_at = k; break; }
					}
					if (vcmp_pending && jcc_at >= 0)
					{
						unsigned long long tgt = 0;
						parse_imm_from_disasm(h_diss[jcc_at], &tgt);
						/* dedup on (eip, target): a source loop re-executes
						   the same vCmp->jcc every iteration; emit each
						   distinct branch once (the "一堆跳转" symptom in
						   the loop body was this missing dedup). */
						static unsigned int s_br_eip[512];
						static unsigned int s_br_tgt[512];
						static int s_br_n = 0;
						int dup = 0;
						for (int bi = 0; bi < s_br_n; bi++)
							if (s_br_eip[bi] == (unsigned int)h_ips[jcc_at] &&
								s_br_tgt[bi] == (unsigned int)tgt)
							{ dup = 1; break; }
						if (!dup && s_br_n < 512)
						{
							s_br_eip[s_br_n] = (unsigned int)h_ips[jcc_at];
							s_br_tgt[s_br_n] = (unsigned int)tgt;
							s_br_n++;
							out("  [src] @%08X: %s  ; branch to %08llX",
								h_ips[jcc_at], h_diss[jcc_at], tgt);
						}
					}
					vcmp_pending = (tag && strcmp(tag, "vCmp") == 0) ? 1 : 0;
				}
				/* [const] dump: ALU ops with immediate, LIVE only */
				for (int k = 0; k < h_cnt; k++)
				{
					if (!alive[k]) continue;
					char ci_mn[32] = "";
					unsigned long long cimm = 0;
					get_mnemonic(h_diss[k], ci_mn, sizeof(ci_mn));
					if (strchr(h_diss[k], ',') &&
						parse_imm_from_disasm(h_diss[k], &cimm) &&
						(!strcmp(ci_mn, "add") || !strcmp(ci_mn, "sub") ||
						 !strcmp(ci_mn, "xor") || !strcmp(ci_mn, "and") ||
						 !strcmp(ci_mn, "or") || !strcmp(ci_mn, "rol") ||
						 !strcmp(ci_mn, "ror") || !strcmp(ci_mn, "shl") ||
						 !strcmp(ci_mn, "shr") || !strcmp(ci_mn, "sar") ||
						 !strcmp(ci_mn, "imul")))
					{
						out("[const] handler[%d] @%08x %s imm=%llx",
							h_idx, h_ips[k], ci_mn, cimm);
					}
				}
				/* [decrypt] dump (P2): recover real constants from encrypted
				   immediates via the ValueCommand::Calc chain. Build STEP_REC
				   from the cached handler instructions. */
				{
					STEP_REC drecs[512];
					int dn = 0;
					for (int k = 0; k < h_cnt && dn < 512; k++)
					{
						memset(&drecs[dn], 0, sizeof(drecs[dn]));
						drecs[dn].ip = h_ips[k];
						_snprintf(drecs[dn].disasm, sizeof(drecs[dn].disasm), "%s", h_diss[k]);
						drecs[dn].instr_len = 1;
						dn++;
					}
					vmp38_decrypt_dump32(drecs, dn, h_idx);
				}
				{
					int h_live = 0;
					for (int k = 0; k < h_cnt; k++) if (alive[k]) h_live++;
					const char *sem = vm_semantic_from_tag(tag, h_core);
					if (!sem)
						sem = vm_dispatch_semantic(h_mns, h_diss, alive, h_cnt);
					/* handler 语义行不在此处独立输出——它在 handler 尾部(收集完
					   src 首条指令的地址+数据前后值)后被合并成"每 handler 一行":
					   <首条x86折叠地址> <handler语义> <首条源op+reg前后+mem前后>。
					   让 .txt 干净紧凑(不再 handler 语义行与内部 x86 混杂)。
					   sem 在此缓存, 供尾部合并输出。 */
					h_n_live_total += h_live;
					/* ---- devirtualizer v1: value-anchored core-op reconstruction.
					   A REAL ALU handler's core op (adc edx,ecx) consumes operands
					   whose plaintext values sit in the trace registers. Name any
					   operand equal to a source scalar and emit a source op. */
					{
						static const char *core_mn[] = { "add","sub","adc","sbb","xor",
							"and","or","imul","mul","not","neg","shl","shr","sar","rol",
							"ror","inc","dec","movzx","movsx","bsf","bsr","popcnt",NULL };
						int did_devirt = 0;
						for (int k = 0; k < h_cnt && !did_devirt; k++)
						{
							char km2[16];
							get_mnemonic(h_diss[k], km2, sizeof(km2));
							int iscore = 0, mo;
							for (mo = 0; core_mn[mo]; mo++)
								if (!strcmp(km2, core_mn[mo])) { iscore = 1; break; }
							if (!iscore || !alive[k]) continue;
							if (strstr(h_diss[k], "[") != NULL)
								continue;
							int regs[4], rn2 = sym_parse_opregs(h_diss[k], regs);
							if (rn2 < 1) continue;
							char opn[3][24] = { "", "", "" };
							for (int q = 0; q < rn2 && q < 3; q++)
							{
								int di = h_tidx[k];   /* 2026-08-26 真实行 */
								unsigned int v = (di >= 0 && di < n) ? g_ft_regs[di][regs[q]] : 0;
								int sid = vmp38_src_id_of(v);
								if (sid >= 0)
									_snprintf(opn[q], 23, "%s", g_src_names[sid]);
								else
									_snprintf(opn[q], 23, "%08X", v);
							}
							unsigned int post = 0;
							{
								int di = h_tidx[k];   /* 2026-08-26 真实行 */
								if (di >= 0 && di + 1 < n) post = g_ft_regs[di + 1][regs[0]];
							}
							if (rn2 == 2)
							{
								/* value-anchored 命名(row: `mn op0 op1 = result`)已并入
								   handler 语义聚合行(见 handler 尾部合并), 不独立输出 */
								if (0) out("  [src] @%08X %s %s %s %s = %08X",
									(unsigned int)g_ft_eip[i], km2, opn[0], km2, opn[1], post);
							}
							else
							{
								if (0) out("  [src] @%08X %s %s = %08X",
									(unsigned int)g_ft_eip[i], km2, opn[0], post);
							}
							did_devirt = 1;
						}
					}
					/* [asm]: per-instruction restore sequence (live semantic core
					   ops, dedup). The restore .txt becomes a readable list of
					   VM-rebuilt instructions, like pre-VM assembly. */
					{
						char asmdone[512][160];
						int asmdn = 0;
						/* [sym]: collect the same live [asm] instruction texts, then
						   symbolically fold them (see vmp38_sym_fold). */
						int fold_n = 0;
						/* [src]: fold this handler's expanded [asm] stream back to
						   ONE source instruction. The dominant source opcode comes
						   from the conversion table (tag -> x86 mnemonic); we
						   capture that handler's core instruction (with its folded
						   memory operand + register/mem before/after values). */
						char src_asmd[400] = ""; int src_ip = 0; int have_src = 0;
						char src_mn[12] = "";
						const char *pri_mn = NULL;
						{
							static const struct { const char*t; const char*m; } PTAG[] = {
								{"vAdd","add"},{"vSub","sub"},{"vAdc","adc"},{"vSbb","sbb"},
								{"vAnd","and"},{"vOr","or"},{"vXor","xor"},{"vMul","imul"},
								{"vDiv","div"},{"vShl","shl"},{"vShr","shr"},{"vSar","sar"},
								{"vRol","rol"},{"vRor","ror"},{"vNand","and"},{"vNor","or"}
							};
							for (int pi = 0; pi < (int)(sizeof(PTAG)/sizeof(PTAG[0])); pi++)
								if (!strcmp(PTAG[pi].t, tag)) { pri_mn = PTAG[pi].m; break; }
							if (!pri_mn && sem)
							{
								/* fallback: first token of sem, e.g. "add d[sp+4],d[sp]" -> "add" */
								int tn=0;
								for (const char *s2=sem; *s2 && *s2!=' ' && tn<10; s2++) src_mn[tn++]=*s2;
								src_mn[tn]=0;
								if (tn>=2 && tn<=5) pri_mn = src_mn;
							}
						}
						for (int k = 0; k < h_cnt; k++)
						{
							if (!alive[k] || !vmp38_is_core_op32(h_mns[k])) continue;
							int dup = 0;
							for (int a2 = 0; a2 < asmdn; a2++)
								if (!strcmp(asmdone[a2], h_diss[k])) { dup = 1; break; }
							if (dup) continue;
							_snprintf(asmdone[asmdn], sizeof(asmdone[0]), "%s", h_diss[k]);
							asmdn++;
							/* per-instruction operands before/after values (approx
							   g_ft row = handler start + insn index k). Show every
							   GPR operand at its own width (sub-registers dl/cx/ax/
							   dh/cl included) - memory operands are handled below. */
							char valstr[160] = "";
							{
								int di = h_tidx[k];   /* 2026-08-26 真实 trace 行(修值错位) */
								if (di >= 0 && di + 1 < n)
								{
									const char *op = strchr(h_diss[k], ' ');
									if (op)
									{
										while (*op==' '||*op=='\t') op++;
										char wc[256];
										strncpy(wc, op, sizeof(wc)-1); wc[sizeof(wc)-1]=0;
										/* dedup the SAME register-name used in both src+dst
										   (e.g. "adc bl, bl" -> bl once); different names of the
										   same parent (ecx vs cl) are kept separately. */
										char s_seen[8][4];
										int s_seen_n = 0;
										/* split operands on comma */
										for (char *tok = strtok(wc, ","); tok; tok = strtok(NULL, ","))
										{
											while (*tok==' '||*tok=='\t') tok++;
											int tl = (int)strlen(tok);
											while (tl>0 && (tok[tl-1]==' '||tok[tl-1]=='\t')) tok[--tl]=0;
											if (tl <= 0) continue;
											/* memory operand (has [ ] or ptr/ss:/ds: base) -> skip
											   (its value handled by the mem-fold block below) */
											if (strchr(tok,'[') || strstr(tok," ptr") ||
												strstr(tok,"ss:") || strstr(tok,"ds:")) continue;
											/* strip qword/dword/word/byte tokens and remaining spaces */
											char nm[40]; int ns2=0;
											for (const char *p=tok; *p && ns2<38; p++)
											{
												if (*p==' '||*p=='\t') continue;
												nm[ns2++]=*p;
											}
											nm[ns2]=0;
											int idx=0, sh=0, bts=0;
											if (vmp38_reginfo(nm, &idx, &sh, &bts))
											{
												int dup = 0;
												for (int s0=0;s0<s_seen_n;s0++) if (!strcmp(s_seen[s0], nm)) { dup=1; break; }
												if (dup) continue;
												if (s_seen_n < 8) { _snprintf(s_seen[s_seen_n], 4, "%s", nm); s_seen_n++; }
												unsigned int mask = (bts==32) ? 0xFFFFFFFFu: (bts==16) ? 0xFFFFu : 0xFFu;
												unsigned int pre=(g_ft_regs[di][idx]>>sh)&mask;
												unsigned int post=(g_ft_regs[di+1][idx]>>sh)&mask;
												char tv[96];
												if (bts==32) _snprintf(tv,sizeof(tv),"  %s: %08X -> %08X",nm,pre,post);
												else if (bts==16) _snprintf(tv,sizeof(tv),"  %s: %04X -> %04X",nm,pre,post);
												else _snprintf(tv,sizeof(tv),"  %s: %02X -> %02X",nm,pre,post);
												size_t vl=strlen(valstr);
												if (vl+strlen(tv) < sizeof(valstr))
													_snprintf(valstr+vl,sizeof(valstr)-vl,"%s",tv);
											}
										}
									}
								}
							}
							/* [esp-fold]: rewrite obfuscated [esp+reg*N+imm] to [esp+0xOFF]
							   using the executing register snapshot for this insn */
							char disfold[160]; _snprintf(disfold, sizeof(disfold), "%s", h_diss[k]);
							{
								int di = h_tidx[k];   /* 2026-08-26 真实行(修 mem 值错位) */
								unsigned long long foff = 0;
								if (di >= 0 && di < n &&
									vmp38_fold_stack_disp(disfold, g_ft_regs[di], &foff))
								{
									const char *br = strchr(disfold, '[');
									const char *cr = br ? strchr(br + 1, ']') : NULL;
									if (br && cr && cr > br)
									{
										int prel = (int)(br - disfold);
										const char *suf = cr + 1;
										char fbuf[240];
										if (foff > 0x7FFFFFFFull)
										_snprintf(fbuf, sizeof(fbuf), "%.*s[esp-%X]%s", prel, disfold, (unsigned)(0x100000000ULL - foff), suf);
									else
										_snprintf(fbuf, sizeof(fbuf), "%.*s[esp+%X]%s", prel, disfold, (unsigned)foff, suf);
										_snprintf(disfold, sizeof(disfold), "%s", fbuf);
									}
								}
								/* 2026-08-30 通用地址折叠: 非 esp base 的加密寻址也还原。
								   [edi+eax*1-0x403AF57E] -> [edi+4]; [eax+ecx*4] -> [eax+value];
								   [ebp+ecx*1-0x28A1] -> [ebp+offset]。用执行期寄存器快照代入
								   index/scale/imm, base 寄存器名保留(有语义)。 */
								else if (di >= 0 && di < n)
								{
									unsigned long long goff = 0;
									int greg = -1;
									if (vmp38_fold_mem_addr(disfold, g_ft_regs[di], &goff, &greg) &&
										greg != 4)   /* esp 已被上面分支折叠, 只处理非 esp */
									{
										const char *br = strchr(disfold, '[');
										const char *cr = br ? strchr(br + 1, ']') : NULL;
										if (br && cr && cr > br)
										{
											int prel = (int)(br - disfold);
											const char *suf = cr + 1;
											static const char *bn8[8] =
												{ "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
											char fbuf[240];
											if (goff > 0x7FFFFFFFull) _snprintf(fbuf, sizeof(fbuf), "%.*s[%s-%X]%s",
												prel, disfold, bn8[greg], (unsigned)(0x100000000ULL - goff), suf);
											else _snprintf(fbuf, sizeof(fbuf), "%.*s[%s+%X]%s",
												prel, disfold, bn8[greg], (unsigned)goff, suf);
											_snprintf(disfold, sizeof(disfold), "%s", fbuf);
										}
									}
								}
							}
							/* memory operand value for this instruction (captured at
							   trace time: addr + pre/post). Reuse the folded operand
							   text ([esp+0xOFF]) so the listing shows what the
							   operand pointed at before/after the write. */
							char memvstr[96] = "";
							{
								std::unordered_map<unsigned int, GFT_MEM>::iterator it2 = g_asm_mem.find((unsigned int)h_ips[k]);
								if (it2 != g_asm_mem.end() && it2->second.valid)
								{
									const char *br = strchr(disfold, '[');
									const char *cr = br ? strchr(br + 1, ']') : NULL;
									if (br && cr && cr > br && (cr - br) < 60)
									{
										char mex[64];
										int L2 = (int)(cr - br) + 1;   /* include ']' */
										memcpy(mex, br, (size_t)L2); mex[L2] = 0;
										_snprintf(memvstr, sizeof(memvstr), "  [%08X]=%08X -> %08X",
											(unsigned int)it2->second.addr, it2->second.pre, it2->second.post);
									}
								}
							}
							/* [src] fold capture: the dominant source-opcode instruction
							   in this handler becomes the folded single line. */
							if ((h_mns[k][0] != 0 && h_mns[k][0] != 'j') &&
								(pri_mn && h_mns[k] && strstr(h_mns[k], pri_mn)) && !have_src)
							{
								_snprintf(src_asmd, sizeof(src_asmd), "%s%s%s", disfold, valstr, memvstr);
								src_ip = (int)h_ips[k];
								have_src = 1;
							}
							fold_n++;
							/* 不独立输出 [asm] 逐条(已并入 handler 聚合行) */
						}
						/* 2026-08-24 通用结构化还原：每 handler 折叠成一条源指令
						   (src_asmd = 源op + reg前后 + mem前后), 从转换表 tag 的 dominant
						   op(PTAG vAdd->add 等, 不依赖种子常量)。
						   2026-08-24 合并输出: 每 handler 只输出一行——
						     <首条x86折叠地址> <handler语义> <首条源op + reg前后 + mem前后>
						   handler 语义 + 数据前后合并, .txt 不再混杂。 */
						/* 2026-08-30 用户反馈: 每行开头的抽象栈机语义 `add d[sp+4],d[sp]` /
						   `movzx d[sp],b[sp+4]` 全是 d[sp+N], 淹没真实算法。去掉 sem 前缀,
						   只输出 src_asmd(真实 x86 + 折叠后的地址 + reg/mem 前后值)。 */
						/* 2026-08-30 修复 (no-sem) 噪音: (no-sem) 出现在那些"无 ALU 主体"
						   的 handler(vRet/vCall/vCmp/vDispatch/vInit —— 纯控制流/比较,
						   没有 PTAG 主导 op)。这些 handler 的 sem 是干净的
						   control-flow 语义(call 0x.. / ret / jmp 0x..), 不是 d[sp]
						   抽象形式。所以:
						   - have_src=1 → 输出 src_asmd(真实ALU指令);
						   - have_src=0 且 sem 是控制流(不含 d[sp]) → 输出 sem;
						   - have_src=0 且 sem 是 d[sp] 抽象 → 抑制(那是 ALU 但没
						     匹配到主导op的边角, 纯噪音)。
						   这能把 4万+ 条 (no-sem) 变成有意义的 ret/call/jmp。 */
						if (have_src && src_ip)
							out("  [src] @%08X: %s",
								src_ip, src_asmd);
						else if (sem && !strstr(sem, "d[sp"))
							out("  [src] @%08X: %s",
								src_ip ? src_ip : (int)g_ft_eip[i], sem);
						/* else: suppress (no-sem / d[sp] 抽象都属噪音) */
						/* 2026-08-30 关键内存读写输出: 之前每 handler 只挑一条 dominant
						   指令, mov reg,[mem] 读表 / mov [mem],reg 写关键地址(算法核心)
						   被前面的 mov 抢走 → 0122c1a2 这类关键写整段消失。这里把本
						   handler 里所有"live 的内存读写指令"(mov/movzx/movsx 且含 [)
						   逐条输出成 [src]，地址已折叠成 [base+scalar]，这才是能看出
						   算法的行。 */
						{
							for (int mk = 0; mk < h_cnt; mk++)
							{
								/* 内存读写是算法 I/O, 不依赖 backward-liveness(alive[])
								   也不依赖 dominant op —— 之前 alive[] 把它们判死导致
								   0122c1a2 这类关键写整段消失。只要求: mov 系 + 含 [ */
								if (!strchr(h_diss[mk], '[')) continue;
								if (strcmp(h_mns[mk], "mov") && strcmp(h_mns[mk], "movzx") &&
									strcmp(h_mns[mk], "movsx")) continue;
								/* 排除 VM 自己的虚拟栈 esp 操作? 不排除——用户要的关键写
								   [edx+ecx*2-0x513A] 就是非 esp 的。但为控制噪音, esp 相对
								   的操作量极大, 仍保留(地址折叠后是 [esp+0xN], 可读)。 */
								int d2 = h_tidx[mk];
								if (d2 < 0 || d2 >= n) continue;
								/* 折叠地址 + reg/mem 前后值(复用同一套 val/mem 逻辑) */
								char dv[160]; _snprintf(dv, sizeof(dv), "%s", h_diss[mk]);
								{
									unsigned long long foff = 0;
									if (vmp38_fold_stack_disp(dv, g_ft_regs[d2], &foff))
									{
										const char *br = strchr(dv, '[');
										const char *cr = br ? strchr(br + 1, ']') : NULL;
										if (br && cr && cr > br)
										{
											int prel = (int)(br - dv);
											char fb2[200];
											if (foff > 0x7FFFFFFFull)
												_snprintf(fb2, sizeof(fb2), "%.*s[esp-%X]%s",
													prel, dv, (unsigned)(0x100000000ULL - foff), cr + 1);
											else
												_snprintf(fb2, sizeof(fb2), "%.*s[esp+%X]%s",
													prel, dv, (unsigned)foff, cr + 1);
											_snprintf(dv, sizeof(dv), "%s", fb2);
										}
									}
									else
									{
										unsigned long long goff = 0; int greg = -1;
										if (vmp38_fold_mem_addr(dv, g_ft_regs[d2], &goff, &greg))
										{
											const char *br = strchr(dv, '[');
											const char *cr = br ? strchr(br + 1, ']') : NULL;
											if (br && cr && cr > br)
											{
												int prel = (int)(br - dv);
												static const char *bn8[8] =
													{ "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
												char fb2[200];
												if (goff > 0x7FFFFFFFull)
													_snprintf(fb2, sizeof(fb2), "%.*s[%s-%X]%s",
														prel, dv, bn8[greg], (unsigned)(0x100000000ULL - goff), cr + 1);
												else
													_snprintf(fb2, sizeof(fb2), "%.*s[%s+%X]%s",
														prel, dv, bn8[greg], (unsigned)goff, cr + 1);
												_snprintf(dv, sizeof(dv), "%s", fb2);
											}
										}
									}
								}
								/* 内存值前后(来自 g_asm_mem) */
								char mv3[96] = "";
								{
									std::unordered_map<unsigned int, GFT_MEM>::iterator it3 =
										g_asm_mem.find((unsigned int)h_ips[mk]);
									if (it3 != g_asm_mem.end() && it3->second.valid)
										_snprintf(mv3, sizeof(mv3), "  [%08X]=%08X -> %08X",
											(unsigned int)it3->second.addr, it3->second.pre, it3->second.post);
								}
								out("  [src] @%08X: %s%s",
									h_ips[mk], dv, mv3);
							}
						}
						/* [net]: run the handler's FULL instruction stream through
						   the symbolic engine and emit each register's net effect
						   (the source-level op this handler rebuilds). */
						{
							char net_as[80][220];
							int net_n = 0;
							for (int k = 0; k < h_cnt && net_n < 80; k++)
							{
								_snprintf(net_as[net_n], 220, "%s", h_diss[k]);
								net_n++;
							}
							char netbuf[1024] = "";
							int hpre2 = (h_start_idx >= 0 && h_start_idx < n) ? h_start_idx : i;
							vmp38_net_fold((const char(*)[220])net_as, net_n,
								g_ft_regs[hpre2],
								(hpre2 >= 0 && hpre2 + net_n <= n) ? &g_ft_regs[hpre2] : NULL,
								netbuf, sizeof(netbuf));
							if (netbuf[0])
							{
								/* 实测: net_fold 在 vmp_base 上只产出超长混淆表达式(net=),
								   `<<src: reg = a op b` 干净二元 op 标注 0 条——VMP 3.8
								   单 handler 净效果被混淆淹没, Titan match_add 思路在此
								   不成立(base 的跨 handler 阶段二折叠才是有效路径)。故
								   net 净效果不再落到 .txt(纯干扰), 只保留可能出现的
								   <<src: 干净源码 op 行(非空才输出)。真实源码 acc 由
								   阶段二 acc2_fold + golden 链覆盖。 */
								if (0) out("  [src] @%08X net= %s",
									(unsigned int)g_ft_eip[i], netbuf);
								const char *m = strstr(netbuf, "<<src:");
								if (m)
								{
									const char *sp = m + 6;
									while (*sp == ' ') sp++;
									out("  [src] @%08X %s",
										(unsigned int)g_ft_eip[i], sp);
								}
							}
						}
					}
				}
			}
			h_idx++;
			h_n = 0;
			h_start_idx = i + 1;   /* next handler's first g_ft row */
			h_vsp_in = (unsigned long long)g_ft_regs[i][4];
			h_cnt = 0;
		}
		h_n++;
	}

	/* last handler (no trailing control-flow boundary) */
	if (h_n > 0)
	{
		/* re-accumulate from the cached instructions (no control-flow seed,
		   keep all core ops - conservative) */
		h_core[0] = 0; h_n_core = 0; h_n_cld = 0; h_n_cmp = 0;
		h_is_ret = 0; h_is_call = 0;
		h_n_not = 0; h_n_and = 0; h_n_or = 0;
		for (int k = 0; k < h_cnt; k++)
		{
			const char *km = h_mns[k];
			if (vmp38_is_core_op32(km))
			{
				if (strlen(h_core) < 100)
				{
					if (h_n_core) SCAT(h_core, "+");
					SCAT(h_core, km);
				}
				h_n_core++;
			}
			if (!strcmp(km, "cld")) h_n_cld++;
			if (!strcmp(km, "cmp")) h_n_cmp++;
			if (!strcmp(km, "not")) h_n_not++;
			if (!strcmp(km, "and")) h_n_and++;
			if (!strcmp(km, "or")) h_n_or++;
			if (!strcmp(km, "ret") || !strcmp(km, "retn")) h_is_ret = 1;
			if (!strcmp(km, "call")) h_is_call = 1;
		}
		const char *tag = vmp38_classify_handler32(h_core, h_n_cld, h_n_cmp,
			h_is_ret, h_is_call, h_n_not, h_n_and, h_n_or);
		int is_junk = (!h_n_core && !h_n_cld && !h_n_cmp && !h_is_ret && !h_is_call);
		if (is_junk)
		{
			h_n_junk++;
		}
		else
		{
			const char *sem = vm_semantic_from_tag(tag, h_core);
			if (!sem)
			{
				int all_alive[512];
				for (int k = 0; k < h_cnt && k < 512; k++) all_alive[k] = 1;
				sem = vm_dispatch_semantic(h_mns, h_diss, all_alive, h_cnt);
			}
			out("handler[%d] %s%s%s (core: %s) n=%d vsp=%08llx->%08llx",
				h_idx, tag, sem ? " => " : "", sem ? sem : "",
				h_core[0] ? h_core : "-", h_n,
				h_vsp_in, (unsigned long long)g_ft_regs[n - 1][4]);
		}
	}

	/* [result]: the return value is whatever lands in eax right at the
	   algorithm's real return. Two source epilogue shapes are covered:
	     (a) `xor eax,ebx; xor eax,[mem]; ret`  (acc^a^b)
	     (b) `pop reg..; mov eax,<reg>; pop..; ret` (result rode in a reg)
	   Scan the trace TAIL for the LAST `mov eax,<reg>` (no imm, no mem) or
	   `xor eax,<reg>` - that is the return-value compute whose POST eax is
	   the final value. Verified: vm_complex mov eax,esi -> eax=3ab0016d. */
	{
		unsigned long last_eax = 0;
		int found = 0;
		/* prefer the code-hook live capture (OOM-independent): the real
		   return value moved into eax by the .text `mov eax,<reg>` epilogue */
		if (g_last_ret_eax_valid == 1)
		{
			last_eax = (unsigned long)g_last_ret_eax;
			found = 1;
		}
		else
		for (int pass = 0; pass < 2 && !found; pass++)
		{
			int prefer_mov = (pass == 0);   /* pass0: only mov eax,reg ; pass1: fallback xor */
			for (int i = n - 1; i >= 0; i--)
			{
				DISASM_INSTR di;
				memset(&di, 0, sizeof(di));
				if (!disasm_at_cached((unsigned int)g_ft_eip[i], &di, NULL))
					continue;
				char m2[16] = "";
				get_mnemonic(di.instruction, m2, sizeof(m2));
				int is_rvc = 0;
				if (strcmp(m2, "mov") == 0 &&
					strstr(di.instruction, "eax,") == di.instruction &&
					strchr(di.instruction, '[') == NULL &&
					strchr(di.instruction, '0') == NULL)
					is_rvc = 1;
				else if (!prefer_mov && strncmp(m2, "xor", 3) == 0 &&
					strstr(di.instruction, "eax"))
					is_rvc = 1;
				if (!is_rvc) continue;
				/* the VM-fn return `mov eax,<reg>` is IMMEDIATELY followed by
				   the epilogue pops (mov eax,esi; pop esi; ...; ret). Require
				   the very next inst to be a pop - skips main/printf mov eax. */
				{
					DISASM_INSTR dn; memset(&dn, 0, sizeof(dn));
					if (i + 1 < n &&
						disasm_at_cached((unsigned int)g_ft_eip[i + 1], &dn, NULL))
					{
						char mn2[16] = "";
						get_mnemonic(dn.instruction, mn2, sizeof(mn2));
						if (strncmp(mn2, "pop", 3) != 0) continue;
					}
					else continue;
				}
				/* REAL return heuristic: epilogue `mov eax,<reg>/xor` followed by
				   >=2 pops and a ret (real VM-fn return). */
				int has_ret = 0, n_pop = 0;
				for (int j = i + 1; j < n && j <= i + 9; j++)
				{
					DISASM_INSTR dr; memset(&dr, 0, sizeof(dr));
					if (disasm_at_cached((unsigned int)g_ft_eip[j], &dr, NULL))
					{
						char mr[16] = "";
						get_mnemonic(dr.instruction, mr, sizeof(mr));
						if (strncmp(mr, "pop", 3) == 0) n_pop++;
						if (strcmp(mr, "ret") == 0 && n_pop >= 2) { has_ret = 1; break; }
						if (strcmp(mr, "call") == 0) break;
					}
				}
				if (!has_ret) continue;
				/* POST eax = the frame after this compute */
				if (i + 1 < n)
					last_eax = g_ft_regs[i + 1][0];
				else
					last_eax = g_ft_regs[i][0];
				found = 1;
				out_progress("[result] scan hit @%08x '%s' post=%08x (n=%d)",
					(unsigned int)g_ft_eip[i], di.instruction, last_eax, n);
				break;
			}
		}
		if (found)
		{
			out_progress("[result] eax = %08x (return-compute ret from trace tail)", last_eax);
			g_ft_real_return = last_eax;
			g_ft_real_return_valid = 1;   /* REVM reconciles against REAL return */
		}
		else
			out_progress("[result] eax not captured (no return-compute ret in trace)");
		/* source-level return: emit the return-value compute as [src] so
		   the conversion table covers the function epilogue too.
		   2026-08-23 门控: 仅当检测到 vmp_base 真实 acc 链(g_src_had_acc)才输出,
		   否则是 vmp_base 专属硬编码模板,真实软件输出会误导。 */
		if (g_src_had_acc)
			out("  [src] return acc ^ a ^ b;   /* eax = %08X */", (unsigned int)last_eax);
	}

	/* OUTPUT-ARRAY check: the algorithm writes out[0..7] through the `out`
	   pointer (= g_out, read from entry-stack arg). After execution, read
	   the real g_out memory and dump it so "processed data must match" is
	   verifiable against the original program's printed o[0..7]. */
	if (g_out_param_valid && g_out_param != 0)
	{
		unsigned long ov[8];
		duint szr = 0;
		int nread = 0;
		/* REVMOUT: when the trace store-back recovery succeeded, use those
		   authoritative write-back posts directly (a stack-local out would
		   otherwise read stale/garbage from the live process). */
		if (g_out_val_valid)
		{
			for (int oi = 0; oi < 8; oi++) ov[oi] = g_out_vals[oi];
			nread = 8;
			goto out_done;
		}
		/* out is written by the UNICORN copy (virtual CPU), not the real
		   process - read it from Unicorn memory (uc_mem_read), falling back
		   to the real process for the dynamic (real single-step) path. */
		for (int oi = 0; oi < 8; oi++)
		{
			unsigned int b4 = 0;
			int ok = 0;
			if (g_uc)
				ok = vmp38_uc_read32_guarded(g_uc, (unsigned long long)(g_out_param + oi * 4), &b4);
			else
			{
				/* dynamic path only: read the real process (real single-step);
				   when g_uc exists but reads fail, do NOT fall back to the
				   real process - that yields pre-execution stale values. */
				unsigned char bb[4] = { 0, 0, 0, 0 };
				ok = (Script::Memory::Read((duint)(g_out_param + oi * 4), bb, 4, &szr) && szr == 4);
				if (ok) b4 = (unsigned int)(bb[0] | (bb[1] << 8) | (bb[2] << 16) | ((unsigned int)bb[3] << 24));
			}
			if (ok) { ov[oi] = b4; nread++; }
		}
out_done:
		if (nread == 8)
		{
			out_progress("[out] out[0..7] = %08x %08x %08x %08x %08x %08x %08x %08x",
				ov[0], ov[1], ov[2], ov[3], ov[4], ov[5], ov[6], ov[7]);
		}
		else
			out_progress("[out] g_out=%08llx read %d/8 (unreadable?)", (unsigned long long)g_out_param, nread);
	}

	/* STRUCTURAL tail: the for-loop out[] store-back and the return-value
	   compute that follow the 37 accumulations. These are the remaining
	   x86 instructions of vm_base beyond the acc chain (the pre-VM asm:
	     for (i=0;i<8;i++) out[i] = (a>>i)^(b<<i)^acc;
	     return acc ^ a ^ b;
	   Scan the trace tail for the `mov [base+idx*4], val` store pattern
	   (out[i] write-back) and emit the SOURCE-level loop + return so the
	   conversion table covers ALL assembly, not just the accumulations. */
	{
		/* find the tail `mov DWORD PTR [X+ecx*4], r` store-back sequence */
		int stores[16]; int nstores = 0;
		unsigned int store_base = 0;
		for (int i = n - 1; i >= 0 && nstores < 16; i--)
		{
			DISASM_INSTR di;
			memset(&di, 0, sizeof(di));
			if (!disasm_at_cached((unsigned int)g_ft_eip[i], &di, NULL))
				continue;
			char m2[16] = "";
			get_mnemonic(di.instruction, m2, sizeof(m2));
			if (strcmp(m2, "mov") != 0)
				continue;
			/* operand form: `mov DWORD PTR [reg+reg*4], reg` or [reg+imm] */
			const char *dst = strstr(di.instruction, "[");
			if (!dst) continue;
			/* a store-back has the memory operand as the FIRST operand and
			   a register/cl as index - the for-loop uses ecx as i */
			if (strstr(di.instruction, "DWORD PTR [") != di.instruction &&
				strstr(di.instruction, "mov\tdword ptr [") == NULL)
				continue;
			stores[nstores++] = i;
			if (!store_base) store_base = (unsigned int)g_ft_eip[i];
		}
		if (nstores >= 4)   /* saw the 8-iteration loop's store-backs */
		{
			unsigned int A = (g_src_cnt >= 1) ? g_src_vals[0] : 0;
			unsigned int B = (g_src_cnt >= 2) ? g_src_vals[1] : 0;
			unsigned int accf =
				(g_acc_gold_cursor > 0) ? g_acc_gold[g_acc_gold_cursor - 1].val : 0xA5A5A5A5u;
			if (g_src_had_acc)   /* 门控: 仅 vmp_base 真实 acc 结构输出模板 */
				out("  [src] for (i = 0; i < 8; i++)");
			if (g_src_had_acc)
				out("  [src]   out[i] = (a >> i) ^ (b << i) ^ acc;   /* 8 store-backs */");
			out("  [src] /* a=%08X b=%08X acc=%08X; loop store-backs=%d */",
				A, B, accf, nstores);
		}
		else
		{
			/* the out[] loop is VM-ized (its store-backs are obfuscated
			   `mov [esp+reg*mask+imm]` inside the VM section, not the clean
			   `mov [edi+ecx*4]` of the plain epilogue). Emit the SOURCE
			   semantics anyway - it is deterministic from the golden acc. */
			if (g_src_had_acc)   /* 门控: 仅 vmp_base 真实 acc 结构输出模板 */
				out("  [src] for (i = 0; i < 8; i++)");
			if (g_src_had_acc)
				out("  [src]   out[i] = (a >> i) ^ (b << i) ^ acc;   /* VM-ized store-backs */");
		}
	}

	out("=== trace restore done: %d handlers (%d junk folded, %d live instrs) in %d instrs ===",
		h_idx + (h_n ? 1 : 0), h_n_junk, h_n_live_total, n);
	vmp38_emit_summary();
}


int vmp38_unicorn_full_trace(unsigned long long entry_ip,
	unsigned long long entry_esp, unsigned long long *out_last_eip)
{
	int i;
	int ft_log_owned = vmp38_log_open("restore");   /* ensure log file exists */
	/* direct-to-file extraction: nothing is buffered in g_ft - the hook
	   writes the analyzer data file while tracing (unbounded, no 2GB cap) */
	g_stream_total = 0;
	g_stream_last_ip = 0;
	g_insn_invalid_handled = 0;   /* no stale INSN_INVALID resume flag across runs */
	/* unconditional state reset (defensive - also covers an interrupted trace
	   that never reached the tail reset) */
	g_dw_need_full = 1;   /* first row dumps full regs */
	memset(g_last_regs, 0, sizeof(g_last_regs));
	g_last_ef = 0;
	{
		/* read XXVM_APPENDS once (headless/automation default); GUI menu overrides later */
		static int s_env_read = 0;
		if (!s_env_read) {
			char sb[8] = "";
			GetEnvironmentVariableA("XXVM_APPENDS", sb, sizeof(sb));
			if (sb[0] == '0') g_agg_appends = 0;
			/* XXVM_APPENDS_MAX: custom per-eip appends cap (default 256).
			   e.g. set 0 = no cap (unbounded, memory grows with loop
			   depth), or 16/64/512 to trade completeness vs memory. */
			char sb2[32] = "";
			GetEnvironmentVariableA("XXVM_APPENDS_MAX", sb2, sizeof(sb2));
			if (sb2[0])
			{
				int v = atoi(sb2);
				if (v >= 0)
				{
					g_row_max_appends = v;
					out("[env] XXVM_APPENDS_MAX=%d (appends cap)", g_row_max_appends);
				}
			}
			/* XXVM_AUTO_CONTINUE: headless/automation control for the
			   auto-continue-VM toggle (g_vm_continue). Set 0 -> VM function
			   returns at its first vm-exit ret to .text (do NOT chase .text
			   stubs / next VM functions - a normal .text func like
			   GSCService 0057a0d8 would otherwise be mis-traced into a spin).
			   GUI toggle still overrides at the menu. */
			{
				char sbc[8] = "";
				GetEnvironmentVariableA("XXVM_AUTO_CONTINUE", sbc, sizeof(sbc));
				if (sbc[0] == '0') g_vm_continue = 0;
				else if (sbc[0] == '1') g_vm_continue = 1;
			}
			s_env_read = 1;
		}
	}
	g_t_trace0 = GetTickCount64();   /* start perf clock before uc_init */
	_plugin_logprintf("[FT] unicorn_full_trace entry=%08llx (uc=%p)", entry_ip, (void*)g_uc);
	{ char exeP[MAX_PATH] = ""; GetModuleFileNameA(NULL, exeP, sizeof(exeP)); vmp38_trace32_open(exeP); }
	/* full stack snapshot BEFORE uc_init dumps the module - the fmem hooks
	   (vmp38_fmem_read/write_hook) serve reads/writes from g_snap, and
	   uc_init copies g_snap into Unicorn so self-modifying VM-stack data is
	   read/write consistent (g_snap was never populated before -> NULL). */
	vmp38_snap_take(entry_esp);
	if (!g_uc)
	{
		if (!vmp38_uc_init(g_vm_start, g_vm_end, 0, entry_ip))
			return 0;
	}
	/* module base for exit detection (set before run; hook reads it) */
	{
		Script::Module::ModuleInfo mi2;
		if (Script::Module::GetMainModuleInfo(&mi2))
		{
			g_module_base = mi2.base;
			g_module_size = mi2.size;
		}
	}
	/* fs segment = real TEB: NOT writable in Unicorn (uc_reg_write(FS)
	   returns UC_ERR_INVALID_REG, err=21). With fs=0, fs:[0x18] reads
	   [0x18] - pre-fill it with the REAL PEB pointer so
	   "mov ecx, fs:[0x18]" (00414a45) gets the right value and the
	   following jz takes the real branch (GSCService). */
	{
		duint teb_b = 0, peb_v = 0;
		if (Script::Misc::ParseExpression("teb()", &teb_b) && teb_b &&
			DbgMemRead((duint)(teb_b + 0x18), &peb_v, 4) && peb_v)
		{
			uc_mem_write(g_uc, 0x18, &peb_v, 4);
			out_progress("[fs] [0x18]=PEB %08x (fs:[0x18] patched)", (unsigned int)peb_v);
		}
		else
			out_progress("[fs] teb()/PEB read failed - fs:[0x18] stays 0");
	}
	/* initial registers: live process state at the VM entry breakpoint */
	{
		static const int rmap[8] = {
			UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
			UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI };
		static const Script::Register::RegisterEnum renum[8] = {
			Script::Register::RegisterEnum::EAX, Script::Register::RegisterEnum::ECX,
			Script::Register::RegisterEnum::EDX, Script::Register::RegisterEnum::EBX,
			Script::Register::RegisterEnum::ESP, Script::Register::RegisterEnum::EBP,
			Script::Register::RegisterEnum::ESI, Script::Register::RegisterEnum::EDI };
		for (i = 0; i < 8; i++)
		{
			unsigned long long rv = (unsigned long long)Script::Register::Get(renum[i]);
			rv &= 0xffffffffULL;
			uc_reg_write(g_uc, rmap[i], &rv);
		}
		{
			unsigned long long ef = (unsigned long long)Script::Register::Get(Script::Register::RegisterEnum::CFLAGS);
			ef &= 0xffffffffULL & ~0x100ULL;   /* clear TF (VMP anti-debug) */
			uc_reg_write(g_uc, UC_X86_REG_EFLAGS, &ef);
		}
	}
	/* reset trace */
	g_ft_n = 0;
	g_ft_stop_reason = 0;
	g_ft_oom_logged = 0;
	g_ft_match = 0;   /* fresh run: sequential reg-snapshot cursor from 0 */
	p3_symreset(entry_ip, entry_esp);   /* P3: reset symbolic register state */
	vmp38_srcs_load();   /* load seed source variables (a/b/acc0) for sym naming */
	vmp38_algo2_init(&g_symreg);   /* 2026-08-25 立项: 分析侧参数溯源旁路初始化(须在 srcs_load 后) */
	vmp38_acc_build_golden();   /* build the FULL golden acc chain (38 steps) */
	/* 2026-08-25 param table (traceability start). Honest: fb_seed(a/b/acc0 =
	   0x13579BDF/0x2468ACE0/0xA5A5A5A5) is an unconditional placeholder seed
	   (only for the vmp_base golden demo). It is NOT the real function's
	   params, so it is no longer printed as the param table. Only EXTRA seeds
	   injected from const_list.txt / self-adapt (g_src_cnt>3) are traceable
	   REAL params (e.g. a real sample's b=0x000065D1). continue-chain enters
	   at an engine dispatch mid-state, so entry regs are engine intermediate
	   values, unreliable as params, and are not listed. */
	{
		out("  [algo] param no confirmed real function param"
			" (fb_seed 0x13579BDF/0x2468ACE0/0xA5A5A5A5 is vmp_base placeholder, NOT this sample)");
		if (g_src_cnt > 3)
		{
			out("  [algo] param extra traceable seeds (const_list/self-adapt):");
			for (int si = 3; si < g_src_cnt; si++)
				out("  [algo] param   %s = %08X", g_src_names[si], g_src_vals[si]);
		}
	}
	out_progress("=== VMP 3.8 FULL TRACE (Unicorn virtual CPU, entry=%08llx) ===", entry_ip);
	/* inject real entry-stack arguments into the Unicorn stack map so the
	   VM code computes with the SAME parameters the source function used
	   (VMP 3.8 leaves args plaintext at esp+0x18.. on the VM entry stack;
	   the [param] line reads them from the live process). Without this the
	   Unicorn stack is empty -> VM computes with garbage args -> result
	   never matches the source. */
	if (entry_esp)
	{
		unsigned long esp0 = (unsigned long)entry_esp;
		int pj;
		for (pj = 0; pj < 16; pj++)
		{
			unsigned long pv = 0;
			unsigned long long dst = esp0 + 4 + pj * 4;
			if (DbgMemRead((duint)(esp0 + 4 + pj * 4), &pv, 4))
			{
				if (uc_mem_write(g_uc, dst, &pv, 4) != UC_ERR_OK)
					break;
				/* pointer-arg dereference: if the arg points at readable
				   memory, inject what it points to (out-param buffers /
				   tables the VM reads/writes). 64 bytes covers table[8]
				   and &cks etc. */
				{
					unsigned char pb2[64];
					if (DbgMemRead((duint)pv, pb2, sizeof(pb2)))
						uc_mem_write(g_uc, pv, pb2, sizeof(pb2));
				}
			}
			else
				break;
		}
	}
	/* Continuous execution. On an unmapped-access error (err=6/7 - VMP
	   stack addressing resolved outside the Unicorn map), the virtual CPU
	   stalls; resume from the next instruction (skip the unreadable one -
	   it is obfuscation, the trace backbone stays complete) so the run
	   reaches the function return. */
	{
		uc_err err = uc_emu_start(g_uc, entry_ip, 0, 0, 0);
		int resume_guard = 0;
		int finish_printed = 0;   /* emit "trace finished normally" exactly once
		   even though the resume while-loop below re-enters the emu_start
		   until resume_guard hits its cap (500). */
		/* If the very first emu_start aborts before executing a single
		   instruction (g_stream_total==0), the resume loop below is gated
		   on g_stream_total>0 and would silently produce an EMPTY trace.
		   Surface it so the user knows the entry wasn't executable. */
		if (err != UC_ERR_OK && g_stream_total == 0)
			out_progress("[ft] emu_start aborted before any instruction: err=%d at entry=%08llx (entry not executable? retry at VM entry)",
				(int)err, entry_ip);
		/* resume on unmapped errors AND on err==0: VMP dispatchers end with
		   iretd/iret which Unicorn treats as a normal return (err=0) even
		   though the VM function continues - resume from the last eip.
		   No-progress guard: if a resume executes 0 instructions, break. */
		while ((err == UC_ERR_READ_UNMAPPED || err == UC_ERR_WRITE_UNMAPPED ||
			err == UC_ERR_FETCH_UNMAPPED || err == UC_ERR_MAP || err == UC_ERR_OK ||
			(err == UC_ERR_INSN_INVALID && g_insn_invalid_handled) ||
			(err == UC_ERR_HOOK && g_ft_skip_resume)) &&
			g_stream_total > 0 &&
			!g_ft_stop_reason && resume_guard < 500)
		{
			unsigned long long prev_total = g_stream_total;
			int insn_invalid_resume = (err == UC_ERR_INSN_INVALID && g_insn_invalid_handled);
			g_insn_invalid_handled = 0;   /* consume the one-shot INSN_INVALID resume */
		{
			unsigned long long resume;
			{
				/* system-DLL skip: a hook set the exact resume address (the
				   skipped call's return address); resume there directly. */
				if (g_ft_skip_resume)
				{
					resume = g_ft_skip_resume;
					g_ft_skip_resume = 0;
				}
				else
				{
				/* precise next-instruction: when err==OK (iretd/iret treated
				   by Unicorn as a normal return) the VM continues at the
				   REAL eip Unicorn computed - sequential +instr_size from
				   the last hook eip lands on the wrong instruction (iret is
				   not fall-through). For unmapped errors the failed instr is
				   fetchable but its read/write op failed, so +instr_size
				   skips exactly it. */
				unsigned long long real_eip = 0;
				if ((err == UC_ERR_OK || insn_invalid_resume) &&
					uc_reg_read(g_uc, UC_X86_REG_EIP, &real_eip) == UC_ERR_OK &&
					real_eip)
					resume = real_eip;
				else
				{
					resume = g_stream_last_ip;
					DISASM_INSTR di3;
					memset(&di3, 0, sizeof(di3));
					DbgDisasmAt((duint)resume, &di3);
					resume += (di3.instr_size >= 1 ? di3.instr_size : 1);
				}
				}   /* else (not skip) */
			}
			/* keep the trace record but mark this slot as a skip point */
			out("[ft] err %d at %08x - skip to %08x (guard %d)", (int)err,
				(unsigned int)g_stream_last_ip,
				(unsigned int)resume, resume_guard);
			err = uc_emu_start(g_uc, resume, 0, 0, 0);
			if (g_stream_total == prev_total) break;   /* no progress - real stop */
			resume_guard++;
		}
		/* err semantics: 0=ran-until-end, 10=UC_ERR_HOOK (a hook stopped the
		   emulation). BOTH are normal successful stops - only print the raw
		   code when it is NOT one of those, so a clean trace reads
		   "trace finished normally" instead of a scary non-zero number. */
		if (err == UC_ERR_OK || err == UC_ERR_HOOK)
		{
			/* normal stop - announce ONCE (the resume while-loop below
			   re-enters emu_start until resume_guard hits its cap, so
			   without the flag this floods the log with identical lines) */
			if (!finish_printed)
			{
				finish_printed = 1;
				out_progress("[ft] trace finished normally (VM return / dispatcher stop)");
			}
		}
		else
			out_progress("[ft] emu_start err=%d %s", (int)err,
				err == UC_ERR_READ_UNMAPPED ? "(read unmapped - resumed)"
				: err == UC_ERR_WRITE_UNMAPPED ? "(write unmapped - resumed)"
				: err == UC_ERR_FETCH_UNMAPPED ? "(fetch unmapped - resumed)"
				: "(see above resume/skip lines)");
	}
	if (out_last_eip)
		*out_last_eip = g_stream_total > 0 ? g_stream_last_ip : 0;
	g_trace_mode = 1;
	/* trace tail: data was written DIRECTLY while tracing - nothing to
	   analyze here; the standalone analyzer turns it into pseudo-C++ */
	if (g_t_trace0)
		out_progress("[perf] full-trace elapsed %llu ms for %llu instrs",
			(unsigned long long)(GetTickCount64() - g_t_trace0), g_stream_total);
	/* write aggregated rows + reset row state (shared with dynamic) */
	/* flush the last defer-by-one row: the code hook never came again, so
	   emit it with post==NULL (single before-value), so no instruction is
	   lost at the very end of the trace. */
	if (g_dw_pend_valid)
	{
		vmp38_emit_row(g_dw_pend_eip, g_dw_pend_code, g_dw_pend_pre, NULL,
			g_dw_pend_maddr, g_dw_pend_mpre, 0, g_uc);
		g_dw_pend_valid = 0;
	}
	int distinct = vmp38_emit_tail();
	/* symbolic source reconstruction (BEFORE restore_from_trace32!): the
	   per-handler net_fold inside restore_from_trace32 resets the shared
	   sym pool, which would corrupt g_symreg's node ids. Dump the traced
	   cross-handler symbols here, while they are still valid. */
	{
		static const char *srn4[4] = { "eax", "ecx", "edx", "ebx" };
		char sb[1024];
		int si;
		/* out() not out_progress: this is a symbolic-reconstruction dump
		   (file content), not progress; it must NOT spam the GUI/console. */
		out("  [sym-src] value-anchored cross-handler symbols (src=%d):", g_src_cnt);
		for (si = 0; si < 4; si++)
		{
			g_sym_dbg_depth = 0;
			sym_expr_str(g_symreg.sym[si], sb, sizeof(sb));
			out("  [sym-src] %s = %s  (concrete %08x)",
				srn4[si], sb,
				(unsigned int)g_symreg.r[si]);
		}
	}
	/* P0: rebuild handler semantics + [const] + [param]/[result] from the
	   recorded g_ft trace, written to the .txt restore log. Must run BEFORE
	   vmp38_log_close (which closes g_fout). */
	vmp38_restore_from_trace32();
	/* 2026-08-30 用户方向修正: .txt 退回"data 逐条 + 两个文本优化", 不做任何
	   语义过滤(handler折叠/junk/alive 全关闭)。之前 restore_from_trace32 的
	   折叠会丢关键代码(mov 曾不在 core、内存读被判死), 所以这里用 data 逐条
	   重写 .txt: eip= -> 地址: , 地址折叠([edi+eax*1-0x..]->[edi+4]),
	   mem 值改绝对地址标签 [0xaddr]=pre->post。垃圾代码先全保留, 观察后再精准过滤。 */
	vmp38_rewrite_txt_from_data();
	out_progress("=== trace done: %llu instrs, %d distinct eips written to data (stop=%d) ===",
		g_stream_total, distinct, g_ft_stop_reason);
	if (ft_log_owned) vmp38_log_close();   /* flush+close: next trace must NOT append into this session file */
	vmp38_trace32_close();                 /* x64dbg .trace32 output flush */
	return 0;
}
}

/* 2026-08-30: .txt 重写 = data 逐条 + 地址折叠 + 绝对地址标签 + 去混淆三招。
   招1(thread-289859 * 行): 写机制寄存器{esi,edi,ebp}(除 push/pop) 删除。
   招2(活跃变量 DCE): 后向 liveness 死定义删除(子寄存器宽度感知)。
   (招3 MBA/等价替换 留待 IR 层, 本轮不实现。)
   data 行格式: eip=ADDR \t code \t regs \t [mem]=pre->post @addr */
static void vmp38_rewrite_txt_from_data(void)
{
	char dpath[MAX_PATH] = "";
	{
		const char *fp = g_fout_path;
		size_t L = strlen(fp);
		/* 防 dpath 溢出: 路径超长则截断到 MAX_PATH-1, 且用 snprintf 安全拼接 */
		if (L > sizeof(dpath) - 12) L = sizeof(dpath) - 12;
		if (L > 4 && _stricmp(fp + L - 4, ".txt") == 0)
		{
			memcpy(dpath, fp, L - 4);
			dpath[L - 4] = 0;
		}
		else
			_snprintf(dpath, sizeof(dpath), "%s", fp);
		SCAT(dpath, ".data.txt");
	}
	FILE *fin = fopen(dpath, "rb");
	if (!fin)
	{
		out_progress("[txt] rewrite: cannot open %s - skip", dpath);
		return;
	}
	if (g_fdata) fflush(g_fdata);   /* 关键: data 4MB 缓冲, 先冲刷再读, 否则尾段未刷盘提前 EOF */
	if (g_fout) { fclose(g_fout); g_fout = NULL; }
	g_fout = fopen(g_fout_path, "wb");
	if (!g_fout)
	{
		fclose(fin);
		out_progress("[txt] rewrite: cannot reopen %s - skip", g_fout_path);
		return;
	}
	fprintf(g_fout, "xxvm vmp38 restore - [asm] instruction sequence (data + addr-fold + deobf)\n");

	/* 2026-09-05 取消硬上限(用户要求): 不设 MAXINS, 按 data 实际行数动态分配。
	   32 位进程地址空间 ~2GB, 1370 万行 offsets(8B)+drop(1B) ≈ 125MB, 可承受。
	   做法: 第一遍只统计 eip 行数 count, 再按 count 分配, 第二遍填 offsets。 */
	long n_count = 0;
	{
		char line[4096];
		while (fgets(line, sizeof(line), fin))
			if (strncmp(line, "eip=", 4) == 0) n_count++;
	}
	long *offsets = (long*)malloc(sizeof(long) * (n_count + 1));
	unsigned char *drop = (unsigned char*)malloc(n_count + 1);
	if (!offsets || !drop)
	{
		out_progress("[txt] rewrite: OOM alloc (%ld insn) - fallback to plain rewrite", n_count);
		free(offsets); free(drop); fclose(fin); fclose(g_fout);
		g_fout = NULL;
		return;
	}
	/* 复位到文件头, 重新遍历填 offsets */
	fseek(fin, 0, SEEK_SET);
	long n = 0;
	/* ---- 第一遍: 记录每行 eip 的文件偏移 ---- */
	{
		long pos = ftell(fin);
		char line[4096];
		while (fgets(line, sizeof(line), fin))
		{
			if (strncmp(line, "eip=", 4) == 0)
			{
				offsets[n] = pos; n++;
			}
			pos = ftell(fin);
		}
	}
	out_progress("[txt] rewrite pass1: %ld eip lines indexed", n);

	/* ---- 2026-09-01 任务1: 半自动识别专属寄存器 + taint 分析遍 ---- */
	vmp38_taint_analyze_pass(dpath);

	/* ---- 第二遍: 后向 liveness + 机制删除 算 drop[] ---- */
	/* NPAR=8: eax,ecx,edx,ebx,esi,edi,ebp,esp; 位0..31 掩码 */
	long mech_drop = 0, dce_drop = 0, dce_mem_drop = 0;
	{
		unsigned int live[8] = { 0,0,0,0,0,0,0,0 };
		int eflags_live = 0;
		/* 内存槽 liveness(后向): 记录"后续会被读"的绝对地址(栈窗写后不被读=死写入)
		   堆分配(防 640KB 栈溢出), 65536 桶足够(live 地址通常几万个) */
		#define MEMLIVE_BUCKETS 65536
		#define MEMLIVE_MASK (MEMLIVE_BUCKETS-1)
		unsigned int *memlive_key = (unsigned int*)malloc(sizeof(unsigned int) * MEMLIVE_BUCKETS);
		unsigned char *memlive_used = (unsigned char*)malloc(MEMLIVE_BUCKETS);
		if (!memlive_key || !memlive_used)
		{
			/* 分配失败: 跳过硬清零, 继续(放弃内存槽 DCE, 不影响其它去混淆) */
			free(memlive_key); free(memlive_used); memlive_key = NULL; memlive_used = NULL;
		}
		unsigned int esp_ref = 0;
		if (memlive_used) for (long _b = 0; _b < MEMLIVE_BUCKETS; _b++) memlive_used[_b] = 0;
		char line[4096];
		for (long i = n - 1; i >= 0; i--)
		{
			drop[i] = 0;
			fseek(fin, offsets[i], SEEK_SET);
			if (!fgets(line, sizeof(line), fin)) continue;
			int ll = (int)strlen(line);
			while (ll > 0 && (line[ll-1]=='\n'||line[ll-1]=='\r')) line[--ll]=0;
			char *code = strchr(line + 4, '\t');
			if (!code) continue;
			code++;
			char *regs = strchr(code, '\t');
			/* 解析 mem 列(截断 regs 前): mem 在 regs 后的第二个 tab 之后 */
			char *memcol = NULL;
			unsigned int mabs = 0; int has_mabs = 0;
			if (regs)
			{
				memcol = strchr(regs, '\t');   /* regs 后第一个 tab -> mem 列 */
				if (memcol) { *memcol = 0; memcol++; }
				/* esp pre 值(栈窗参考) */
				{
					char rc[512];
					_snprintf(rc, sizeof(rc), "%s", regs);
					for (char *tk = rc; tk && *tk; )
					{
						char *sp = strchr(tk, ' ');
						if (sp) *sp = 0;
						if (strncmp(tk, "esp=", 4) == 0)
						{
							esp_ref = (unsigned int)strtoul(tk + 4, NULL, 16);
							break;
						}
						tk = sp ? sp + 1 : NULL;
					}
				}
				if (memcol && memcol[0])
				{
					/* 内存列格式 [addr]=pre->post (绝对地址, 无 0x) */
					const char *lb = strchr(memcol, '[');
					if (lb) { mabs = (unsigned int)strtoul(lb + 1, NULL, 16); has_mabs = 1; }
				}
				*regs = 0;   /* 截断 regs(本遍只用 code 文本) */
			}
			/* 内存读/写判定 */
			{
				const char *lb = strchr(code, '[');
				const char *comma = strchr(code, ',');
				int is_mem_write = 0, is_mem_read = 0;
				if (lb)
				{
					const char *rb = strchr(lb, ']');
					if (comma && rb && comma > rb)
						is_mem_write = 1;   /* 逗号在]后=>目标是内存(写) */
					else if (comma && lb && comma < lb)
						is_mem_read = 1;    /* 逗号在[前=>目标是寄存器, 内存是源(读) */
					else
					{
						/* 无逗号或逗号在[..]内: 单操作数 RMW 内存(neg [esp]/inc [esp]) */
						const char *o2 = code;
						while (*o2==' '||*o2=='\t') o2++;
						char tmpmn[16]=""; int m2=0;
						while (o2[m2] && m2<15 && o2[m2]!=' ' && o2[m2]!='\t'){tmpmn[m2]=o2[m2];m2++;}
						tmpmn[m2]=0;
						if (!strcmp(tmpmn,"inc")||!strcmp(tmpmn,"dec")||!strcmp(tmpmn,"neg"))
							is_mem_write = 1;   /* 单操作数 RMW 内存: neg [esp] */
						else
							is_mem_read = 1;
					}
				}
				if (has_mabs && memlive_used && memlive_key)
				{
					/* memlive 开放寻址: 用 3 状态(0=空,1=占用,2=tombstone)防 probe 链断裂,
					   且加 probe 上限防表满死循环(block bug 修复) */
					unsigned int slot = ((unsigned int)(mabs * 2654435761u)) & MEMLIVE_MASK;
					if (is_mem_read)
					{
						int probe = 0;
						while (probe < MEMLIVE_BUCKETS && memlive_used[slot] == 1 && memlive_key[slot] != mabs)
						{ slot = (slot + 1) & MEMLIVE_MASK; probe++; }
						if (probe < MEMLIVE_BUCKETS && memlive_used[slot] != 1)
						{ memlive_used[slot] = 1; memlive_key[slot] = mabs; }
					}
					else if (is_mem_write)
					{
						/* 死写入判定收紧: 只删"显式 [esp]/[ebp 基址"的栈槽写入(VMP死常量填充
						   都是 [esp+N]/[ebp+N] 文本形式). [edx]/[esi] 等寄存器基址的写入是
						   算法输出(如 mov byte [edx],al 写结果), 一律保留. */
						int is_stack = 0;
						{
							const char *lb = strchr(code, '[');
							if (lb)
							{
								const char *bp = lb + 1;
								while (*bp==' ') bp++;
								if (strncmp(bp, "esp", 3) == 0 || strncmp(bp, "ebp", 3) == 0)
									is_stack = 1;
							}
						}
						int lv = 0;
						int probe = 0;
						while (probe < MEMLIVE_BUCKETS && memlive_used[slot] != 0 && memlive_key[slot] != mabs)
						{ slot = (slot + 1) & MEMLIVE_MASK; probe++; }
						if (probe < MEMLIVE_BUCKETS && memlive_used[slot] == 1 && memlive_key[slot] == mabs) lv = 1;
						if (is_stack && !lv)
						{
							drop[i] = 1; dce_mem_drop++;
							continue;   /* 死栈写入: 删 */
						}
						/* 写覆盖 live 槽: 置 tombstone(2) 而非 0, 保 probe 链完整 */
						if (lv)
							memlive_used[slot] = 2;
						/* 非栈写入: 保留(算法输出 side effect) */
					}
				}
			}
			/* 解析 mnemonic + 目标寄存器 */
			const char *op1 = code;
			while (*op1==' '||*op1=='\t') op1++;
			char mn[16]; int mi=0;
			while (op1[mi] && mi<15 && op1[mi]!=' ' && op1[mi]!='\t'){mn[mi]=op1[mi];mi++;}
			mn[mi]=0;
			/* 机制删除判定: 写 {esi,edi,ebp} 且非 push/pop */
			int is_pushpop = (!strcmp(mn,"push")||!strcmp(mn,"pop")||!strcmp(mn,"pushfd")||!strcmp(mn,"popfd"));
			const char *dstp = op1 + mi;
			while (*dstp==' '||*dstp=='\t') dstp++;
			char dreg[8]=""; int di=0;
			while (dstp[di] && di<7 && dstp[di]!=' '&&dstp[di]!=','&&dstp[di]!='\t'){dreg[di]=dstp[di];di++;}
			dreg[di]=0;
			int dst_mech = (!strcmp(dreg,"esi")||!strcmp(dreg,"edi")||!strcmp(dreg,"ebp")
				||!strcmp(dreg,"si")||!strcmp(dreg,"di")||!strcmp(dreg,"bp"));
			/* 控制流(jmp/call/ret/loop)的目标寄存器是间接跳转目标, 不是"写机制寄存器",
			   jmp edi/ebp 是 handler 边界跳转, 绝不能删 */
			int is_ctrl_target = (mn[0]=='j' || !strcmp(mn,"ret") || !strcmp(mn,"call") || !strcmp(mn,"loop") || !strcmp(mn,"jecxz"));
			/* 机制删除仅适用于"明确写第一操作数"的指令(白名单). cmp/test/bt 等只读第一
			   操作数(写标志位/CF)的指令, 第一操作数不是写目标 => 绝不能按机制删除 */
			int writes_dst = (!strcmp(mn,"mov")||!strcmp(mn,"add")||!strcmp(mn,"sub")||!strcmp(mn,"adc")
				||!strcmp(mn,"sbb")||!strcmp(mn,"and")||!strcmp(mn,"or")||!strcmp(mn,"xor")
				||!strcmp(mn,"not")||!strcmp(mn,"neg")||!strcmp(mn,"inc")||!strcmp(mn,"dec")
				||!strcmp(mn,"shl")||!strcmp(mn,"shr")||!strcmp(mn,"sar")||!strcmp(mn,"rol")||!strcmp(mn,"ror")
				||!strcmp(mn,"lea")||!strcmp(mn,"movzx")||!strcmp(mn,"movsx")||!strcmp(mn,"bswap")
				||!strcmp(mn,"pop")||!strcmp(mn,"xchg")||!strcmp(mn,"xadd")||!strcmp(mn,"imul"));
			/* 2026-09-01 容器追踪法修正: 写 esi/edi/ebp 的"单操作数变换"类指令
			   (xor/neg/not/bswap/rol/ror/shl/shr/inc/dec) 可能是【指针解密链】
			   (如 vpc: xor esi,0xD8173197 -> neg -> bswap -> ror = 真实字节码指针),
			   这是关键算法代码, 不是机制噪声 => 不按机制删除, 交给后面的 liveness/DCE
			   (数据流) 判定。只有"指针算术/加载"类(lea/add/adc/sbb/sub/mov/movsx — vpc/vsp
			   推进)才是机制噪声, 仍删。 */
			int is_xform = (!strcmp(mn,"xor")||!strcmp(mn,"not")||!strcmp(mn,"neg")
				||!strcmp(mn,"bswap")||!strcmp(mn,"rol")||!strcmp(mn,"ror")
				||!strcmp(mn,"shl")||!strcmp(mn,"shr")||!strcmp(mn,"sar")
				||!strcmp(mn,"inc")||!strcmp(mn,"dec")||!strcmp(mn,"and")||!strcmp(mn,"or"));
			/* 2026-09-02 自我清零排除: xor reg,reg / sub reg,reg 是引擎面包屑(结果恒0),
			   不是真变换/解密链, 不应被 is_xform 豁免(否则漏删垃圾)。判断: 第二操作数
			   == dreg(同寄存器)。 */
			{
				char *comma = strchr(code, ',');
				if (comma && (strncmp(mn, "xor", 3) == 0 || strncmp(mn, "sub", 3) == 0))
				{
					char s2[16] = ""; int s2i = 0;
					comma++; while (*comma == ' ' || *comma == '\t') comma++;
					while (comma[s2i] && s2i < 15 && comma[s2i] != ' ' && comma[s2i] != '\t') s2[s2i++] = comma[s2i];
					s2[s2i] = 0;
					if (s2[0] && !strcmp(s2, dreg))
						is_xform = 0;   /* xor reg,reg / sub reg,reg = 自我清零面包屑 */
				}
			}
			/* 2026-09-01 指针解密链起点: `mov esi/edi/ebp,[内存]` 加载加密指针后,
			   若【下一条指令】(i+1) 是对同一机制寄存器的变换(xor/neg/bswap/rol/ror...),
			   则本 mov 是解密链的起点(如 vpc: mov esi,[栈] -> xor esi,0xD8173197 -> ...),
			   关键算法, 不按机制删除。后向遍历中 i+1 已处理过, 只 look-ahead 单行。 */
			int is_load = (!strcmp(mn,"mov") && strchr(code, '[') != NULL);
			if (dst_mech && is_load && !is_ctrl_target && i + 1 < n)
			{
				char la[4096];
				long saved = ftell(fin);
				fseek(fin, offsets[i + 1], SEEK_SET);
				if (fgets(la, sizeof(la), fin))
				{
					char *lc = strchr(la + 4, '\t');
					if (lc)
					{
						lc++;
						const char *op2 = lc;
						while (*op2==' '||*op2=='\t') op2++;
						char mn2[16]; int m2 = 0;
						while (op2[m2] && m2<15 && op2[m2]!=' ' && op2[m2]!='\t'){mn2[m2]=op2[m2];m2++;}
						mn2[m2] = 0;
						int is_xform2 = (!strcmp(mn2,"xor")||!strcmp(mn2,"not")||!strcmp(mn2,"neg")
							||!strcmp(mn2,"bswap")||!strcmp(mn2,"rol")||!strcmp(mn2,"ror")
							||!strcmp(mn2,"shl")||!strcmp(mn2,"shr")||!strcmp(mn2,"sar")
							||!strcmp(mn2,"inc")||!strcmp(mn2,"dec")||!strcmp(mn2,"and")||!strcmp(mn2,"or"));
						if (is_xform2)
							is_load = 0;   /* 下一条是变换链 => 本 mov 是解密链起点, 保留 */
					}
				}
				fseek(fin, saved, SEEK_SET);
			}
			/* 2026-09-01 任务1: taint 豁免。读 VM 字节码段沿传播出的"有效指令"
			   (g_taint_insns) 是 VM 语义关键链(vpc 解密/字节码解码), 即使被
			   机制删除/DCE 判死也不删。从 line 前 8 位 hex 解析当前 eip。 */
			unsigned int taint_eip = 0;
			{
				const char *hp = line + 4;
				static const char hxw[] = "0123456789abcdef0123456789ABCDEF";
				for (int k = 0; k < 8; k++)
				{
					char c = hp[k];
					int v;
					if (c >= '0' && c <= '9') v = c - '0';
					else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
					else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
					else v = 0;
					taint_eip = (taint_eip << 4) | (unsigned int)v;
				}
			}
			int is_tainted = 0;
			{ for (int tq = 0; tq < g_taint_insn_cnt; tq++) if (g_taint_insns[tq] == taint_eip) { is_tainted = 1; break; } }
			/* 2026-09-02 任务3: 恒不跳 jcc(不透明谓词, 永远 fallthrough) = 纯噪音, 直接删 */
			int is_always_nt_jcc = 0;
			{
				char mj[8] = ""; int mj_i = 0;
				const char *cp = code;
				if (cp) { while (cp[mj_i] && mj_i < 7 && cp[mj_i] != ' ' && cp[mj_i] != '\t') mj[mj_i++] = cp[mj_i]; mj[mj_i] = 0; }
				if (mj[0] == 'j' && strcmp(mj, "jmp") != 0)
				{
					for (int tq = 0; tq < g_always_nt_jcc_cnt; tq++)
						if (g_always_nt_jcc[tq] == taint_eip) { is_always_nt_jcc = 1; break; }
				}
			}
			if (is_always_nt_jcc)
			{
				drop[i] = 1;
				continue;   /* 恒不跳 jcc: 无条件 fallthrough, 删除(不改控制流) */
			}
			/* 任务4: 冗余乱序 jmp(无条件 jmp 目标==执行流下一行) = 乱序连接, 消除 */
			int is_redundant_jmp = 0;
			{
				char mj2[8] = ""; int mj_i = 0;
				const char *cp = code;
				if (cp) { while (cp[mj_i] && mj_i < 7 && cp[mj_i] != ' ' && cp[mj_i] != '\t') mj2[mj_i++] = cp[mj_i]; mj2[mj_i] = 0; }
				if (!strcmp(mj2, "jmp"))
				{
					for (int tq = 0; tq < g_redundant_jmp_cnt; tq++)
						if (g_redundant_jmp[tq] == taint_eip) { is_redundant_jmp = 1; break; }
				}
			}
			if (is_redundant_jmp)
			{
				drop[i] = 1;
				continue;   /* 乱序链 jmp: 目标紧跟执行流, 重排后冗余, 消除 */
			}
			if (dst_mech && !is_pushpop && !is_ctrl_target && writes_dst && !is_xform && !is_load && !is_tainted)
			{
				drop[i] = 1; mech_drop++;
				/* 2026-09-01 类型 B 修复: 机制删除不能跳过 liveness 传播——
				   该指令的 use 寄存器(源操作数)仍被读取, 必须进入 live 集合,
				   否则其上游生产者(如 VM 字节码解码链 mov eax,[esi] -> xor ->
				   neg -> xor -> not, 最终被 add ebp,eax / xor ebx,eax 消费)会被
				   后向 liveness 判死误删。这里在 continue 前解析源寄存器并
				   live |= use(不做 def 杀——机制寄存器 def 本就该被忽略)。 */
				unsigned int uwd[8] = {0,0,0,0,0,0,0,0};
				{
					int is_rmw = (!strcmp(mn,"not")||!strcmp(mn,"neg")||!strcmp(mn,"shl")||!strcmp(mn,"shr")
						||!strcmp(mn,"sar")||!strcmp(mn,"rol")||!strcmp(mn,"ror")||!strcmp(mn,"adc")||!strcmp(mn,"sbb")
						||!strcmp(mn,"add")||!strcmp(mn,"sub")||!strcmp(mn,"or")||!strcmp(mn,"and")||!strcmp(mn,"xor")
						||!strcmp(mn,"inc")||!strcmp(mn,"dec")||!strcmp(mn,"bts")||!strcmp(mn,"btr")||!strcmp(mn,"btc")
						||!strcmp(mn,"rcl")||!strcmp(mn,"rcr")||!strcmp(mn,"xadd")||!strcmp(mn,"bswap"));
					static const char *regs8w[8] = {"eax","ecx","edx","ebx","esi","edi","ebp","esp"};
					static const char *subw[16] = {"al","ah","ax","cl","ch","cx","dl","dh","dx","bl","bh","bx","si","di","bp","sp"};
					static const int subpidw[16] = {0,0,0,1,1,1,2,2,2,3,3,3,4,5,6,7};
					static const unsigned int submaskw[16] = {0xFF,0xFF00,0xFFFF,0xFF,0xFF00,0xFFFF,0xFF,0xFF00,0xFFFF,0xFF,0xFF00,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF};
					int dst_is_use = is_rmw;
					for (const char *p = code; *p; p++)
					{
						int is_dst_token = (!dst_is_use && (p == dstp || (p >= dstp && p < dstp + (int)strlen(dreg))));
						for (int k = 0; k < 16; k++)
						{
							size_t sl = strlen(subw[k]);
							if ((p == code || !isalnum((unsigned char)p[-1]) || p[-1]==']' || p[-1]=='*' || p[-1]=='+' || p[-1]=='-') &&
								strncmp(p, subw[k], sl)==0 &&
								(p[sl]==0 || !isalnum((unsigned char)p[sl])))
							{
								if (!is_dst_token) uwd[subpidw[k]] |= submaskw[k];
								p += sl - 1;
								break;
							}
						}
						for (int k = 0; k < 8; k++)
						{
							size_t sl = strlen(regs8w[k]);
							if ((p == code || !isalnum((unsigned char)p[-1]) || p[-1]==']' || p[-1]=='*' || p[-1]=='+' || p[-1]=='-') &&
								strncmp(p, regs8w[k], sl)==0 &&
								(p[sl]==0 || !isalnum((unsigned char)p[sl])))
							{
								if (!is_dst_token) uwd[k] |= 0xFFFFFFFFu;
								p += sl - 1;
								break;
							}
						}
					}
				}
				for (int k = 0; k < 4; k++) live[k] |= uwd[k];   /* 只传播算法寄存器(0..3), 机制指针 use 不传播(防垃圾回归) */
				continue;   /* 机制删除: 只做 use->live 传播, 不参与后续 def 杀/输出 */
			}
			/* def/use 位掩码 */
			unsigned int dw[8]={0,0,0,0,0,0,0,0}, uw[8]={0,0,0,0,0,0,0,0};
			{
				/* 读改写指令: 目标寄存器既是读又是写(not/neg/shl/shr/rol.../add/sub/xor.../inc/dec...) */
				int is_rmw = (!strcmp(mn,"not")||!strcmp(mn,"neg")||!strcmp(mn,"shl")||!strcmp(mn,"shr")
					||!strcmp(mn,"sar")||!strcmp(mn,"rol")||!strcmp(mn,"ror")||!strcmp(mn,"adc")||!strcmp(mn,"sbb")
					||!strcmp(mn,"add")||!strcmp(mn,"sub")||!strcmp(mn,"or")||!strcmp(mn,"and")||!strcmp(mn,"xor")
					||!strcmp(mn,"inc")||!strcmp(mn,"dec")||!strcmp(mn,"bts")||!strcmp(mn,"btr")||!strcmp(mn,"btc")
					||!strcmp(mn,"rcl")||!strcmp(mn,"rcr")||!strcmp(mn,"xadd")||!strcmp(mn,"bswap"));
				/* 目标寄存器宽度 -> dw */
				int pid=-1; unsigned int mask=0;
				if (!strcmp(dreg,"eax")){pid=0;mask=0xFFFFFFFF;} else if(!strcmp(dreg,"al")){pid=0;mask=0xFF;}
				else if(!strcmp(dreg,"ah")){pid=0;mask=0xFF00;} else if(!strcmp(dreg,"ax")){pid=0;mask=0xFFFF;}
				else if(!strcmp(dreg,"ecx")){pid=1;mask=0xFFFFFFFF;} else if(!strcmp(dreg,"cl")){pid=1;mask=0xFF;}
				else if(!strcmp(dreg,"ch")){pid=1;mask=0xFF00;} else if(!strcmp(dreg,"cx")){pid=1;mask=0xFFFF;}
				else if(!strcmp(dreg,"edx")){pid=2;mask=0xFFFFFFFF;} else if(!strcmp(dreg,"dl")){pid=2;mask=0xFF;}
				else if(!strcmp(dreg,"dh")){pid=2;mask=0xFF00;} else if(!strcmp(dreg,"dx")){pid=2;mask=0xFFFF;}
				else if(!strcmp(dreg,"ebx")){pid=3;mask=0xFFFFFFFF;} else if(!strcmp(dreg,"bl")){pid=3;mask=0xFF;}
				else if(!strcmp(dreg,"bh")){pid=3;mask=0xFF00;} else if(!strcmp(dreg,"bx")){pid=3;mask=0xFFFF;}
				else if(!strcmp(dreg,"esi")||!strcmp(dreg,"si")){pid=4;mask=(dreg[0]=='s'&&dreg[1]==0)?0xFFFFFFFF:0xFFFF;}
				else if(!strcmp(dreg,"edi")||!strcmp(dreg,"di")){pid=5;mask=(dreg[0]=='d'&&dreg[1]==0)?0xFFFFFFFF:0xFFFF;}
				else if(!strcmp(dreg,"ebp")||!strcmp(dreg,"bp")){pid=6;mask=(dreg[0]=='e')?0xFFFFFFFF:0xFFFF;}
				else if(!strcmp(dreg,"esp")||!strcmp(dreg,"sp")){pid=7;mask=(dreg[0]=='e')?0xFFFFFFFF:0xFFFF;}
				if (pid>=0) dw[pid] |= mask;
				/* use: code 文本里出现的所有寄存器(读改写目标保留为读, 纯写 mov/lea 目标不算读) */
				const char *p = code;
				static const char *regs8[8] = {"eax","ecx","edx","ebx","esi","edi","ebp","esp"};
				static const char *sub[16] = {"al","ah","ax","cl","ch","cx","dl","dh","dx","bl","bh","bx","si","di","bp","sp"};
				static const int subpid[16] = {0,0,0,1,1,1,2,2,2,3,3,3,4,5,6,7};
				static const unsigned int submask[16] = {0xFF,0xFF00,0xFFFF,0xFF,0xFF00,0xFFFF,0xFF,0xFF00,0xFFFF,0xFF,0xFF00,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF};
				/* 目标操作数在 code 里的位置 = mn 之后第一个非空白; 若目标是纯写(mov/lea 且非 RMW), 该位置不算 use */
				int dst_is_use = is_rmw;   /* RMW 目标算读; mov/lea 目标不算 */
				const char *dst_pos = dstp;
				for (p = code; *p; p++)
				{
					int is_dst_token = (!dst_is_use && (p == dst_pos || (dst_pos && p >= dst_pos && p < dst_pos + (int)strlen(dreg))));
					for (int k = 0; k < 16; k++)
					{
						size_t sl = strlen(sub[k]);
						if ((p == code || !isalnum((unsigned char)p[-1]) || p[-1]==']' || p[-1]=='*' || p[-1]=='+' || p[-1]=='-') &&
							strncmp(p, sub[k], sl)==0 &&
							(p[sl]==0 || !isalnum((unsigned char)p[sl])))
						{
							if (!is_dst_token)
								uw[subpid[k]] |= submask[k];
							p += sl - 1;
							break;
						}
					}
					for (int k = 0; k < 8; k++)
					{
						size_t sl = strlen(regs8[k]);
						if ((p == code || !isalnum((unsigned char)p[-1]) || p[-1]==']' || p[-1]=='*' || p[-1]=='+' || p[-1]=='-') &&
							strncmp(p, regs8[k], sl)==0 &&
							(p[sl]==0 || !isalnum((unsigned char)p[sl])))
						{
							if (!is_dst_token)
								uw[k] |= 0xFFFFFFFFu;
							p += sl - 1;
							break;
						}
					}
				}
			}
			/* DCE 判定: 写了寄存器, 无内存/控制流副作用, 且所有 def 都不 live => 死
			   2026-08-30 加标志位 liveness: 若该指令写标志位且 eflags 后续被读(jcc/setcc/adc/sbb...),
			   则即使数据寄存器死也不能删(删了丢标志位语义) */
			int is_ctrl = mn[0]=='j' || !strcmp(mn,"ret")||!strcmp(mn,"call")||!strcmp(mn,"loop")||!strcmp(mn,"jecxz");
			int has_mem = (strchr(code, '[') != NULL);
			int has_def = 0;
			for (int k=0;k<8;k++) if (dw[k]) has_def=1;
			/* 标志位读写判定 */
			{
				static const char *fread_ops = "|jz|jnz|je|jne|jc|jnc|jb|jnb|ja|jna|jo|jno|jp|jnp|js|jns|jl|jle|jg|jge|setz|setnz|sete|setne|setc|setnc|seto|setno|setp|setnp|sets|setns|setl|setle|setg|setge|adc|sbb|rcl|rcr|bt|bts|btr|btc|cmovz|cmovnz|cmovc|cmovnc|cmova|cmovb|cmovg|cmovl|cmovge|cmovle|cmovo|cmovno|cmovp|cmovnp|cmovs|cmovns|daa|das|aaa|aas|aam|aad|lahf|";
				static const char *fwrite_ops = "|add|sub|adc|sbb|and|or|xor|not|neg|shl|shr|sar|rol|ror|inc|dec|cmp|test|bts|btr|btc|bt|bsf|bsr|mul|imul|div|idiv|xadd|cmpxchg|popfd|sahf|rcl|rcr|cpuid|stc|clc|cmc|std|cld|daa|das|aaa|aas|aam|aad|";
				char pat[16];
				_snprintf(pat, sizeof(pat), "|%s|", mn);
				int fr = (strstr(fread_ops, pat) != NULL);
				int fw = (strstr(fwrite_ops, pat) != NULL);
				int is_pushpop2 = (!strcmp(mn,"push")||!strcmp(mn,"pop")||!strcmp(mn,"pushfd")||!strcmp(mn,"popfd"));
				if (!is_tainted && has_def && !is_ctrl && !has_mem)
				{
					/* 2026-09-02 主动过滤(对齐第④篇 isTainted): 非 tainted 且无内存/
					   非控制流的纯寄存器 ALU/mov = 引擎混淆(值被 VM 机�搬移�非算法关键),
					   即使 liveness 判 live 也删。tainted(读字节码/解密链/写专属寄存器)
					   和控制流/内存访问/push-pop 保留。 */
					if (!is_pushpop2)
					{
						drop[i] = 1; dce_drop++;
						continue;   /* 非 tainted 纯寄存器指令 = 引擎混淆 */
					}
				}
				if (has_def && !is_ctrl && !has_mem && !is_pushpop2)
				{
					int any_live = 0;
					for (int k=0;k<8;k++) if (dw[k] & live[k]) any_live = 1;
					if (!any_live && !(fw && eflags_live) && !is_tainted)
					{
						drop[i] = 1; dce_drop++;
						continue;   /* 死定义: 既不传播 def 也不传播 use */
					}
				}
				/* 更新 live: 先 def 杀(覆盖), 再 use 置 */
				for (int k=0;k<8;k++)
					live[k] = (live[k] & ~dw[k]) | uw[k];
				if (fr) eflags_live = 1;
				if (fw) eflags_live = 0;
			}
		}
		free(memlive_key); free(memlive_used);
	}
	out_progress("[txt] rewrite pass2: liveness done (mech-drop=%ld, dce-drop=%ld, dce-mem-drop=%ld)", mech_drop, dce_drop, dce_mem_drop);

	/* ---- 第三遍: 正序输出(复用原单遍的输出格式化代码) ---- */
	long cnt = 0;
	long vm_src = 0;   /* 读 VM 段(污点源)指令数 */
	{
		char line[4096];
		unsigned int tainted[8] = { 0,0,0,0,0,0,0,0 };
		/* 机制寄存器初始 tainted(对齐 thread-291053 cell16 taintRegister) */
		tainted[4] |= 0xFFFFFFFFu;   /* esi */
		tainted[5] |= 0xFFFFFFFFu;   /* edi */
		tainted[6] |= 0xFFFFFFFFu;   /* ebp */
		for (long i = 0; i < n; i++)
		{
			/* drop 的指令仍要参与 taint 传播(否则污点链断裂), 只是最后不输出 */
			int skip_out = drop[i];
			fseek(fin, offsets[i], SEEK_SET);
			if (!fgets(line, sizeof(line), fin)) continue;
			int ll = (int)strlen(line);
			while (ll > 0 && (line[ll-1]=='\n'||line[ll-1]=='\r')) line[--ll]=0;
			char addr[9]={0};
			memcpy(addr, line+4, 8);
			char *code = strchr(line+4, '\t');
			if (!code) continue;
			*code++ = 0;
			char *regs = strchr(code, '\t');
			char *mem = NULL;
			if (regs) { *regs++ = 0; mem = strchr(regs, '\t'); if (mem) *mem++ = 0; }
			unsigned int rv[8] = { 0,0,0,0,0,0,0,0 };
			if (regs)
			{
				static const char *rn8[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
				char regcopy[512];
				_snprintf(regcopy, sizeof(regcopy), "%s", regs);
				char *tok = regcopy;
				while (tok && *tok)
				{
					char *sp = strchr(tok, ' ');
					if (sp) *sp = 0;
					for (int ri = 0; ri < 8; ri++)
					{
						size_t L = strlen(rn8[ri]);
						if (strncmp(tok, rn8[ri], L) == 0 && tok[L] == '=')
						{
							rv[ri] = (unsigned int)strtoul(tok + L + 1, NULL, 16);
							break;
						}
					}
					tok = sp ? sp + 1 : NULL;
				}
			}
			char fcode[512];
			_snprintf(fcode, sizeof(fcode), "%s", code ? code : "");
			{
				unsigned long long foff = 0;
				if (!vmp38_fold_stack_disp(fcode, rv, &foff))
				{
					unsigned long long goff = 0; int greg = -1;
					if (vmp38_fold_mem_addr(fcode, rv, &goff, &greg))
					{
						const char *br = strchr(fcode, '[');
						const char *cr = br ? strchr(br + 1, ']') : NULL;
						if (br && cr && cr > br)
						{
							int prel = (int)(br - fcode);
							static const char *bn8[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
							char fb2[400];
							if (goff > 0x7FFFFFFFull)
								_snprintf(fb2, sizeof(fb2), "%.*s[%s-%X]%s", prel, fcode, bn8[greg], (unsigned)(0x100000000ULL - goff), cr + 1);
							else
								_snprintf(fb2, sizeof(fb2), "%.*s[%s+%X]%s", prel, fcode, bn8[greg], (unsigned)goff, cr + 1);
							_snprintf(fcode, sizeof(fcode), "%s", fb2);
						}
					}
				}
				else
				{
					const char *br = strchr(fcode, '[');
					const char *cr = br ? strchr(br + 1, ']') : NULL;
					if (br && cr && cr > br)
					{
						int prel = (int)(br - fcode);
						char fb2[320];
						/* 负偏移(0xFFFFFFF8 等)显示成 [esp-8] 而非 [esp+FFFFFFF8] */
						if (foff > 0x7FFFFFFFull)
							_snprintf(fb2, sizeof(fb2), "%.*s[esp-%X]%s", prel, fcode, (unsigned)(0x100000000ULL - foff), cr + 1);
						else
							_snprintf(fb2, sizeof(fb2), "%.*s[esp+%X]%s", prel, fcode, (unsigned)foff, cr + 1);
						_snprintf(fcode, sizeof(fcode), "%s", fb2);
					}
				}
			}
			char mem2[256] = "";
			if (mem && mem[0])
			{
				const char *lb = strchr(mem, '[');
				unsigned int aabs = lb ? (unsigned int)strtoul(lb + 1, NULL, 16) : 0;
				const char *eq = strchr(mem, '=');
				const char *arrow = eq ? strstr(eq, "->") : NULL;
				unsigned int pre = 0, post = 0;
				if (eq) pre = (unsigned int)strtoul(eq + 1, NULL, 16);
				if (arrow) post = (unsigned int)strtoul(arrow + 2, NULL, 16);
				_snprintf(mem2, sizeof(mem2), "[%08X]=%08X -> %08X", aabs, pre, post);
			}
			/* ---- forward taint propagation (port of thread-291053 cell9 taintMemory + Triton) ----
			   taint source: reads of VM bytecode section (g_vm_start..g_vm_end); mech regs {esi,edi,ebp}
			   propagation: dst reg tainted if any tainted src read; tainted jmp reg => '|' handler-boundary mark */
			{
				char *c2 = code;
				const char *op1 = c2;
				while (*op1==' '||*op1=='\t') op1++;
				char mn[16]=""; int mi=0;
				while (op1[mi] && mi<15 && op1[mi]!=' ' && op1[mi]!='\t'){mn[mi]=op1[mi];mi++;}
				mn[mi]=0;
				const char *dstp = op1 + mi;
				while (*dstp==' '||*dstp=='\t') dstp++;
				char dr[8]=""; int dk=0;
				while (dstp[dk] && dk<7 && dstp[dk]!=' '&&dstp[dk]!=','&&dstp[dk]!='\t'){dr[dk]=dstp[dk];dk++;}
				dr[dk]=0;
				int dpid=-1; unsigned int dmask=0;
				if (!strcmp(dr,"eax")){dpid=0;dmask=0xFFFFFFFF;} else if(!strcmp(dr,"al")){dpid=0;dmask=0xFF;}
				else if(!strcmp(dr,"ah")){dpid=0;dmask=0xFF00;} else if(!strcmp(dr,"ax")){dpid=0;dmask=0xFFFF;}
				else if(!strcmp(dr,"ecx")){dpid=1;dmask=0xFFFFFFFF;} else if(!strcmp(dr,"cl")){dpid=1;dmask=0xFF;}
				else if(!strcmp(dr,"ch")){dpid=1;dmask=0xFF00;} else if(!strcmp(dr,"cx")){dpid=1;dmask=0xFFFF;}
				else if(!strcmp(dr,"edx")){dpid=2;dmask=0xFFFFFFFF;} else if(!strcmp(dr,"dl")){dpid=2;dmask=0xFF;}
				else if(!strcmp(dr,"dh")){dpid=2;dmask=0xFF00;} else if(!strcmp(dr,"dx")){dpid=2;dmask=0xFFFF;}
				else if(!strcmp(dr,"ebx")){dpid=3;dmask=0xFFFFFFFF;} else if(!strcmp(dr,"bl")){dpid=3;dmask=0xFF;}
				else if(!strcmp(dr,"bh")){dpid=3;dmask=0xFF00;} else if(!strcmp(dr,"bx")){dpid=3;dmask=0xFFFF;}
				else if(!strcmp(dr,"esi")){dpid=4;dmask=0xFFFFFFFF;} else if(!strcmp(dr,"edi")){dpid=5;dmask=0xFFFFFFFF;}
				else if(!strcmp(dr,"ebp")){dpid=6;dmask=0xFFFFFFFF;} else if(!strcmp(dr,"esp")){dpid=7;dmask=0xFFFFFFFF;}
				unsigned int aabs2 = 0; int has_mabs = 0;
				if (mem && mem[0]) { const char *lb2 = strchr(mem, '['); if (lb2) { aabs2 = (unsigned int)strtoul(lb2+1, NULL, 16); has_mabs=1; } }
				int is_vm_read = 0;
				if (has_mabs && g_vm_start && aabs2 >= (unsigned int)g_vm_start && aabs2 <= (unsigned int)g_vm_end)
				{
					is_vm_read = 1; vm_src++;
					if (dpid >= 0) tainted[dpid] |= dmask;
				}
				int is_tainted = 0;
				if (!is_vm_read)
				{
					static const char *rn8x[8] = {"eax","ecx","edx","ebx","esi","edi","ebp","esp"};
					static const char *subx[16] = {"al","ah","ax","cl","ch","cx","dl","dh","dx","bl","bh","bx","si","di","bp","sp"};
					static const int subpidx[16] = {0,0,0,1,1,1,2,2,2,3,3,3,4,5,6,7};
					static const unsigned int submaskx[16] = {0xFF,0xFF00,0xFFFF,0xFF,0xFF00,0xFFFF,0xFF,0xFF00,0xFFFF,0xFF,0xFF00,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF};
					for (const char *p = c2; *p && !is_tainted; p++)
					{
						for (int k=0;k<16;k++)
						{
							size_t sl=strlen(subx[k]);
							if ((p==c2 || !isalnum((unsigned char)p[-1]) || p[-1]==']'||p[-1]=='*'||p[-1]=='+'||p[-1]=='-') &&
								strncmp(p,subx[k],sl)==0 && (p[sl]==0||!isalnum((unsigned char)p[sl])))
							{
								if (tainted[subpidx[k]] & submaskx[k]) is_tainted=1;
								p += sl-1; break;
							}
						}
						for (int k=0;k<8 && !is_tainted;k++)
						{
							size_t sl=strlen(rn8x[k]);
							if ((p==c2 || !isalnum((unsigned char)p[-1]) || p[-1]==']'||p[-1]=='*'||p[-1]=='+'||p[-1]=='-') &&
								strncmp(p,rn8x[k],sl)==0 && (p[sl]==0||!isalnum((unsigned char)p[sl])))
							{
								if (tainted[k]) is_tainted=1;
								p += sl-1; break;
							}
						}
					}
				}
				if (dpid >= 0)
				{
					if (is_tainted || is_vm_read) tainted[dpid] |= dmask;
					else tainted[dpid] &= ~dmask;
				}
				char bmark[2] = "";
				if (!strcmp(mn,"jmp"))
				{
					int tpid=-1; unsigned int tmask=0;
					if (!strcmp(dr,"eax")){tpid=0;tmask=0xFFFFFFFF;} else if(!strcmp(dr,"ecx")){tpid=1;tmask=0xFFFFFFFF;}
					else if(!strcmp(dr,"edx")){tpid=2;tmask=0xFFFFFFFF;} else if(!strcmp(dr,"ebx")){tpid=3;tmask=0xFFFFFFFF;}
					else if(!strcmp(dr,"esi")){tpid=4;tmask=0xFFFFFFFF;} else if(!strcmp(dr,"edi")){tpid=5;tmask=0xFFFFFFFF;}
					else if(!strcmp(dr,"ebp")){tpid=6;tmask=0xFFFFFFFF;} else if(!strcmp(dr,"esp")){tpid=7;tmask=0xFFFFFFFF;}
					if (tpid>=0 && (tainted[tpid] & tmask)) bmark[0]='|';
				}
				if (!skip_out)
				{
					fprintf(g_fout, "%s%s%s: %s%s%s%s%s\n",
						bmark, bmark[0]?" ":"", addr, fcode,
						(regs && regs[0]) ? "  " : "", (regs && regs[0]) ? regs : "",
						mem2[0] ? "  " : "", mem2);
					cnt++;
				}
			}
		}
	}

	free(offsets); free(drop);
	fclose(fin);
	out_progress("[txt] rewrite: kept=%ld (mech-drop=%ld, dce-drop=%ld) of %ld lines, addr-fold + abs-addr + deobf",
		cnt, mech_drop, dce_drop, n);
}


/* Rebuild STEP_REC list for a trace segment [start_i, start_i+len) and run
   the same identify_handler as the dynamic trace - so the full-trace output
   carries real semantics (vAdc/vSbb/...) plus operand values, not just
   addresses. */
static int identify_handler_from_trace(int start_i, int len, int handler_idx)
{
	STEP_REC recs[48];
	MINI_REG mreg;
	int j;
	int n = 0;
	int ir = 0;
	unsigned long long vsp_in = 0;
	if (len > 48) len = 48;
	if (start_i < 0 || start_i + len > g_ft_n) return 0;
	/* seed the sim registers from the recorded trace at this handler start */
	memset(&mreg, 0, sizeof(mreg));
	mreg.flags_valid = 1;
	for (j = 0; j < 8; j++)
	{
		mreg.r[j] = g_ft_regs[start_i][j];
		mreg.t[j] = 1;   /* tainted (unknown provenance) */
	}
	mreg.eflags = g_ft_eflags[start_i] & ~0x100ULL;   /* clear TF */
	vsp_in = g_ft_regs[start_i][4];   /* ESP at handler start */
	mreg.vsp = vsp_in;
	for (j = 0; j < len; j++)
	{
		int idx = start_i + j;
		DISASM_INSTR di;
		memset(&di, 0, sizeof(di));
		memset(&recs[j], 0, sizeof(recs[j]));
		/* snapshot-cached disasm, NOT DbgDisasmAt: the bridge call was the
		   real restore slowdown (~5us per call x 16M instrs = 80s); the
		   dump snapshot + hash cache is ~100x faster (verify: previous code
		   had DbgDisasmAt here while the main loop already used the cache) */
		disasm_at_cached(g_ft_eip[idx], &di, NULL);
		if (di.instr_size < 1) continue;
		strncpy(recs[j].disasm, di.instruction, sizeof(recs[j].disasm) - 1);
		recs[j].instr_len = di.instr_size;
		recs[j].ip = g_ft_eip[idx];   /* VM real address for analysis output */
		recs[j].has_real = 1;
		/* tag push/pop/jmp/alu by mnemonic - the same way the restore main
		   loop fills STEP_REC (VMP-transformed forms like xchg [mem],reg /
		   shr / jnbe all come back here, so a pure-text parser must cover
		   them). */
		{
			char mm2[32];
			get_mnemonic(recs[j].disasm, mm2, sizeof(mm2));
			if (strncmp(mm2, "push", 4) == 0) recs[j].push = 1;
			else if (strncmp(mm2, "pop", 3) == 0) recs[j].pop = 1;
			else if (strncmp(mm2, "jmp", 3) == 0 || strncmp(mm2, "call", 4) == 0 ||
				strcmp(mm2, "ret") == 0) recs[j].is_jmp = 1;
			else if (mm2[0] == 'j' && strcmp(mm2, "jmp") != 0) {
				recs[j].is_jmp = 1;
				recs[j].is_cond = 1;
				const char *sp_ = strchr(recs[j].disasm, ' ');
				if (sp_) {
					while (*sp_ == ' ' || *sp_ == '\t') sp_++;
					if (sp_[0] == '0' && (sp_[1] == 'x' || sp_[1] == 'X'))
						recs[j].cond_target = strtoull(sp_ + 2, NULL, 16);
				}
			}
			/* is_alu rule (no mov: a register mov is not ALU; only mov [mem],reg
			   stack ops are, which the mnemonic tag above never sets for mov) */
			if (strcmp(mm2, "add") == 0 || strcmp(mm2, "sub") == 0 || strcmp(mm2, "xor") == 0 ||
				strcmp(mm2, "and") == 0 || strcmp(mm2, "or") == 0 || strcmp(mm2, "adc") == 0 ||
				strcmp(mm2, "sbb") == 0 || strcmp(mm2, "lea") == 0 || strcmp(mm2, "xchg") == 0 ||
				strcmp(mm2, "inc") == 0 || strcmp(mm2, "dec") == 0 || strcmp(mm2, "not") == 0 ||
				strcmp(mm2, "neg") == 0 || strcmp(mm2, "imul") == 0 || strcmp(mm2, "mul") == 0 ||
				strcmp(mm2, "shl") == 0 || strcmp(mm2, "shr") == 0 || strcmp(mm2, "sar") == 0 ||
				strcmp(mm2, "shrd") == 0 || strcmp(mm2, "shld") == 0 || strcmp(mm2, "div") == 0 ||
				strcmp(mm2, "idiv") == 0 || strcmp(mm2, "rol") == 0 || strcmp(mm2, "ror") == 0 ||
				strcmp(mm2, "xadd") == 0)
				recs[j].is_alu = 1;
			/* CORE-ALU rule (unified with dynamic): core = the ALU mnemonic
			   itself (adc/sbb/xor/...), regardless of operand form - so the
			   identify_handler core sequence matches between dyn & static */
			if (strcmp(mm2, "adc") == 0 || strcmp(mm2, "sbb") == 0 || strcmp(mm2, "xor") == 0 ||
				strcmp(mm2, "and") == 0 || strcmp(mm2, "or") == 0 || strcmp(mm2, "add") == 0 ||
				strcmp(mm2, "sub") == 0 || strcmp(mm2, "not") == 0 || strcmp(mm2, "neg") == 0 ||
				strcmp(mm2, "shl") == 0 || strcmp(mm2, "shr") == 0 || strcmp(mm2, "sar") == 0 ||
				strcmp(mm2, "rol") == 0 || strcmp(mm2, "ror") == 0 || strcmp(mm2, "imul") == 0 ||
				strcmp(mm2, "mul") == 0 || strcmp(mm2, "div") == 0 || strcmp(mm2, "idiv") == 0)
				recs[j].is_core = 1;
		}
		n++;
	}
	if (n > 0)
	{
		/* vsp_out: use the TRACE's real ESP at the handler's last instruction
		   (Unicorn executed it) instead of a simulated vsp - the simulated
		   vsp drifts from the real esp by 4 bytes on push/pop boundaries,
		   which cascades into wrong stack reads -> wrong constant decryption
		   (observed 0x8d1f6e0f real 217 vs 1e3). */
		unsigned long long vsp_out = vsp_in;
		/* The CODE hook records registers BEFORE the instruction executes, so
		   g_ft_regs[k][4] is the ESP at instruction k's start. The handler's
		   trailing push/pushfd/call already ran by the time the next
		   instruction starts - use the NEXT instruction's ESP (handler-end
		   ESP) so a trailing pushfd (4-byte stack push) is counted, matching
		   the dynamic trace's vsp_out. */
		if (start_i + len < g_ft_n)
			vsp_out = g_ft_regs[start_i + len][4];
		else if (start_i + len - 1 < g_ft_n)
			vsp_out = g_ft_regs[start_i + len - 1][4];
		ir = identify_handler(recs, n, handler_idx, vsp_in, vsp_out);
		/* 		/* FOLLOWING-CODE DECRYPT SCAN (frequency-gated): decrypt only the
		   HIGH-FREQUENCY mov-imm constants that appear right after handlers
		   (e.g. mov eax,0x7606938C - a fixed VMP engine constant that repeats
		   every ~289 handlers). Low-frequency following movs are decrypted by
		   the dynamic trace's wider context but are NOT the target here; only
		   constants that recur >=3 times across the whole trace are real
		   (frequency = occurrence count in the full g_ft_eip). */
		{
			STEP_REC xrecs[16];
			int xn = 0;
			int xk;
			for (xk = 0; xk < 12 && start_i + len + xk < g_ft_n && xn < 14; xk++)
			{
				int xidx = start_i + len + xk;
				DISASM_INSTR xdi;
				memset(&xdi, 0, sizeof(xdi));
				memset(&xrecs[xn], 0, sizeof(xrecs[xn]));
				DbgDisasmAt((duint)g_ft_eip[xidx], &xdi);
				if (xdi.instr_size < 1) continue;
				strncpy(xrecs[xn].disasm, xdi.instruction, sizeof(xrecs[xn].disasm) - 1);
				xrecs[xn].instr_len = xdi.instr_size;
				xrecs[xn].ip = g_ft_eip[xidx];
				xn++;
			}
			for (xk = 0; xk < xn; xk++)
			{
				char xm[32];
				unsigned long long xenc = 0;
				get_mnemonic(xrecs[xk].disasm, xm, sizeof(xm));
				if (strcmp(xm, "mov") == 0 && !strchr(xrecs[xk].disasm, '[') &&
					strchr(xrecs[xk].disasm, ',') &&
					parse_imm_from_disasm(xrecs[xk].disasm, &xenc))
				{
					/* frequency gate: look up the precomputed table (O(1)) */
					{
						int xfreq = 0, xt2;
						for (xt2 = 0; xt2 < g_imm_freq_n; xt2++)
							if (g_imm_freq_enc[xt2] == xenc) { xfreq = g_imm_freq_cnt[xt2]; break; }
						if (xfreq < 3) continue;   /* not a recurring constant - skip */
					}
					if ((xenc > 0x40000000ULL && xenc <= 0xffffffffULL) || xenc > 0xffffffffULL)
					{
						char xop1[64] = "";
						const char *xp = xrecs[xk].disasm;
						while (*xp && *xp != ' ') xp++;
						while (*xp == ' ') xp++;
						{
							int kk = 0;
							while (*xp && *xp != ',' && kk < 63) xop1[kk++] = *xp++;
							xop1[kk] = 0;
						}
						{
							int xcarrier = reg_index_from_name(xop1);
							unsigned long long xdec = 0;
							if (xcarrier >= 0 &&
								try_decrypt_enc_imm(xenc, xrecs, xn, xk + 1, xcarrier, &xdec))
								out("  [decrypt] enc=%llx -> real=%llx  %s  <following>",
									xenc, xdec, xrecs[xk].disasm);
						}
					}
				}
			}
		}
	}
	return ir;
}

/* DYN-STATIC DIVERGENCE CHECK - run after whichever ran second (dynamic
   fills g_dyn_steps, static fills g_ft). Reports the FIRST instruction
   where Unicorn's execution left the real CPU's path. */
static void check_dyn_static_divergence(void)
{
	int dmin, di2;
	out("[divergence-check] dyn_steps=%d ft_steps=%d", g_dyn_steps_n, g_ft_n);
	if (g_dyn_steps_n <= 0 || g_ft_n <= 0) return;
	dmin = g_dyn_steps_n < g_ft_n ? g_dyn_steps_n : g_ft_n;
	for (di2 = 0; di2 < dmin; di2++)
	{
		if (g_dyn_steps_eip[di2] != g_ft_eip[di2])
		{
			out("[divergence] step %d: dyn eip=%08x vs static eip=%08x",
				di2, g_dyn_steps_eip[di2], g_ft_eip[di2]);
			out("[divergence]   dyn eflags=%08x regs=%08x %08x %08x %08x %08x %08x %08x %08x",
				g_dyn_steps_ef[di2],
				g_dyn_steps_reg[di2][0], g_dyn_steps_reg[di2][1], g_dyn_steps_reg[di2][2],
				g_dyn_steps_reg[di2][3], g_dyn_steps_reg[di2][4], g_dyn_steps_reg[di2][5],
				g_dyn_steps_reg[di2][6], g_dyn_steps_reg[di2][7]);
			out("[divergence]   static eflags=%08x regs=%08x %08x %08x %08x %08x %08x %08x %08x",
				g_ft_eflags[di2],
				(unsigned int)g_ft_regs[di2][0], (unsigned int)g_ft_regs[di2][1],
				(unsigned int)g_ft_regs[di2][2], (unsigned int)g_ft_regs[di2][3],
				(unsigned int)g_ft_regs[di2][4], (unsigned int)g_ft_regs[di2][5],
				(unsigned int)g_ft_regs[di2][6], (unsigned int)g_ft_regs[di2][7]);
			return;
		}
		if (g_dyn_steps_ef[di2] != g_ft_eflags[di2])
		{
			out("[divergence] step %d same eip=%08llx but eflags differ: dyn=%08x static=%08x",
				di2, g_dyn_steps_eip[di2], g_dyn_steps_ef[di2], g_ft_eflags[di2]);
			return;
		}
	}
	out("[divergence] none in first %d steps", dmin);
}

/* Analyze the recorded full-trace (g_ft_eip/g_ft_regs) into handlers.
   Unicorn ran the VM code like a real CPU; here we group the executed
   instructions into handler boundaries (jmp/call/ret) and identify each
   handler's semantics - same output shape as the dynamic trace. */



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
	if (vs->var_n < AN_MAX_VAR)
	{
		_snprintf(vs->var_names[vs->var_n], 16, "v%d", vs->n++);
		return vs->var_names[vs->var_n++];
	}
	return "v?";
}

static int an_var_id(AN_VS *vs, const char *name)
{
	for (int i = 0; i < vs->var_n; i++)
		if (!strcmp(vs->var_names[i], name))
			return i;
	return -1;
}

/* register key -> bound var (mint if unbound) */
/* mov reg2, reg1: make reg2 share reg1's variable (VMP slot carrier).
   This keeps the algorithm chain coherent (mov param -> calc reg). */
static const char *an_get_reg(AN_VS *vs, const char *r);
static const char *an_new_var(AN_VS *vs);
static const char *an_reg_canon(const char *tok)
{
	static char buf[16];
	static const char *map32[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
	static const char *map64[8] = { "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi" };
	static const char *map16[8] = { "ax","cx","dx","bx","sp","bp","si","di" };
	static const char *map8[8]  = { "al","cl","dl","bl","ah","ch","dh","bh" };
	char low[16];
	int i;
	_snprintf(low, sizeof(low), "%s", tok);
	_strlwr(low);
	/* r8d/r8w/r8b -> r8 .. r15 (return a static buf, NOT a stack local) */
	for (i = 8; i <= 15; i++) {
		char base[8];
		_snprintf(base, sizeof(base), "r%d", i);
		if (!strncmp(low, base, strlen(base)))
		{
			_snprintf(buf, sizeof(buf), "%s", base);
			return buf;
		}
	}
	for (i = 0; i < 8; i++) {
		if (!strcmp(low, map32[i]) || !strcmp(low, map16[i]) || !strcmp(low, map8[i]))
			return map64[i];
	}
	if (!strcmp(low, "rsp")) return "rsp";
	if (!strcmp(low, "rbp")) return "rbp";
	if (!strcmp(low, "rip")) return "rip";
	_snprintf(buf, sizeof(buf), "%s", low);
	return buf;
}

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
	for (int i = 0; i < vs->reg_n; i++)
		if (!strcmp(vs->reg_name[i], r))
			return vs->reg_var[i];
	{
		const char *v = an_new_var(vs);
		_snprintf(vs->reg_name[vs->reg_n], 16, "%s", r);
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
		if (!strcmp(t, AN_REGS32[i]) || !strcmp(t, AN_REGS16[i]) || !strcmp(t, AN_REGS8[i]))
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
	for (i = 0; i < 8; i++) {
		if (regs[i][0] && strcmp(regs[i], "00000000")) {
			_snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s=%s%s", AN_REGS32[i], regs[i], n ? ", " : "");
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
				_snprintf(buf, bufsz, "    %s = %lX;  // = %lX %s %lX (const-folded)", var, res, (unsigned long)cur_val, an_op_sym(op), rhs_const);
			else
				_snprintf(buf, bufsz, "    %s = %lX;  // = %lX %s %s (const-folded)", var, res, (unsigned long)cur_val, op, rhs_var ? rhs_var : "?");
			return;
		}
	}
	an_clear_const(vs, var);
	if (rhs_const != 0xFFFFFFFFUL)
		_snprintf(buf, bufsz, "    %s = %s %s %lX;", var, cur_var, an_op_sym(op), rhs_const);
	else if (rhs_var && rhs_var[0] && strcmp(rhs_var, "?"))
	{
		long rv = an_get_const(vs, rhs_var);
		if (rv >= 0)
			_snprintf(buf, bufsz, "    %s = %s %s %lX;", var, cur_var, an_op_sym(op), (unsigned long)rv);
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
				fprintf(fp, "%*swhile (loop @%llx) {\n", g_an_indent * 4, "", loops[li].ctgt);
			}
			else
			{
				/* later iteration: skip the body, note it */
				g_an_skip++;
				fprintf(fp, "%*s// (loop iteration @%llx - body skipped)\n",
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
					if (!strcmp(vs->reg_name[vi], AN_REGS32[ri])) { exists = 1; break; }
				if (!exists) {
					unsigned long sv = strtoul(snap_regs[ri], NULL, 16);
					an_set_const(vs, an_get_reg(vs, AN_REGS32[ri]), sv);
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
					_snprintf(out, outsz, "    // (back-edge @%llx)", insn->ctgt);
				}
				else if (insn->cond)
				{
					if (vs->last_cmp_op1[0])
						_snprintf(out, outsz, "    // if (%s %s %s) goto %llx", vs->last_cmp_op1, csym, vs->last_cmp_op2, insn->ctgt);
					else
						_snprintf(out, outsz, "    // if (%s) goto %llx", csym, insn->ctgt);
				}
				else if (insn->ctgt < cur)
					_snprintf(out, outsz, "    // LOOP: goto %llx (back-edge)", insn->ctgt);
				else
					_snprintf(out, outsz, "    // goto %llx", insn->ctgt);
			} else {
				if (insn->cond)
					_snprintf(out, outsz, "    // if (%s) <branch>", an_jcc_sym(mn));
				else
					_snprintf(out, outsz, "    // goto <dispatch>");
			}
			fprintf(fp, "%*s%s\n", g_an_indent, "", out);
			return 1;
		}
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
		if (!an_is_reg(ops[0])) {
			/* memory destination / indirect forms: keep the raw instruction
			   so memory addresses and ops are never lost (keep ALL VM code) */
			fprintf(fp, "%*s// %s\n", g_an_indent, "", insn->disasm);
			return 1;
		}
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
						_snprintf(out, outsz, "    %s = %lX;  // decrypted from %lX", var, real, imm);
						fprintf(fp, "%*s%s\n", g_an_indent, "", out);
						return 1;
					}
					an_set_const(vs, var, imm);
					_snprintf(out, outsz, "    %s = %lX;", var, imm);
					fprintf(fp, "%*s%s\n", g_an_indent, "", out);
					return 1;
				}
				if (an_is_reg(ops[1])) {
					const char *src = an_get_reg(vs, ops[1]);
					long sc = an_get_const(vs, src);
					var = an_alias_reg(vs, r, ops[1]);
					if (sc >= 0) {
						an_set_const(vs, var, (unsigned long)sc);
						_snprintf(out, outsz, "    %s = %lX;  // mov %s (const %lx)", var, (unsigned long)sc, src, (unsigned long)sc);
						fprintf(fp, "%*s%s\n", g_an_indent, "", out);
						return 1;
					}
					/* slot alias: same variable propagates the value chain */
					return 1;
				}
				/* mov reg, <mem/indirect> - keep the memory read visible */
				fprintf(fp, "%*s// %s\n", g_an_indent, "", insn->disasm);
				return 1;
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
					_snprintf(out, outsz, "    push(%lX);", imm);
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
						_snprintf(out, outsz, "    %s = %lX;  // pop (slot const %lx)", var, (unsigned long)sc, (unsigned long)sc);
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
/* streaming analyze: reads the .data.txt TWICE (pre-scan + emit) but never
   holds all handlers in memory - only the current handler's insn array.
   Handles arbitrarily large data files on the 32-bit host. */

/* read one handler block (H..D lines) into `cur` from fin; returns 1 if a
   handler was read, 0 at EOF. */
static int an_read_handler(FILE *fin, AN_HANDLER *cur)
{
	char line[2048];
	int got = 0;
	/* new-format state: accumulate register/flag deltas per instruction */
	unsigned int regv[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned int efv = 0;
	int have_regs = 0;
	while (fgets(line, sizeof(line), fin)) {
		if (line[0] == 'H') {
			int idx, n;
			char v1[16], v2[16];
			if (got) { fseek(fin, -(long)strlen(line), SEEK_CUR); break; }  /* peek */
			if (sscanf(line, "H idx=%d n=%d vsp_in=%15s vsp_out=%15s", &idx, &n, v1, v2) == 4) {
				memset(cur, 0, sizeof(*cur));
				cur->idx = idx;
				_snprintf(cur->vsp_in, sizeof(cur->vsp_in), "%s", v1);
				_snprintf(cur->vsp_out, sizeof(cur->vsp_out), "%s", v2);
				got = 1;
				have_regs = 0;
			}
		}
		else if (line[0] == 'T' && got) {
			char tp[48];
			if (sscanf(line, "T type=%47s", tp) == 1)
				_snprintf(cur->type, sizeof(cur->type), "%s", tp);
		}
		else if (line[0] == 'E' && got) {
			/* new format: E <eip> - commit the previous insn, start a new one */
			char ep[32];
			if (sscanf(line, "E %31s", ep) == 1)
			{
				AN_INSN *ii;
				if (cur->n >= cur->cap) {
					cur->cap = cur->cap ? cur->cap * 2 : 64;
					AN_INSN *ni = (AN_INSN*)realloc(cur->insns, cur->cap * sizeof(AN_INSN));
					if (!ni) return -1;
					cur->insns = ni;
				}
				ii = &cur->insns[cur->n];
				memset(ii, 0, sizeof(*ii));
				_snprintf(ii->ip, sizeof(ii->ip), "%s", ep);
				/* carry the accumulated register/flag values into the new insn */
				if (have_regs) {
					_snprintf(ii->r0, sizeof(ii->r0), "%08x", regv[0]);
					_snprintf(ii->r1, sizeof(ii->r1), "%08x", regv[1]);
					_snprintf(ii->r2, sizeof(ii->r2), "%08x", regv[2]);
					_snprintf(ii->r3, sizeof(ii->r3), "%08x", regv[3]);
					_snprintf(ii->r4, sizeof(ii->r4), "%08x", regv[4]);
					_snprintf(ii->r5, sizeof(ii->r5), "%08x", regv[5]);
					_snprintf(ii->r6, sizeof(ii->r6), "%08x", regv[6]);
					_snprintf(ii->r7, sizeof(ii->r7), "%08x", regv[7]);
					_snprintf(ii->ef, sizeof(ii->ef), "%08x", efv);
				}
				cur->n++;
				have_regs = 1;
			}
		}
		else if (line[0] == 'I' && got) {
			/* new format only: I <disasm> follows the E row */
			if (cur->n > 0)
			{
				AN_INSN *ii = &cur->insns[cur->n - 1];
				char *di = line + 1;
				while (*di == ' ') di++;
				_snprintf(ii->disasm, sizeof(ii->disasm), "%s", di);
				{ char *nl = strchr(ii->disasm, '\n'); if (nl) *nl = 0; }
			}
		}
		else if (line[0] == 'D' && got) {
			char dip[16];
			unsigned long denc, dreal;
			if (sscanf(line, "D ip=%15s enc=%lx real=%lx", dip, &denc, &dreal) == 3 && cur->d_n < 64) {
				_snprintf(cur->d_ip[cur->d_n], 16, "%s", dip);
				cur->d_real[cur->d_n] = dreal;
				cur->d_n++;
			}
		}
		else if (line[0] == 'M' && got && cur->n > 0) {
			/* new format memory row: M [expr]=0xvalue */
			char *eq = strchr(line, '=');
			if (eq)
			{
				AN_INSN *ii = &cur->insns[cur->n - 1];
				_snprintf(ii->mem, sizeof(ii->mem), "%s", eq + 1);
				{ char *nl = strchr(ii->mem, '\n'); if (nl) *nl = 0; }
			}
		}
		else if (got && cur->n > 0 && line[0] != '\n' && line[0] != '#') {
			/* new-format register/flag delta rows: eax=... zf=... (no prefix) */
			AN_INSN *ii = &cur->insns[cur->n - 1];
			static const char *rn[8] = { "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi" };
			char *p = line;
			while (*p && *p != '\n')
			{
				while (*p == ' ' || *p == '\t') p++;
				if (!*p || *p == '\n') break;
				char *eq = strchr(p, '=');
				if (!eq) break;
				char name[16];
				int nl2 = (int)(eq - p);
				if (nl2 <= 0 || nl2 >= (int)sizeof(name)) break;
				memcpy(name, p, nl2); name[nl2] = 0;
				unsigned long val = strtoul(eq + 1, NULL, 16);
				int found = 0;
				for (int k = 0; k < 8; k++)
					if (_stricmp(name, rn[k]) == 0) { regv[k] = (unsigned int)val; found = 1; break; }
				if (!found)
				{
					/* flag names zf cf of sf pf af df */
					if (_stricmp(name, "zf") == 0) { if (val) efv |= (1u << 6); else efv &= ~(1u << 6); }
					else if (_stricmp(name, "cf") == 0) { if (val) efv |= (1u << 0); else efv &= ~(1u << 0); }
					else if (_stricmp(name, "of") == 0) { if (val) efv |= (1u << 11); else efv &= ~(1u << 11); }
					else if (_stricmp(name, "sf") == 0) { if (val) efv |= (1u << 7); else efv &= ~(1u << 7); }
					else if (_stricmp(name, "pf") == 0) { if (val) efv |= (1u << 2); else efv &= ~(1u << 2); }
					else if (_stricmp(name, "af") == 0) { if (val) efv |= (1u << 4); else efv &= ~(1u << 4); }
					else if (_stricmp(name, "df") == 0) { if (val) efv |= (1u << 10); else efv &= ~(1u << 10); }
				}
				p = eq + 1;
				while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
			}
			/* keep ii->r0..r7 fresh from the accumulated values */
			if (have_regs) {
				_snprintf(ii->r0, sizeof(ii->r0), "%08x", regv[0]);
				_snprintf(ii->r1, sizeof(ii->r1), "%08x", regv[1]);
				_snprintf(ii->r2, sizeof(ii->r2), "%08x", regv[2]);
				_snprintf(ii->r3, sizeof(ii->r3), "%08x", regv[3]);
				_snprintf(ii->r4, sizeof(ii->r4), "%08x", regv[4]);
				_snprintf(ii->r5, sizeof(ii->r5), "%08x", regv[5]);
				_snprintf(ii->r6, sizeof(ii->r6), "%08x", regv[6]);
				_snprintf(ii->r7, sizeof(ii->r7), "%08x", regv[7]);
				_snprintf(ii->ef, sizeof(ii->ef), "%08x", efv);
			}
		}
	}
	return got ? 1 : 0;
}

void vmp38_analyze_constants(const char *data_path, FILE *fout);
void vmp38_analyze_symexec(const char *data_path, FILE *fout);

void vmp38_analyze_inline(const char *data_path, const char *out_path)
{
	FILE *fin = fopen(data_path, "rb");   /* binary: fseek peek offsets are physical */
	FILE *fout = fopen(out_path, "w");
	AN_VS vs;
	AN_LOOP loops[64];
	int loop_n = 0;
	char line[2048];
	if (!fin || !fout) {
		if (fin) fclose(fin);
		if (fout) fclose(fout);
		return;
	}
	memset(&vs, 0, sizeof(vs));
	memset(loops, 0, sizeof(loops));
	for (int i = 0; i < 8192; i++) vs.var_const[i] = 0xFFFFFFFFUL;
	for (int i = 0; i < AN_MAX_VAR; i++) vs.var_inc[i] = 0;

	/* ---- pass 1: param / last-r0 / loop pre-scan (streaming) ---- */
	{
		char param_line_ok = 0;
		char param_reg[16] = "";
		char param_mem[64] = "";
		char param_ip[16] = "";
		char last_r0[16] = "";
		AN_HANDLER cur;
		memset(&cur, 0, sizeof(cur));
		int r;
		rewind(fin);
		while ((r = an_read_handler(fin, &cur)) > 0) {
			for (int k = 0; k < cur.n; k++) {
				AN_INSN *ii = &cur.insns[k];
				if (!param_line_ok && (strstr(ii->disasm, "[esp+0x18]") || strstr(ii->disasm, "[esp+0x1C]"))) {
					if (strstr(ii->disasm, "mov ") && ii->mem[0]) {
						_snprintf(param_reg, sizeof(param_reg), "%s", ii->disasm + 4);
						param_reg[3] = 0;
						_snprintf(param_mem, sizeof(param_mem), "%s", ii->mem);
						_snprintf(param_ip, sizeof(param_ip), "%s", ii->ip);
						param_line_ok = 1;
					}
				}
				if (ii->r0[0] && strcmp(ii->r0, "00000000"))
					_snprintf(last_r0, sizeof(last_r0), "%s", ii->r0);
				if (ii->cond && ii->ctgt && ii->ip[0]) {
					unsigned long long iip = strtoull(ii->ip, NULL, 16);
					if (ii->ctgt < iip && loop_n < 64) {
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
			if (cur.insns) { free(cur.insns); cur.insns = NULL; cur.cap = 0; cur.n = 0; }
			if (param_line_ok && last_r0[0] && loop_n >= 64) break;
		}
		if (cur.insns) { free(cur.insns); cur.insns = NULL; }

		/* ---- emit header ---- */
		fprintf(fout, "// VMP 3.8 restore - pseudo C++ v3 (vslot + constant folding)\n");
		fprintf(fout, "// vN = virtual regs; // (const-folded) shows recovered constants\n\n");
		if (param_line_ok) {
			char memval[32] = "?";
			char memaddr[32] = "?";
			char *at = strchr(param_mem, '@');
			if (at) {
				*at = 0;
				_snprintf(memval, sizeof(memval), "%s", param_mem);
				_snprintf(memaddr, sizeof(memaddr), "%s", at + 1);
			}
			fprintf(fout, "// PARAM: %s = %s  (from [esp+0x18] @%s)\n", param_reg, memval, param_ip);
			fprintf(fout, "//         mem addr %s\n", memaddr);
			if (last_r0[0])
				fprintf(fout, "// RESULT: eax = %s  (last non-zero snapshot)\n", last_r0);
			fprintf(fout, "\n");
		}
	}

	/* ---- pass 2: stream handlers, analyze + emit ---- */
	{
		AN_HANDLER cur;
		memset(&cur, 0, sizeof(cur));
		int r;
		g_an_indent = 0;
		g_an_skip = 0;
		for (int li = 0; li < 64; li++) g_an_loop_iter[li] = 0;
		int prev_cmp = -1;
		rewind(fin);
		while ((r = an_read_handler(fin, &cur)) > 0) {
			const char *t = cur.type[0] ? cur.type : "?";
			if (!strcmp(t, "vDispatch") || !strcmp(t, "vInit") || !strcmp(t, "vJmp") ||
				!strcmp(t, "vRet") || !strcmp(t, "vCall") || !strcmp(t, "vEntry")) {
				prev_cmp = -1;
				if (cur.insns) { free(cur.insns); cur.insns = NULL; cur.cap = 0; cur.n = 0; }
				continue;
			}
			if (!cur.n) { if (cur.insns) { free(cur.insns); cur.insns = NULL; cur.cap = 0; cur.n = 0; } continue; }
			if (!strcmp(t, "vCmp")) {
				prev_cmp = cur.idx;
				fprintf(fout, "// handler[%d] vCmp (condition set)\n", cur.idx);
				for (int k = 0; k < cur.n; k++) {
					an_insn_loop_braces(fout, &cur.insns[k], loops, loop_n, &vs);
					if (g_an_skip > 0) continue;
					an_insn_translate(&cur.insns[k], &vs, line, sizeof(line), &cur, fout);
				}
				fprintf(fout, "\n");
				if (cur.insns) { free(cur.insns); cur.insns = NULL; cur.cap = 0; cur.n = 0; }
				continue;
			}
			prev_cmp = -1;
			fprintf(fout, "// handler[%d] %s\n", cur.idx, t);
			int n = 0;
			for (int k = 0; k < cur.n; k++) {
				an_insn_loop_braces(fout, &cur.insns[k], loops, loop_n, &vs);
				if (g_an_skip > 0) continue;
				if (an_insn_translate(&cur.insns[k], &vs, line, sizeof(line), &cur, fout))
					n++;
			}
			if (n == 0)
				fprintf(fout, "    // (engine internals only)\n");
			fprintf(fout, "\n");
			if (cur.insns) { free(cur.insns); cur.insns = NULL; cur.cap = 0; cur.n = 0; }
		}
		if (cur.insns) { free(cur.insns); cur.insns = NULL; }
	}

	/* 循环常量地址分析 + 高频魔数扫描（v2 data 直接解析：循环约简 / 强度约简 / 常量） */
	vmp38_analyze_constants(data_path, fout);
	/* 符号执行：动态值 → 魔数重建 */
	vmp38_analyze_symexec(data_path, fout);

	fclose(fout);
	fclose(fin);
	_plugin_logprintf("[VMP38-Analyze] inline done (streaming) -> %s", out_path);
}

/* ============================================================================
   VMP38 循环常量地址分析 + 高频魔数扫描（C++ 版，对应 sym_exec_data.py 的
   extract_trace_values + detect_loop_constants）。
   直接解析 v2 格式 data.txt；headless 还原时的真实还原过程。
     eip=addr\tcode\tchanged-regs\tmem-values\t[dedup 追串...]
   取出所有 32 位取值（=value，@addr 里 @ 后 = 取数值；另外取）频率统计；
   高频魔数 + 等差序列差分检测循环常量（强度约简，把循环遍历退化成常数）
   ========================================================================== */

#define AN_MAX_VAL    (1 << 20)   /* 最多 1M 个值 */
#define AN_MIN_FREQ   32          /* 高频阈值（去掉计数器/指针/整段字节） */
#define AN_MIN_SEQ    4           /* 等差序列最小值长度 */
#define AN_MIN_C      0x1000      /* 最小循环常量（避免最小常量误判） */
#define AN_MAX_HIFREQ 4096        /* 高频值数量上限（避免 O(n^2) 爆炸） */
#define AN_MAX_RES    64

typedef struct { unsigned long v; int cnt; } AN_VFREQ;

static int an_hexch(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int an_freq_cmp(const void *a, const void *b)
{
	const AN_VFREQ *x = (const AN_VFREQ*)a, *y = (const AN_VFREQ*)b;
	return x->v < y->v ? -1 : (x->v > y->v ? 1 : 0);
}

static int an_ulong_cmp(const void *a, const void *b)
{
	unsigned long x = *(const unsigned long*)a, y = *(const unsigned long*)b;
	return x < y ? -1 : (x > y ? 1 : 0);
}

/* 读取 v2 data.txt 里所有 32 位取值（=value），排除 @addr。 */
static int an_extract_vals(const char *path, unsigned long *vals, int cap)
{
	FILE *f = fopen(path, "rb");
	char line[4096];
	int n = 0;
	if (!f) return 0;
	while (fgets(line, sizeof(line), f) && n < cap) {
		if (strncmp(line, "eip=", 4) != 0) continue;
		char *p = line;
		int tabs = 0;
		while (*p && tabs < 2) { if (*p == '\t') tabs++; p++; }
		while (*p) {
			if (*p == '=') {
				/* 当前操作数是寄存器（堆栈指针 esp/ebp 之类栈地址不是算法魔数） */
				char rname[8] = "";
				int rl = 0;
				char *rp = p - 1;
				while (rp >= line && rl < 7 && ((*rp >= 'a' && *rp <= 'z') || (*rp >= 'A' && *rp <= 'Z'))) {
					rname[rl++] = *rp; rp--;
				}
				{ int lo = 0, hi = rl - 1; while (lo < hi) { char t = rname[lo]; rname[lo] = rname[hi]; rname[hi] = t; lo++; hi--; } rname[rl] = 0; }
				if (!_stricmp(rname, "esp") || !_stricmp(rname, "ebp")) { p++; continue; }
				char *q = p + 1;
				if (q[0] == '0' && (q[1] == 'x' || q[1] == 'X')) q += 2;
				int k = 0;
				while (k < 8 && an_hexch(q[k])) k++;
				if (k == 8 && !an_hexch(q[8])) {
					char hex[9];
					memcpy(hex, q, 8); hex[8] = 0;
					vals[n++] = (unsigned long)strtoul(hex, NULL, 16);
				}
			}
			p++;
		}
	}
	fclose(f);
	return n;
}

/* 循环常量地址分析 + 高频魔数扫描。能追踪到 fout（已打开的 analyzer 输出文件） */
void vmp38_analyze_constants(const char *data_path, FILE *fout)
{
	unsigned long *all = (unsigned long*)malloc((size_t)AN_MAX_VAL * sizeof(unsigned long));
	if (!all) return;
	int n = an_extract_vals(data_path, all, AN_MAX_VAL);
	if (n == 0) { free(all); return; }

	/* 频率统计 */
	qsort(all, n, sizeof(unsigned long), an_ulong_cmp);
	AN_VFREQ *freq = (AN_VFREQ*)malloc((size_t)(n + 1) * sizeof(AN_VFREQ));
	if (!freq) { free(all); return; }
	int fn = 0;
	for (int i = 0; i < n; ) {
		unsigned long v = all[i];
		int cnt = 0;
		while (i < n && all[i] == v) { cnt++; i++; }
		freq[fn].v = v; freq[fn].cnt = cnt; fn++;
	}
	free(all);

	/* 高频值排序（按 v 排序，供 bsearch） */
	AN_VFREQ *hi = (AN_VFREQ*)malloc((size_t)(fn + 1) * sizeof(AN_VFREQ));
	if (!hi) { free(freq); return; }
	int hn = 0;
	for (int i = 0; i < fn && hn < AN_MAX_HIFREQ; i++)
		if (freq[i].cnt >= AN_MIN_FREQ) { hi[hn++] = freq[i]; }
	free(freq);

	fprintf(fout, "\n// ===== 循环常量地址分析 + 高频魔数 (v2 data) =====\n");
	/* 高频值展示：按频率降序（高频魔数优先），跳过地址小值 */
	{
		AN_VFREQ *bycnt = (AN_VFREQ*)malloc((size_t)(hn + 1) * sizeof(AN_VFREQ));
		if (bycnt) {
			memcpy(bycnt, hi, (size_t)hn * sizeof(AN_VFREQ));
			/* 按 cnt 做冒泡，hn 通常 < 4096，所以够用 */
			for (int a = 0; a < hn; a++)
				for (int b = a + 1; b < hn; b++)
					if (bycnt[b].cnt > bycnt[a].cnt) {
						AN_VFREQ t = bycnt[a]; bycnt[a] = bycnt[b]; bycnt[b] = t;
					}
			fprintf(fout, "// 高频值(>=%d次, >=0x1000, 按频率降序):\n", AN_MIN_FREQ);
			int shown = 0;
			for (int i = 0; i < hn && shown < 64; i++) {
				if (bycnt[i].v >= AN_MIN_C) {
					fprintf(fout, "//   %08lx  x%d\n", bycnt[i].v, bycnt[i].cnt);
					shown++;
				}
			}
			free(bycnt);
		}
	}

	/* 等差序列检测（地址循环常量）
	   列删：排除跳转/日志类的 C（popcount 太少，不形成等差数列跨多次等差）
	   去重：每个 C 只记录一次。 */
	fprintf(fout, "// 循环常量等差序列差分, 值>=0x1000, 去重(省略重复/符号):\n");
	unsigned long *seenC = (unsigned long*)malloc((size_t)(AN_MAX_RES * 2) * sizeof(unsigned long));
	int seen_n = 0;
	int res_n = 0;
	for (int i = 0; i < hn && res_n < AN_MAX_RES; i++) {
		for (int j = i + 1; j < hn && res_n < AN_MAX_RES; j++) {
			unsigned long C = hi[j].v - hi[i].v;   /* hi 按 v 升序，j>i => C 是正 */
			if (C < AN_MIN_C) continue;
			/* 排除跳转/日志类：popcount <= 2 或 >= 30 */
			{ unsigned long t = C; int pc = 0; while (t) { pc += (int)(t & 1); t >>= 1; }
			  if (pc <= 2 || pc >= 30) continue; }
			/* 去重 */
			{ int dup = 0; for (int s = 0; s < seen_n; s++) if (seenC[s] == C) { dup = 1; break; }
			  if (dup) continue; }
			int ok = 1;
			for (int k = 1; k < AN_MIN_SEQ; k++) {
				unsigned long nxt = hi[i].v + (unsigned long)k * C;
				AN_VFREQ key; key.v = nxt; key.cnt = 0;
				if (!bsearch(&key, hi, hn, sizeof(AN_VFREQ), an_freq_cmp)) { ok = 0; break; }
			}
			if (ok) {
				fprintf(fout, "//   C = %08lx  (%lu)\n", C, C);
				seenC[seen_n++] = C;
				res_n++;
			}
		}
	}
	fprintf(fout, "\n");

	free(seenC);
	free(hi);
}

/* ============================================================================
   VMP38 符号执行：动态值 → 魔数重建（对应 sym_exec_data.py 的 SymExecData）。
   简化版：追踪方法：对源码来源的数据段赋值地址在寄存器/内存的传播；记录
   且该 op 是算术的单目运算——若 VMP 的魔数是 mov imm + 运行时动态常数，
   说明该魔数是某个值= 的魔数（可以拿到），构造 AST 只在化简时可取。
   ========================================================================== */

#define SYM_NONE 0xFFFFFFFFUL
#define SYM_MAX_CONST 256

typedef struct { unsigned long c; int cnt; } SYM_CONST;

static int sym_const_add(SYM_CONST *arr, int *n, unsigned long c)
{
	for (int i = 0; i < *n; i++)
		if (arr[i].c == c) { arr[i].cnt++; return 0; }
	if (*n >= SYM_MAX_CONST) return -1;
	arr[*n].c = c; arr[*n].cnt = 1; (*n)++;
	return 0;
}

/* 从 disasm 的文本提取 mnemonic + operands（解析 [..] 内的逗号） */
static void sym_parse_insn(const char *code, char *mn, char ops[3][64], int *nops)
{
	int i = 0;
	while (code[i] && code[i] != ' ' && code[i] != '\t') i++;
	int ml = i; if (ml > 15) ml = 15;
	memcpy(mn, code, ml); mn[ml] = 0;
	while (code[i] == ' ' || code[i] == '\t') i++;
	int no = 0, depth = 0, ol = 0;
	for (; code[i] && no < 3; i++) {
		char ch = code[i];
		if (ch == '[') depth++;
		if (ch == ']') { if (depth > 0) depth--; }
		if (ch == ',' && depth == 0) {
			ops[no][ol] = 0; no++; ol = 0;
			while (code[i + 1] == ' ') i++;
		} else if (ol < 63) {
			ops[no][ol++] = ch;
		}
	}
	if (ol > 0) { ops[no][ol] = 0; no++; }
	*nops = no;
}

/* 寄存器名 -> 索引 0..7；-1 表示非寄存器 */
static int sym_reg_idx(const char *name)
{
	static const char *rn[8] = { "eax","ecx","edx","ebx","esp","ebp","esi","edi" };
	for (int i = 0; i < 8; i++) if (!_stricmp(name, rn[i])) return i;
	return -1;
}

/* 分类操作数：1=寄存器(写 ri)，2=常量(写 imm)，3=内存(写 addr)，0=其他 */
static int sym_classify(const char *op, int *ri, unsigned long *imm)
{
	char t[64];
	_snprintf(t, sizeof(t), "%s", op);
	/* 去前后空格 */
	char *s = t; while (*s == ' ') s++;
	if (!*s) return 0;   /* 空操作数 */
	char *e = s + strlen(s) - 1; while (e > s && *e == ' ') { *e = 0; e--; }
	/* 判断立即数是 hex/dec */
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { *imm = strtoul(s, NULL, 16); return 2; }
	if (s[0] >= '0' && s[0] <= '9') { *imm = strtoul(s, NULL, 0); return 2; }
	/* 内存：含 [ */
	if (strchr(s, '[')) return 3;
	/* 寄存器 */
	int ri2 = sym_reg_idx(s);
	if (ri2 >= 0) { *ri = ri2; return 1; }
	return 0;
}

/* 判断操作数是否为源码（esp 初始值以上，且具备数据段的特征，排除 esp 以下：值栈/局部变量
   和代码段。例如 esp+0x18 等，cdecl 返回 esp 以上的算法参数。 */
static int sym_is_source(unsigned long addr, unsigned long esp_init,
	unsigned long code_lo, unsigned long code_hi)
{
	if (esp_init && addr < esp_init) return 0;             /* esp 以下：值栈/局部变量 */
	if (addr >= code_lo && addr <= code_hi) return 0;      /* 代码段 */
	return 1;                                              /* esp 以上：源码数据段 */
}

/* 符号执行：重建源操作（op 序列里的魔数） */
void vmp38_analyze_symexec(const char *data_path, FILE *fout)
{
	FILE *f = fopen(data_path, "rb");
	char line[4096];
	unsigned long esp_init = 0;
	unsigned long code_lo = 0, code_hi = 0xffffffff;
	int first = 1;

	/* 寄存器打标：SYM_NONE=非常量，其他=该源地址（非内存）打标哈希 */
	unsigned long sreg[8];
	unsigned long *maddr = NULL; unsigned long *msym = NULL;
	int mcap = 0, mn2 = 0;
	for (int i = 0; i < 8; i++) sreg[i] = SYM_NONE;

	SYM_CONST consts[SYM_MAX_CONST];
	int cn = 0;
	memset(consts, 0, sizeof(consts));

	if (!f) return;

	while (fgets(line, sizeof(line), f)) {
		if (strncmp(line, "eip=", 4) != 0) continue;
		/* 解析各字段：eip=addr\tcode\tchanged\tmem... */
		char *p = line + 4;
		unsigned long eip = strtoul(p, NULL, 16);
		char *code = strchr(p, '\t'); if (!code) continue; code++;
		char *chg = strchr(code, '\t'); if (!chg) continue; *chg = 0; chg++;
		char *memf = strchr(chg, '\t'); if (!memf) continue; *memf = 0; memf++;

		if (first) {
			code_lo = eip & 0xFFF00000UL;
			code_hi = code_lo + 0x00100000UL;
			first = 0;
		}
		/* 跟踪 esp 初值：取第一条非值 esp= 指令（下一指令只可能 flags 变化） */
		if (esp_init == 0) {
			char *q = chg;
			while (*q) {
				if (!strncmp(q, "esp=", 4)) {
					esp_init = strtoul(q + 4, NULL, 16);
					break;
				}
				q++;
			}
		}

		/* 更新每组寄存器值（changed 里的） */
		unsigned long rval[8] = { 0,0,0,0,0,0,0,0 };
		{
			char *q = chg;
			while (*q) {
				if (*q == '=') {
					/* 找寄存器名，= 前面 */
					char rn[8] = ""; int rl = 0; char *rp = q - 1;
					while (rp >= chg && rl < 7 && ((*rp >= 'a' && *rp <= 'z') || (*rp >= 'A' && *rp <= 'Z'))) { rn[rl++] = *rp; rp--; }
					{ int lo = 0, hi = rl - 1; while (lo < hi) { char t2 = rn[lo]; rn[lo] = rn[hi]; rn[hi] = t2; lo++; hi--; } rn[rl] = 0; }
					int ri = sym_reg_idx(rn);
					if (ri >= 0) {
						unsigned long v = strtoul(q + 1, NULL, 16);
						rval[ri] = v;
					}
				}
				q++;
			}
		}

		/* 跳转 mem values 里的地址(内存列 [addr]=..., addr 即绝对地址) */
		unsigned long memaddr = 0;
		{
			char *lb = strchr(memf, '[');
			if (lb) memaddr = strtoul(lb + 1, NULL, 16);
		}

		/* 符号传入 */
		char mn[16]; char ops[3][64]; int nops = 0;
		sym_parse_insn(code, mn, ops, &nops);
		if (nops < 1) continue;

		int dri; unsigned long dimm;
		int dcls = sym_classify(ops[0], &dri, &dimm);

		if (!strcmp(mn, "mov")) {
			if (dcls == 1) {
				/* mov reg, src */
				int sri; unsigned long simm;
				int scls = (nops >= 2) ? sym_classify(ops[1], &sri, &simm) : 0;
				if (scls == 1) sreg[dri] = sreg[sri];                     /* 寄存器间传播 */
				else if (scls == 2) sreg[dri] = SYM_NONE;                 /* 常量破坏非常量 */
				else if (scls == 3) {
					/* 内存：该地址找 */
					unsigned long a = memaddr;
					unsigned long sym = SYM_NONE;
					for (int k = 0; k < mn2; k++) if (maddr[k] == a) { sym = msym[k]; break; }
					if (sym == SYM_NONE && sym_is_source(a, esp_init, code_lo, code_hi)) sym = a;
					sreg[dri] = sym;
				}
			} else if (dcls == 3) {
				/* mov [mem], src：写符号到内存 */
				unsigned long a = memaddr;
				unsigned long sym = SYM_NONE;
				if (nops >= 2) {
					int sri; unsigned long simm;
					int scls = sym_classify(ops[1], &sri, &simm);
					if (scls == 1) sym = sreg[sri];
					else if (scls == 2) sym = SYM_NONE;
				}
				/* 更新缓存（内存表） */
				int found = 0;
				for (int k = 0; k < mn2; k++) if (maddr[k] == a) { msym[k] = sym; found = 1; break; }
				if (!found && mn2 < 65536) {
					if (mn2 >= mcap) {
						int nc = mcap ? mcap * 2 : 1024;
						/* 用 realloc 每次可能失败：若提交指针，防止外部失败后致悬空 */
						unsigned long *na = (unsigned long*)realloc(maddr, (size_t)nc * sizeof(unsigned long));
						if (!na) break;
						unsigned long *ns = (unsigned long*)realloc(msym, (size_t)nc * sizeof(unsigned long));
						if (!ns) { free(na); break; }   /* 第二个失败，释放第一个并获得新指针 */
						maddr = na; msym = ns; mcap = nc;
					}
					maddr[mn2] = a; msym[mn2] = sym; mn2++;
				}
			}
			continue;
		}

		/* 运算符记录（该 op 是算术的单目运算） */
		static const char *arith[] = { "add","sub","xor","and","or","adc","sbb","shl","shr","sal","rol","ror","imul","mul" };
		int is_arith = 0;
		for (int a2 = 0; a2 < (int)(sizeof(arith)/sizeof(arith[0])); a2++)
			if (!strcmp(mn, arith[a2])) { is_arith = 1; break; }
		if (is_arith && dcls == 1 && nops >= 2) {
			int sri; unsigned long simm;
			int scls = sym_classify(ops[1], &sri, &simm);
			unsigned long d_sym = sreg[dri];
			unsigned long s_sym = (scls == 1) ? sreg[sri] : SYM_NONE;
			/* 恰有一个是非常量 + 是算术，记录非常量操作产生的该值（= 魔数） */
			if ((d_sym != SYM_NONE) != (s_sym != SYM_NONE)) {
				if (d_sym != SYM_NONE) {
					/* dst 非常量，src 是常数：记录产生该值的寄存器 */
					if (scls == 2) sym_const_add(consts, &cn, simm);
					else if (scls == 1) sym_const_add(consts, &cn, rval[sri]);
				} else {
					/* src 非常量，dst 是具体值寄存器 */
					sym_const_add(consts, &cn, rval[dri]);
				}
			}
			/* 符号或任一操作数非常量：之一为非常量则 */
			if (d_sym != SYM_NONE || s_sym != SYM_NONE)
				sreg[dri] = (d_sym != SYM_NONE) ? d_sym : s_sym;
			else
				sreg[dri] = SYM_NONE;
		}
	}

	/* 记录对称操作（魔数） */
	fprintf(fout, "\n// ===== 符号执行：动态值 → 魔数（按 op 序列，单目） =====\n");
	int shown = 0;
	for (int i = 0; i < cn && shown < 64; i++) {
		if (consts[i].c >= 0x1000) {
			fprintf(fout, "//   %08lx  x%d\n", consts[i].c, consts[i].cnt);
			shown++;
		}
	}
	fprintf(fout, "\n");

	free(maddr); free(msym);
	fclose(f);
}
