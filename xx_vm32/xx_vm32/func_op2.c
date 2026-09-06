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
int func_op_sal(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_sar(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_shl(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_shr(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);

int func_op_rcl(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_rcr(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_rol(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_ror(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);

int func_op_shld(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_shrd(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);

////////////////////////////////////////////////////////////////////////

__declspec(noinline) int func_op_sal(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			mov ecx,var2
			push eflag
			popfd
			sal al,cl
			//_emit 0xd2
			//_emit 0xf0
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
			mov ecx,var2
			push eflag
			popfd
			sal ax,cl
			//_emit 0x66
			//_emit 0xd3
			//_emit 0xf0
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
			mov ecx,var2
			push eflag
			popfd
			sal eax,cl
			//_emit 0xd3
			//_emit 0xf0
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



__declspec(noinline) int func_op_sar(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			mov ecx,var2
			push eflag
			popfd
			sar al,cl
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
			mov ecx,var2
			push eflag
			popfd
			sar ax,cl
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
			mov ecx,var2
			push eflag
			popfd
			sar eax,cl
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



__declspec(noinline) int func_op_shl(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			mov ecx,var2
			push eflag
			popfd
			shl al,cl
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
			mov ecx,var2
			push eflag
			popfd
			shl ax,cl
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
			mov ecx,var2
			push eflag
			popfd
			shl eax,cl
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



__declspec(noinline) int func_op_shr(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			mov ecx,var2
			push eflag
			popfd
			shr al,cl
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
			mov ecx,var2
			push eflag
			popfd
			shr ax,cl
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
			mov ecx,var2
			push eflag
			popfd
			shr eax,cl
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



__declspec(noinline) int func_op_rcl(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			mov ecx,var2
			push eflag
			popfd
			rcl al,cl
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
			mov ecx,var2
			push eflag
			popfd
			rcl ax,cl
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
			mov ecx,var2
			push eflag
			popfd
			rcl eax,cl
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


__declspec(noinline) int func_op_rcr(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			mov ecx,var2
			push eflag
			popfd
			rcr al,cl
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
			mov ecx,var2
			push eflag
			popfd
			rcr ax,cl
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
			mov ecx,var2
			push eflag
			popfd
			rcr eax,cl
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


__declspec(noinline) int func_op_rol(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			mov ecx,var2
			push eflag
			popfd
			rol al,cl
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
			mov ecx,var2
			push eflag
			popfd
			rol ax,cl
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
			mov ecx,var2
			push eflag
			popfd
			rol eax,cl
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


__declspec(noinline) int func_op_ror(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			mov ecx,var2
			push eflag
			popfd
			ror al,cl
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
			mov ecx,var2
			push eflag
			popfd
			ror ax,cl
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
			mov ecx,var2
			push eflag
			popfd
			ror eax,cl
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



__declspec(noinline) int func_op_shld(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	unsigned int var3=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1=var[0].var;
	var2=var[1].var;
	var3=var[2].var;
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
			mov ecx,var3
			push eflag
			popfd
			shld dx,ax,cl
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
			mov ecx,var3
			push eflag
			popfd
			shld edx,eax,cl
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


__declspec(noinline) int func_op_shrd(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	unsigned int var3=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*eflag*/
	var1=var[0].var;
	var2=var[1].var;
	var3=var[2].var;
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
			mov ecx,var3
			push eflag
			popfd
			shrd dx,ax,cl
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
			mov ecx,var3
			push eflag
			popfd
			shrd edx,eax,cl
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

