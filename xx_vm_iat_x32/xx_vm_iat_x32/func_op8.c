#include "stdio.h"
#include <windows.h>
#include <shlwapi.h>
//#include "Plugin.h"
#include "xx_iat.h"
#include "xx_comm32.h"
#include "xxdisasm32.h"
#include "xx_opdef2.h"
#include "xx_link_list.h"


extern int xx_read_fmem(unsigned long addr, char* pdata, unsigned int datasize);
extern int xx_write_fmem(unsigned long addr, char* pdata, unsigned int datasize);
extern int calc_item(struct XX_INST *inst, int nitem, struct ITEM_VAR *out_var, struct XX_CONTEXT *out_context, unsigned  long result, int sign_flag);
extern void item_sign_ext(unsigned long size1, unsigned long size2, unsigned char *pvar2);

extern char debug_buf[500];
extern int debug_log(char *tmp);
///////////////////////////////////////////////////////////////////////////
int func_op_cmove(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovne(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmova(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovae(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovb(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovbe(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovg(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovge(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovl(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovle(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovs(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovns(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovo(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovno(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovp(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmovnp(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
////////////////////////////////////////////////////////////////////////////


__declspec(noinline) int func_op_cmove(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//zf=1
	if ((eflag & 0x00000040) == 0x00000040)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cmovne(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//zf=0
	if ((eflag & 0x00000040) == 0)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cmova(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//cf=0  zf=0
	if ((eflag & 0x00000001) == 0 && (eflag & 0x00000040) == 0)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cmovae(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//cf=0
	if ((eflag & 0x00000001) == 0)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_cmovb(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//cf=1
	if ((eflag & 0x00000001) == 0x00000001)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cmovbe(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//cf=1 or zf=1
	if ((eflag & 0x00000001) == 0x00000001 || (eflag & 0x00000040) == 0x00000040)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_cmovg(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//zf=0 and sf=of
	if ((eflag & 0x00000040) == 0)
	{
		if (((eflag & 0x00000880) == 0x00000880) || ((eflag & 0x00000880) == 0))
		{
			var1 = var[1].var;

			iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
			if (iret == 0)
			{
				return 0;
			}
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cmovge(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//sf=of
	if (((eflag & 0x00000880) == 0x00000880) || ((eflag & 0x00000880) == 0))
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_cmovl(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//sf!=of
	if (((eflag & 0x00000880) != 0x00000880) && ((eflag & 0x00000880) != 0))
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cmovle(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//zf=1 or sf!=of 
	if ((eflag & 0x00000040) == 0x00000040)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}
	else if (((eflag & 0x00000880) != 0x00000880) && ((eflag & 0x00000880) != 0))
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_cmovs(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//sf=1
	if ((eflag & 0x00000080) == 0x00000080)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cmovns(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//sf=0
	if ((eflag & 0x00000080) == 0)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_cmovo(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//of=1
	if ((eflag & 0x00000800) == 0x00000800)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cmovno(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//of=0
	if ((eflag & 0x00000800) == 0)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_cmovp(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//pf=1
	if ((eflag & 0x00000004) == 0x00000004)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cmovnp(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	eflag = in_context->eflag;
	var1 = 0;
	//pf=0
	if ((eflag & 0x00000004) == 0)
	{
		var1 = var[1].var;

		iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
		if (iret == 0)
		{
			return 0;
		}
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}







