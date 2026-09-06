#include "stdio.h"
#include <windows.h>
#include <shlwapi.h>
//#include "Plugin.h"
#include "xx_vm.h"
#include "xx_comm64.h"
#include "xxdisasm64.h"
#include "xx_opdef2.h"
//#include "xx_file.h"
#include "xx_link_list.h"


extern int xx_read_fmem(unsigned long long addr, char* pdata, unsigned int datasize);
extern int xx_write_fmem(unsigned long long addr, char* pdata, unsigned int datasize);
extern int calc_item(struct XX_INST *inst, int nitem, struct ITEM_VAR64 *out_var, struct XX_CONTEXT64 *out_context, unsigned long long result, int);
extern void item_sign_ext(unsigned long long size1, unsigned long long size2, unsigned char *pvar2);


extern char debug_buf[500];
extern int debug_log(char *tmp);
///////////////////////////////////////////////////////////////////////////

int func_op_and(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_xor(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_or(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_test(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_not(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_neg(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_bsr(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_bsf(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern int asm_xor(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_and(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_or(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_test(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_not(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_neg(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_cmp(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_bsr(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_bsf(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int func_op_xor(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_xor(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	iret = calc_item(xx_inst, 0, var, out_context, var1, 2);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_and(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	item_sign_ext(var[0].size, var[1].size, (char*)&var[1].var);

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_and(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	iret = calc_item(xx_inst, 0, var, out_context, var1, 2);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_or(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	item_sign_ext(var[0].size, var[1].size, (char*)&var[1].var);

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_or(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	iret = calc_item(xx_inst, 0, var, out_context, var1, 2);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_test(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_test(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_not(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_not(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	iret = calc_item(xx_inst, 0, var, out_context, var1, 2);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_neg(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_neg(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	iret = calc_item(xx_inst, 0, var, out_context, var1, 2);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_cmp(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_cmp(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_bsr(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var0 = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	ulong64 size = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	size = var[1].size;
	if (size == INST_SYSTEM_16)
	{
		var0 = var[1].var & 0x000000000000ffff;
	}
	else if (size == INST_SYSTEM_32)
	{
		var0 = var[1].var & 0x00000000ffffffff;
	}
	else if (size == INST_SYSTEM_64)
	{
		var0 = var[1].var ;
	}
	else
	{
		return 0;
	}
	
	memset(&out_var, 0, sizeof(out_var));
	iret = asm_bsr(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	if (var0 != 0)
	{
		iret = calc_item(xx_inst, 0, var, out_context, var1, 2);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_bsf(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var0 = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	ulong64 size = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	size = var[1].size;
	if (size == INST_SYSTEM_16)
	{
		var0 = var[1].var & 0x000000000000ffff;
	}
	else if (size == INST_SYSTEM_32)
	{
		var0 = var[1].var & 0x00000000ffffffff;
	}
	else if (size == INST_SYSTEM_64)
	{
		var0 = var[1].var;
	}
	else
	{
		return 0;
	}

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_bsf(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	if (var0 != 0)
	{
		iret = calc_item(xx_inst, 0, var, out_context, var1, 2);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}
