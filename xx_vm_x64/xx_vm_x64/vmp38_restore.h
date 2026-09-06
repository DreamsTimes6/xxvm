/* vmp38_restore.h - VMP 3.8 VM restore (dynamic execution + vsp tracking) */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* full context struct must match XX_CONTEXT64 in xx_vm.h */
typedef struct {
	unsigned long long r[16];
	unsigned long long eflag;
	unsigned long long ip;
} VMP38_CTX;

void vmp38_restore_from_x64dbg(VMP38_CTX *ctx, unsigned long long bc_base, int bc_len);
void vmp38_set_module(const char *path, unsigned long long imgbase);

/* auto-locate the VMP 3.8 VM entry: scan .text for call/jmp into the VM section.
   returns the VM entry address, or 0 if not found. */
unsigned long long vmp38_auto_locate();

/* Run to the VM entry and capture the real register state there.
   Sets a temp breakpoint at vm_entry, runs the debuggee, waits for the
   breakpoint, then fills *ctx with the register dump. Returns 1 on success.
   Must be called from a plugin menu callback (GUI or headless). */
int vmp38_capture_vm_regs(unsigned long long vm_entry, VMP38_CTX *ctx);

/* Dynamic single-step trace from start_ip (uses DbgCmdExecDirect for sync
   stepping; works under headless -rpc too). Returns 1 on completion. */
int vmp38_dynamic_trace(unsigned long long start_ip);

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

typedef struct {
	int type;              /* VMP_CC_* */
	unsigned long long val; /* operand (value_) */
	int size_bits;         /* 8/16/32/64 */
} VMP_CMD;

/* decrypt a value through a chain: applies cmds in reverse order with
   add<->sub / inc<->dec / rol<->ror swapped. n = number of cmds. */
unsigned long long vmp38_value_decrypt(unsigned long long value,
	const VMP_CMD *cmds, int n);

/* encrypt a value through a chain (forward order, as VMP does) */
unsigned long long vmp38_value_encrypt(unsigned long long value,
	const VMP_CMD *cmds, int n);

#ifdef __cplusplus
}
#endif
void vmp38_analyze_inline(const char *data_path, const char *out_path);
int vmp38_scan_all_entries(void);
int vmp38_unicorn_full_trace64(unsigned long long entry_ip, unsigned long long entry_rsp, unsigned long long *out_last_eip);
void vmp38_restore_from_trace64(int n);
extern int g_dyn_running;
extern int g_dyn_stop;
extern unsigned long long g_dyn_start;
extern DWORD WINAPI vmp38_dyn_thread(LPVOID param);
