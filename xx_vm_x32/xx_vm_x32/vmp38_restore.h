/* vmp38_restore.h - VMP 3.8 VM restore (dynamic execution + vsp tracking) */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* full context struct must match XX_CONTEXT (x32) / XX_CONTEXT64 (x64)
   IMPORTANT: r[]/eflag/ip are ALWAYS 64-bit fields in BOTH builds.
   The 32-bit build fills them with 32-bit values zero-extended (see
   xx_vm.cpp MENU_VMP38_RESTORE). Do NOT use unsigned long (4-byte)
   fields here - the restore core and the caller both access these
   slots as unsigned long long (8 bytes); a 4-byte field misaligns
   every register (ebp showed ip<<8 junk, bc_base showed bc_len<<32). */
typedef struct {
	unsigned long long r[16];
	unsigned long long eflag;
	unsigned long long ip;
} VMP38_CTX;
#define VMP_REG_NUM 16

void vmp38_set_module(const char *path, unsigned long long imgbase);

/* auto-locate the VMP 3.8 VM entry: scan .text for call/jmp into the VM section.
   returns the VM entry address, or 0 if not found. */
unsigned long long vmp38_auto_locate();
void vmp38_get_vm_bounds(unsigned long long *start, unsigned long long *end);
int vmp38_scan_all_entries(void);
/* Full-trace: Unicorn runs the VM code like a real CPU (continuous),
   code hook records eip+registers, stops on module-exit. Returns the
   number of instructions executed. */
int vmp38_unicorn_full_trace(unsigned long long entry_ip, unsigned long long entry_esp, unsigned long long *out_last_eip);

/* REVM: execute the RESTORED code (not the raw VM bytes) in a fresh
   Unicorn, single-stepping and logging regs each instruction, then
   reconcile the final return value / out[] against the golden chain.
   Returns 1 on success. */
int vmp38_revm_exec(void);

/* Run to the VM entry and capture the real register state there.
   Sets a temp breakpoint at vm_entry, runs the debuggee, waits for the
   breakpoint, then fills *ctx with the register dump. Returns 1 on success.
   Must be called from a plugin menu callback (GUI or headless). */
int vmp38_capture_vm_regs(unsigned long long vm_entry, VMP38_CTX *ctx);

/* ============ ValueCommand::Calc decrypt chain (VMP 3.5.1 source) ============
   VMP encrypts constants with a command chain:
     Encrypt(v) = cmd[0](v) -> cmd[1](v) -> ... -> cmd[n-1](v)
     Decrypt(v) = cmd[n-1](v) -> ... -> cmd[1](v) -> cmd[0](v)
   where each cmd is one of: add/sub/inc/dec/xor/not/neg/bswap/rol/ror,
   and add<->sub, inc<->dec, rol<->ror swap when decrypting.
   This lets us algebraically recover the real constant from an encrypted
   immediate without depending on execution-path tracking.
*/
#define VMP_CC_ADD  0
#define VMP_CC_SUB  1
#define VMP_CC_INC  2
#define VMP_CC_DEC  3
#define VMP_CC_XOR  4
#define VMP_CC_NOT  5
#define VMP_CC_NEG  6
#define VMP_CC_BSWAP 7
#define VMP_CC_ROL  8
#define VMP_CC_ROR  9
#define VMP_CC_SHL  10
#define VMP_CC_SHR  11

typedef struct {
	int type;              /* VMP_CC_* */
	unsigned long long val; /* operand (value_) */
	int size_bits;         /* 8/16/32/64 */
} VMP_CMD;

/* decrypt a value through a chain: applies cmds in reverse order with
   add<->sub / inc<->dec / rol<->ror swapped. n = number of cmds. */
unsigned long long vmp38_value_decrypt(unsigned long long value,
	const VMP_CMD *cmds, int n);

#ifdef __cplusplus
}
#endif

void vmp38_analyze_inline(const char *data_path, const char *out_path);

int vmp38_dynamic_trace(unsigned long long start_ip);

unsigned long long vmp38_get_imgbase(void);

/* live progress on the plugin menu entry while vmp38_dynamic_trace runs */
void vmp38_dyn_menu(int step, int handlers);

/* set to 1 by the VMP38-Stop menu to abort an unbounded background trace.
   Written by the GUI/menu thread, read by the background trace thread ->
   volatile so the reader never sees a stale cached value. */
extern volatile int g_dyn_stop;

/* 1 = append repeat-execution deltas (same eip again), 0 = first-execution only.
   Toggled by the VMP38-Appends menu; XXVM_APPENDS=0 env sets the default. */
extern int g_agg_appends;
extern int g_vm_continue;   /* auto-continue into the next VM function */

/* per-eip repeat-appends cap (default 256). XXVM_APPENDS_MAX env overrides;
   0 = unbounded. */
extern int g_row_max_appends;

/* stream mode: unbounded trace, vm-exit stops (no g_ft buffer) */
