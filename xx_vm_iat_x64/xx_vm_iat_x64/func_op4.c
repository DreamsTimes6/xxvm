#include "stdio.h"
#include <windows.h>
#include <shlwapi.h>
//#include "Plugin.h"
#include "xx_iat64.h"
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern int asm_bt(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_btc(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_btr(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_bts(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_cli(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_cmc(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_sti(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int func_op_bt(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;
	ulong64 eflag=0;
	char bvar=0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_bt(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_btc(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_btc(var, in_context, &out_var);
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


int func_op_btr(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_btr(var, in_context, &out_var);
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


int func_op_bts(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_bts(var, in_context, &out_var);
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



int func_op_clac(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;
	char bvar=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	/*eflag*/

	eflag=in_context->eflag;

	eflag=eflag & 0xfffbffff;

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_clc(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;
	char bvar=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	/*eflag*/

	eflag=in_context->eflag;

	eflag=eflag & 0xfffffffe;

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

int func_op_cld(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;
	char bvar=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	/*eflag*/

	eflag=in_context->eflag;

	eflag=eflag & 0xfffffbff;

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

int func_op_cli(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	char bvar = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_cli(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_cmc(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	char bvar = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_cmc(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

int func_op_stc(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;
	char bvar=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	/*eflag*/

	eflag=in_context->eflag;

	eflag=eflag | 0x00000001;

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_std(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;
	char bvar=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	/*eflag*/

	eflag=in_context->eflag;

	eflag=eflag | 0x00000400;

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

int func_op_sti(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	char bvar = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_sti(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}
