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
int func_op_xadd(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_lahf(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_sahf(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_adc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_nop(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_rdtsc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cpuid(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_mul(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_imul(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_imul1(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_imul2(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_imul3(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_div(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_idiv(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern int asm_xadd(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_lahf(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_sahf(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_adc(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_rdtsc(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_cpuid(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_mul(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_div(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_imul1(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_imul2(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_imul3(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_idiv(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int func_op_xadd(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_xadd(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];
	var2 = out_var.o_var[1];

	iret = calc_item(xx_inst, 1, var, out_context, var2, 2);
	if (iret == 0)
	{
		return 0;
	}

	/*必须后赋值操作数1*/
	iret = calc_item(xx_inst, 0, var, out_context, var1, 2);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;

}




int func_op_lahf(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;
	ulong64 eflag=0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_lahf(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	out_context->r[REG_RAX]=var1;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}




int func_op_sahf(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_lahf(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	out_context->r[REG_RAX] = var1;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_adc(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
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
	iret = asm_adc(var, in_context, &out_var);
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


int func_op_nop(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;
	ulong64 eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}






int func_op_rdtsc(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_rdtsc(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	//out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];
	var2 = out_var.o_var[1];


	out_context->r[REG_RDX]=var1;
	out_context->r[REG_RAX]=var2;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_cpuid(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	ulong64 var3 = 0;
	ulong64 var4 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	var1=out_context->r[REG_RAX];
	var2=out_context->r[REG_RCX];
	var3=out_context->r[REG_RDX];
	var4=out_context->r[REG_RBX];

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_cpuid(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	//out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];
	var2 = out_var.o_var[1];
	var3 = out_var.o_var[2];
	var4 = out_var.o_var[3];

	out_context->r[REG_RAX]=var1;
	out_context->r[REG_RCX]=var2;
	out_context->r[REG_RDX]=var3;
	out_context->r[REG_RBX]=var4;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_mul(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
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
	iret = asm_mul(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];
	var2 = out_var.o_var[1];

	out_context->r[REG_RDX]=var1;
	out_context->r[REG_RAX]=var2;
	//out_context->r[REG_ECX]=var3;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_div(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
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
	iret = asm_div(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];
	var2 = out_var.o_var[1];

	out_context->r[REG_RDX] = var1;
	out_context->r[REG_RAX] = var2;
	//out_context->r[REG_ECX]=var3;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_imul(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;

	switch(xx_inst->xx_inst_items.nitem)
	{
	case 1:
		iret=func_op_imul1(xx_inst,var,in_context,out_context);
		if(iret==0)
		{
			return 0;
		}
		break;
	case 2:
		iret=func_op_imul2(xx_inst,var,in_context,out_context);
		if(iret==0)
		{
			return 0;
		}
		break;
	case 3:
		iret=func_op_imul3(xx_inst,var,in_context,out_context);
		if(iret==0)
		{
			return 0;
		}
		break;
	default:
		return 0;
	}
	
	return 1;
}

int func_op_imul1(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
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
	iret = asm_imul1(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];
	var2 = out_var.o_var[1];

	out_context->r[REG_RDX] = var1;
	out_context->r[REG_RAX] = var2;
	//out_context->r[REG_ECX]=var3;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_imul2(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
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
	iret = asm_imul2(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	iret=calc_item(xx_inst,0,var,out_context,var1, 0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_imul3(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
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
	iret = asm_imul2(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	
	iret=calc_item(xx_inst,0,var,out_context,var1, 0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_idiv(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
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
	iret = asm_idiv(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];
	var2 = out_var.o_var[1];

	out_context->r[REG_RDX] = var1;
	out_context->r[REG_RAX] = var2;
	//out_context->r[REG_ECX]=var3;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}





