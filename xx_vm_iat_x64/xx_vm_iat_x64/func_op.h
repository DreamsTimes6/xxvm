#include "stdio.h"
#include <windows.h>
#include "xx_iat64.h"

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////  1   //////////////////////////////////////////
int func_op_push(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_pop(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_call(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_retn(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_xchg(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_movsx(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_movzx(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_bswap(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cdq(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cwde(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_pushfd(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_popfd(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_mov(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_inc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_dec(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_sbb(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_lea(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_add(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_sub(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
/////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////    2     ///////////////////////////////
int func_op_sal(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_sar(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_shl(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_shr(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_rcl(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_rcr(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_rol(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_ror(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_shld(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_shrd(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
////////////////////////////////////////////////////////////////////////


//////////////////////////////   3    ////////////////////////////////////
int func_op_and(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_xor(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_or(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_test(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_not(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_neg(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_bsr(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_bsf(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
////////////////////////////////////////////////////////////////////////

/////////////////////////////  4   /////////////////////////////////////////
int func_op_bt(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_btc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_btr(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_bts(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);

int func_op_clac(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_clc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cld(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cli(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);

int func_op_stc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_std(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_sti(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
////////////////////////////////////////////////////////////////////////



////////////////////////////   5    ///////////////////////////////////////
int func_op_jmp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_je(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jne(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_ja(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jae(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jb(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jbe(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jg(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jge(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jl(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jle(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jnc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jo(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jno(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_js(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jns(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jnp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jecxz(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
////////////////////////////////////////////////////////////////////////



////////////////////////  6   ////////////////////////////////////////////
int func_op_xadd(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_lahf(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_sahf(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_adc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_nop(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_movsb(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_movsd(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_rdtsc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cpuid(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_mul(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_imul(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_div(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_idiv(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////   7  ////////////////////////////////////////
int func_op_sete(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setne(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_seta(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setae(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setb(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setbe(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setg(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setge(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setl(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setle(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_sets(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setns(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_seto(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setno(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_setnp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
////////////////////////////////////////////////////////////////////////////

/////////////////////////////   8   ///////////////////////////////////////
int func_op_cmove(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovne(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmova(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovae(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovb(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovbe(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovg(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovge(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovl(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovle(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovs(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovns(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovo(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovno(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cmovnp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

