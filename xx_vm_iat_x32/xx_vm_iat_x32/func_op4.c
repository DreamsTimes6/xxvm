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

int func_op_bt(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_btc(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_btr(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_bts(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);

int func_op_clac(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_clc(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cld(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cli(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_cmc(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);

int func_op_stc(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_std(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_sti(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);

////////////////////////////////////////////////////////////////////////

__declspec(noinline) int func_op_bt(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		return 0;
	}
	else if (var[0].size == 2 && var[1].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bt ax, bx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4 && var[1].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bt eax, ebx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2 && var[1].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bt ax, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4 && var[1].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bt eax, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else
	{
		return 0;
	}

#if 0
	iret = calc_item(xx_inst, 0, var, out_context, var1);
	if (iret == 0)
	{
		return 0;
	}
#endif

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_btc(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		return 0;
	}
	else if (var[0].size == 2 && var[1].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			btc ax, bx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4 && var[1].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			btc eax, ebx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2 && var[1].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			btc ax, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4 && var[1].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			btc eax, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else
	{
		return 0;
	}


	iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
	if (iret == 0)
	{
		return 0;
	}


	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_btr(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		return 0;
	}
	else if (var[0].size == 2 && var[1].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			btr ax, bx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4 && var[1].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			btr eax, ebx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2 && var[1].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			btr ax, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4 && var[1].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			btr eax, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else
	{
		return 0;
	}


	iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
	if (iret == 0)
	{
		return 0;
	}


	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_bts(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		return 0;
	}
	else if (var[0].size == 2 && var[1].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bts ax, bx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4 && var[1].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bts eax, ebx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2 && var[1].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bts ax, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4 && var[1].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bts eax, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else
	{
		return 0;
	}


	iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
	if (iret == 0)
	{
		return 0;
	}


	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_clac(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/

	eflag = in_context->eflag;

	eflag = eflag & 0xfffbffff;

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_clc(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/

	eflag = in_context->eflag;

	eflag = eflag & 0xfffffffe;

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cld(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/

	eflag = in_context->eflag;

	eflag = eflag & 0xfffffbff;

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_cli(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/

	eflag = in_context->eflag;

	__asm
	{
		pushad
		push eflag
		popfd
		cli
		pushfd
		pop eflag
		popad
	}

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_cmc(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/

	eflag = in_context->eflag;

	__asm
	{
		pushad
		push eflag
		popfd
		cmc
		pushfd
		pop eflag
		popad
	}

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_stc(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/

	eflag = in_context->eflag;

	eflag = eflag | 0x00000001;

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_std(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/

	eflag = in_context->eflag;

	eflag = eflag | 0x00000400;

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_sti(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int eflag = 0;
	char bvar = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/

	eflag = in_context->eflag;

	__asm
	{
		pushad
		push eflag
		popfd
		sti
		pushfd
		pop eflag
		popad
	}

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}
