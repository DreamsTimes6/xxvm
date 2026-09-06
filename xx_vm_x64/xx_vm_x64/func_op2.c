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
//////////////////////////////////////////////////////////////////////////////////////////////////////
extern int asm_sal(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_sar(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_shl(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_shr(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_rcl(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_rcr(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_rol(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_ror(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_shld(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_shrd(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
///////////////////////////////////////////////////////////////////////////////////////////////////////

int func_op_sal(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_sal(var, in_context, &out_var);
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



int func_op_sar(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_sar(var, in_context, &out_var);
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



int func_op_shl(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_shl(var, in_context, &out_var);
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



int func_op_shr(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_shr(var, in_context, &out_var);
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



int func_op_rcl(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_rcl(var, in_context, &out_var);
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


int func_op_rcr(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_rcr(var, in_context, &out_var);
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


int func_op_rol(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_rol(var, in_context, &out_var);
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


int func_op_ror(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_ror(var, in_context, &out_var);
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



int func_op_shld(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	ulong64 var3=0;
	int index=0;
	ulong64 eflag=0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_shld(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];
	var2 = out_var.o_var[1];

	

	iret=calc_item(xx_inst,0,var,out_context,var1, 2);
	if(iret==0)
	{
		return 0;
	}

	

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_shrd(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	ulong64 var3 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_shrd(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];
	var2 = out_var.o_var[1];




	iret = calc_item(xx_inst, 0, var, out_context, var1, 2);
	if (iret == 0)
	{
		return 0;
	}

	

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

