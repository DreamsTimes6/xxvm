#include "stdio.h"
#include <windows.h>
#include <shlwapi.h>
#include "Plugin.h"
#include "xx_vm.h"
#include "xx_comm32.h"
#include "xxdisasm32.h"
#include "xx_opdef2.h"
#include "xx_link_list.h"


extern int xx_read_fmem(unsigned int addr,char* pdata,unsigned int datasize);
extern int xx_write_fmem(unsigned int addr,char* pdata,unsigned int datasize);
extern int calc_item(struct XX_INST *inst, int nitem, struct ITEM_VAR *out_var, struct XX_CONTEXT *out_context, unsigned  long result, int sign_flag);
extern void item_sign_ext(int size1,int size2,char *pvar2);

extern char debug_buf[500];
extern int debug_log(char *tmp);
///////////////////////////////////////////////////////////////////////////
int func_op_xadd(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_lahf(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_sahf(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_adc(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_nop(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_movsb(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_movsd(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_rdtsc(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_cpuid(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_mul(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_imul(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_imul1(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_imul2(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_imul3(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_div(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_idiv(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
////////////////////////////////////////////////////////////////////////////

__declspec(noinline) int func_op_xadd(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int var1=0;
	int var2=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1=var[0].var;
	var2=var[1].var;
	eflag=in_context->eflag;
	if(var[0].size==1)
	{
		__asm
		{
			pushad
			mov eax,var1
			mov ebx,var2
			push eflag
			popfd
			xadd al,bl
			pushfd
			pop eflag
			mov var1,eax
			mov var2,ebx
			popad
		}
	}
	else if(var[0].size==2)
	{
		__asm
		{
			pushad
			mov eax,var1
			mov ebx,var2
			push eflag
			popfd
			xadd ax,bx
			pushfd
			pop eflag
			mov var1,eax
			mov var2,ebx
			popad
		}
	}
	else if(var[0].size==4)
	{
		__asm
		{
			pushad
			mov eax,var1
			mov ebx,var2
			push eflag
			popfd
			xadd eax,ebx
			pushfd
			pop eflag
			mov var1,eax
			mov var2,ebx
			popad
		}
	}
	else
	{
		return 0;
	}

	iret=calc_item(xx_inst,1,var,out_context,var2,0);
	if(iret==0)
	{
		return 0;
	}

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}




__declspec(noinline) int func_op_lahf(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*eflag*/
	eflag=in_context->eflag;
	var1=in_context->r[REG_EAX];
	
		__asm
		{
			pushad
			mov eax,var1
			push eflag
			popfd
			lahf
			pushfd
			pop eflag
			mov var1,eax
			popad
		};

	out_context->r[REG_EAX]=var1;
	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}




__declspec(noinline) int func_op_sahf(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*eflag*/
	eflag=in_context->eflag;
	var1=in_context->r[REG_EAX];
	
		__asm
		{
			pushad
			mov eax,var1
			push eflag
			popfd
			sahf
			pushfd
			pop eflag
			mov var1,eax
			popad
		};

	out_context->r[REG_EAX]=var1;
	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_adc(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int var1=0;
	int var2=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	item_sign_ext(var[0].size,var[1].size,(char*)&var[1].var);

	/*eflag*/
	var1=var[0].var;
	var2=var[1].var;
	eflag=in_context->eflag;
	if(var[0].size==1)
	{
		__asm
		{
			pushad
			mov eax,var1
			mov ebx,var2
			push eflag
			popfd
			adc al,bl
			pushfd
			pop eflag
			mov var1,eax
			popad
		}
	}
	else if(var[0].size==2)
	{
		__asm
		{
			pushad
			mov eax,var1
			mov ebx,var2
			push eflag
			popfd
			adc ax,bx
			pushfd
			pop eflag
			mov var1,eax
			popad
		}
	}
	else if(var[0].size==4)
	{
		__asm
		{
			pushad
			mov eax,var1
			mov ebx,var2
			push eflag
			popfd
			adc eax,ebx
			pushfd
			pop eflag
			mov var1,eax
			popad
		}
	}
	else
	{
		return 0;
	}

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_nop(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_rdtsc(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	__asm
	{
		pushad
		rdtsc
		mov var1,edx
		mov var2,eax
		popad
	}

	out_context->r[REG_EDX]=var1;
	out_context->r[REG_EAX]=var2;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_cpuid(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	unsigned int var3=0;
	unsigned int var4=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	var1=out_context->r[REG_EAX];
	var2=out_context->r[REG_ECX];
	var3=out_context->r[REG_EDX];
	var4=out_context->r[REG_EBX];

	__asm
	{
		pushad
		mov eax,var1
		mov ecx,var2
		mov edx,var3
		mov ebx,var4
		cpuid
		mov var1,eax
		mov var2,ecx
		mov var3,edx
		mov var4,ebx
		popad
	}

	out_context->r[REG_EAX]=var1;
	out_context->r[REG_ECX]=var2;
	out_context->r[REG_EDX]=var3;
	out_context->r[REG_EBX]=var4;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_mul(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	unsigned int var3=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1=in_context->r[REG_EDX];
	var2=in_context->r[REG_EAX];
	var3=var[0].var;
	eflag=in_context->eflag;
	if(var[0].size==1)
	{
		__asm
		{
			pushad
			mov edx,var1
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			mul bl
			pushfd
			pop eflag
			mov var1,edx
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else if(var[0].size==2)
	{
		__asm
		{
			pushad
			mov edx,var1
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			mul bx
			pushfd
			pop eflag
			mov var1,edx
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else if(var[0].size==4)
	{
		__asm
		{
			pushad
			mov edx,var1
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			mul ebx
			pushfd
			pop eflag
			mov var1,edx
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else
	{
		return 0;
	}

	out_context->r[REG_EDX]=var1;
	out_context->r[REG_EAX]=var2;

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_div(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	unsigned int var3=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1=in_context->r[REG_EDX];
	var2=in_context->r[REG_EAX];
	var3=var[0].var;
	eflag=in_context->eflag;
	if(var[0].size==1)
	{
		__asm
		{
			pushad
			mov edx,var1
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			div bl
			pushfd
			pop eflag
			mov var1,edx
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else if(var[0].size==2)
	{
		__asm
		{
			pushad
			mov edx,var1
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			div bx
			pushfd
			pop eflag
			mov var1,edx
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else if(var[0].size==4)
	{
		__asm
		{
			pushad
			mov edx,var1
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			div ebx
			pushfd
			pop eflag
			mov var1,edx
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else
	{
		return 0;
	}

	out_context->r[REG_EDX]=var1;
	out_context->r[REG_EAX]=var2;

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_imul(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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

__declspec(noinline) int func_op_imul1(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int var1=0;
	int var2=0;
	int var3=0;
	int index=0;
	unsigned int eflag=0;
	
	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));
	
	/*eflag*/
	var1=in_context->r[REG_EDX];
	var2=in_context->r[REG_EAX];
	var3=var[0].var;
	eflag=in_context->eflag;
	if(var[0].size==1)
	{
		__asm
		{
			pushad
				mov edx,var1
				mov eax,var2
				mov ebx,var3
				push eflag
				popfd
				imul bl
				pushfd
				pop eflag
				mov var1,edx
				mov var2,eax
				mov var3,ebx
				popad
		}
	}
	else if(var[0].size==2)
	{
		__asm
		{
			pushad
				mov edx,var1
				mov eax,var2
				mov ebx,var3
				push eflag
				popfd
				imul bx
				pushfd
				pop eflag
				mov var1,edx
				mov var2,eax
				mov var3,ebx
				popad
		}
	}
	else if(var[0].size==4)
	{
		__asm
		{
			pushad
				mov edx,var1
				mov eax,var2
				mov ebx,var3
				push eflag
				popfd
				imul ebx
				pushfd
				pop eflag
				mov var1,edx
				mov var2,eax
				mov var3,ebx
				popad
		}
	}
	else
	{
		return 0;
	}
	
	out_context->r[REG_EDX]=var1;
	out_context->r[REG_EAX]=var2;
	//out_context->r[REG_ECX]=var3;
	
	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_imul2(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int var1=0;
	int var2=0;
	int var3=0;
	int index=0;
	unsigned int eflag=0;
	
	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));
	
	/*eflag*/
	var1=var[0].var;
	var2=var[1].var;
	//var3=var[0].var;
	eflag=in_context->eflag;
	if(var[0].size==1)
	{
		return 0;
	}
	else if(var[0].size==2)
	{
		__asm
		{
			pushad
				mov edx,var1
				mov eax,var2
				push eflag
				popfd
				imul dx,ax
				pushfd
				pop eflag
				mov var1,edx
				mov var2,eax
				popad
		}
	}
	else if(var[0].size==4)
	{
		__asm
		{
			pushad
				mov edx,var1
				mov eax,var2
				push eflag
				popfd
				imul edx,eax
				pushfd
				pop eflag
				mov var1,edx
				mov var2,eax
				popad
		}
	}
	else
	{
		return 0;
	}
	
	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	
	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_imul3(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int var1=0;
	int var2=0;
	int var3=0;
	int size1=0;
	int size2=0;
	int size3=0;
	int index=0;
	unsigned int eflag=0;
	
	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));
	
		/*eflag*/
	//var1=var[0].var;
	var2=var[1].var;
	var3=var[2].var;
	eflag=in_context->eflag;

	size1=var[0].size;
	size2=var[1].size;
	size3=var[2].size;
	
	if((size1==2) && (size2==2) && (size3==1))
	{
		//&& 
		__asm
		{
			pushad
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			imul ax,bl
			pushfd
			pop eflag
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	
	else if((size1==4) && (size2==4) && (size3==1))
	{
		//
		__asm
		{
			pushad
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			imul eax,bl
			pushfd
			pop eflag
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else if((size1==2) && (size2==2) && (size3==2))
	{
		//
		__asm
		{
			pushad
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			imul ax,bx
			pushfd
			pop eflag
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else if((size1==4) && (size2==4) && (size3==4))
	{
		//
		__asm
		{
			pushad
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			imul eax,ebx
			pushfd
			pop eflag
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else
	{
		return 0;
	}
	
	iret=calc_item(xx_inst,0,var,out_context,var2,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_idiv(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	unsigned int var3=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1=in_context->r[REG_EDX];
	var2=in_context->r[REG_EAX];
	var3=var[0].var;
	eflag=in_context->eflag;
	if(var[0].size==1)
	{
		__asm
		{
			pushad
			mov edx,var1
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			idiv bl
			pushfd
			pop eflag
			mov var1,edx
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else if(var[0].size==2)
	{
		__asm
		{
			pushad
			mov edx,var1
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			idiv bx
			pushfd
			pop eflag
			mov var1,edx
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else if(var[0].size==4)
	{
		__asm
		{
			pushad
			mov edx,var1
			mov eax,var2
			mov ebx,var3
			push eflag
			popfd
			idiv ebx
			pushfd
			pop eflag
			mov var1,edx
			mov var2,eax
			mov var3,ebx
			popad
		}
	}
	else
	{
		return 0;
	}

	out_context->r[REG_EDX]=var1;
	out_context->r[REG_EAX]=var2;
	//out_context->r[REG_ECX]=var3;

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}





