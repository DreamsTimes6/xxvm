#include "stdio.h"
#include <windows.h>
#include <shlwapi.h>
//#include "Plugin.h"
#include "xx_vm.h"
#include "xx_comm32.h"
#include "xxdisasm32.h"
#include "xx_opdef2.h"



extern int xx_read_fmem(unsigned long addr, char* pdata, unsigned int datasize);
extern int xx_write_fmem(unsigned long addr, char* pdata, unsigned int datasize);
extern int calc_item(struct XX_INST *inst, int nitem, struct ITEM_VAR *out_var, struct XX_CONTEXT *out_context, unsigned  long result, int sign_flag);
extern void item_sign_ext(unsigned long size1, unsigned long size2, unsigned char *pvar2);
extern int func_inst_prefix(struct XX_INST *pinst, unsigned char pc);

extern char debug_buf[500];
extern int debug_log(char *tmp);

extern void *pfunc_v;
///////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////

__declspec(noinline) int func_op_aaa(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	eflag = in_context->eflag;
	var1 = in_context->r[REG_EAX];

	__asm
	{
		pushad
		mov eax, var1
		push eflag
		popfd
		aaa
		pushfd
		pop eflag
		mov var1, eax
		popad
	};

	out_context->r[REG_EAX] = var1;
	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_aas(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	eflag = in_context->eflag;
	var1 = in_context->r[REG_EAX];

	__asm
	{
		pushad
		mov eax, var1
		push eflag
		popfd
		aas
		pushfd
		pop eflag
		mov var1, eax
		popad
	};

	out_context->r[REG_EAX] = var1;
	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_aam(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;
	unsigned char c_ret[] = {0xc3};

	if (pfunc_v == 0)
	{
		return 0;
	}

	memcpy(pfunc_v, xx_inst->xx_inst_code.disasm, xx_inst->xx_inst_code.disasm_length);
	memcpy((unsigned char*)pfunc_v + xx_inst->xx_inst_code.disasm_length, c_ret, sizeof(c_ret));

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	eflag = in_context->eflag;
	var1 = in_context->r[REG_EAX];

	__asm
	{
		pushad
		mov eax, var1
		push eflag
		popfd
		mov esi, pfunc_v
		call esi
		pushfd
		pop eflag
		mov var1, eax
		popad
	};

	out_context->r[REG_EAX] = var1;
	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_aad(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;
	unsigned char c_ret[] = { 0xc3 };

	if (pfunc_v == 0)
	{
		return 0;
	}

	memcpy(pfunc_v, xx_inst->xx_inst_code.disasm, xx_inst->xx_inst_code.disasm_length);
	memcpy((unsigned char*)pfunc_v + xx_inst->xx_inst_code.disasm_length, c_ret, sizeof(c_ret));

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	eflag = in_context->eflag;
	var1 = in_context->r[REG_EAX];

	__asm
	{
		pushad
		mov eax, var1
		push eflag
		popfd
		mov esi, pfunc_v
		call esi
		pushfd
		pop eflag
		mov var1, eax
		popad
	};

	out_context->r[REG_EAX] = var1;
	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_daa(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	eflag = in_context->eflag;
	var1 = in_context->r[REG_EAX];

	__asm
	{
		pushad
		mov eax, var1
		push eflag
		popfd
		daa
		pushfd
		pop eflag
		mov var1, eax
		popad
	};

	out_context->r[REG_EAX] = var1;
	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_das(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;
	unsigned int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	eflag = in_context->eflag;
	var1 = in_context->r[REG_EAX];

	__asm
	{
		pushad
		mov eax, var1
		push eflag
		popfd
		das
		pushfd
		pop eflag
		mov var1, eax
		popad
	};

	out_context->r[REG_EAX] = var1;
	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}







__declspec(noinline) int func_op_leave(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));


	out_context->r[REG_ESP] = out_context->r[REG_EBP];

	/*取数据*/
	iret = xx_read_fmem(out_context->r[REG_ESP], (unsigned char*)&var1, INST_SYSTEM_CURT);
	if (iret == 0)
	{
		return 0;
	}

	/*sp+4*/
	out_context->r[REG_ESP] = out_context->r[REG_ESP] + INST_SYSTEM_CURT;

	out_context->r[REG_EBP] = var1;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_loop(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;

	/*api则退出*/

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	if (var[0].size != 4)
	{
		return 0;
	}

	out_context->r[REG_ECX] = out_context->r[REG_ECX] - 1;

	if (out_context->r[REG_ECX] != 0)
	{
		out_context->ip = var[0].var;
	}
	else
	{
		out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	}

	return 1;
}




__declspec(noinline) int func_op_loope(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;

	/*api则退出*/

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	if (var[0].size != 4)
	{
		return 0;
	}

	out_context->r[REG_ECX] = out_context->r[REG_ECX] - 1;

	eflag = in_context->eflag;
	//zf=1
	if ((eflag & 0x00000040) == 0x00000040 && out_context->r[REG_ECX] != 0)
	{
		out_context->ip = var[0].var;
	}
	else
	{
		out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	}

	return 1;
}


__declspec(noinline) int func_op_loopne(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;

	/*api则退出*/

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	if (var[0].size != 4)
	{
		return 0;
	}

	out_context->r[REG_ECX] = out_context->r[REG_ECX] - 1;

	eflag = in_context->eflag;
	//zf=0
	if ((eflag & 0x00000040) == 0 && out_context->r[REG_ECX] != 0)
	{
		out_context->ip = var[0].var;
	}
	else
	{
		out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	}

	return 1;
}


