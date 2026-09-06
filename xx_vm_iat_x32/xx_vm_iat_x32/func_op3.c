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

int func_op_and(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_xor(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_or(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_test(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_not(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_neg(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_bsr(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_bsf(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
////////////////////////////////////////////////////////////////////////

__declspec(noinline) int func_op_xor(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			xor al, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			xor ax, bx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			xor eax, ebx
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


__declspec(noinline) int func_op_and(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	item_sign_ext(var[0].size, var[1].size, (char*)&var[1].var);

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ecx, var2
			push eflag
			popfd
			and al, cl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ecx, var2
			push eflag
			popfd
			and ax, cx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ecx, var2
			push eflag
			popfd
			and eax, ecx
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



__declspec(noinline) int func_op_or(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	item_sign_ext(var[0].size, var[1].size, (char*)&var[1].var);

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ecx, var2
			push eflag
			popfd
			or al, cl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ecx, var2
			push eflag
			popfd
			or ax, cx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ecx, var2
			push eflag
			popfd
			or eax, ecx
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


__declspec(noinline) int func_op_test(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ecx, var2
			push eflag
			popfd
			test al, cl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ecx, var2
			push eflag
			popfd
			test ax, cx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ecx, var2
			push eflag
			popfd
			test eax, ecx
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

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_not(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			push eflag
			popfd
			not al
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			push eflag
			popfd
			not ax
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			push eflag
			popfd
			not eax
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


__declspec(noinline) int func_op_neg(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			push eflag
			popfd
			neg al
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			push eflag
			popfd
			neg ax
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			push eflag
			popfd
			neg eax
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


__declspec(noinline) int func_op_cmp(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			cmp al, bl
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			cmp ax, bx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			cmp eax, ebx
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

	out_context->eflag = eflag;
	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_bsr(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		return 0;
	}
	else if (var[0].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bsr ax, bx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bsr eax, ebx
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



__declspec(noinline) int func_op_bsf(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	int var1 = 0;
	int var2 = 0;
	int index = 0;
	unsigned int eflag = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1 = var[0].var;
	var2 = var[1].var;
	eflag = in_context->eflag;
	if (var[0].size == 1)
	{
		return 0;
	}
	else if (var[0].size == 2)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bsf ax, bx
			pushfd
			pop eflag
			mov var1, eax
			popad
		}
	}
	else if (var[0].size == 4)
	{
		__asm
		{
			pushad
			mov eax, var1
			mov ebx, var2
			push eflag
			popfd
			bsf eax, ebx
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
