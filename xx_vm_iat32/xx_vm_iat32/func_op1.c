#include "stdio.h"
#include <windows.h>
#include <shlwapi.h>
#include "Plugin.h"
#include "xx_iat.h"
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

/////////////////////////////////////////////////////////////////////////////////
int func_op_push(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_pop(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_pushad(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_popad(struct XX_INST *, struct ITEM_VAR *, struct XX_CONTEXT *, struct XX_CONTEXT *);
int func_op_call(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_retn(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_xchg(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_movsx(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_movzx(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_bswap(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_cdq(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_cwde(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_pushfd(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_popfd(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_mov(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_inc(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_dec(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_sbb(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_lea(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_add(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_sub(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
/////////////////////////////////////////////////////////////////////////////////
#if 0
int calc_eflag_zf(struct ITEM_VAR *arg,unsigned int result,unsigned int in_eflag,unsigned int *out_eflag)
{
	unsigned int var=0;
	unsigned int eflag=0;

	var=result;
	eflag=in_eflag;

	//zf
	if(var==0)
	{
		eflag=eflag | 0x00000040;
	}

	*out_eflag=eflag;

	return 1;
}

int calc_eflag_sf(struct ITEM_VAR *arg,unsigned int result,unsigned int in_eflag,unsigned int *out_eflag)
{
	unsigned int var=0;
	unsigned int eflag=0;

	var=result;
	eflag=in_eflag;

	//sf
	if(size==1)
	{
		if(var>=0x00000080)
		{
			eflag=eflag | 0x00000080;
		}
		else
		{
			eflag=eflag | 0x00000080;
		}
	}
	else if(size==2)
	{
		if(var>=0x00008000)
		{
			eflag=eflag | 0x00000080;
		}
		else
		{
			eflag=eflag | 0x00000080;
		}
	}
	else if(size==4)
	{
		if(var>=0x80000000)
		{
			eflag=eflag | 0x00000080;
		}
		else
		{
			eflag=eflag | 0x00000080;
		}
	}
	else
	{
		return 0;
	}

	*out_eflag=eflag;
	return 1;
}


int calc_eflag_cf(struct ITEM_VAR *arg,unsigned int result,unsigned int in_eflag,unsigned int *out_eflag)
{
	unsigned int var=0;
	unsigned int eflag=0;

	var=result;
	eflag=in_eflag;

	if(result>arg[0].var && result>arg[1].var)
	{
		eflag=eflag | 0x00000001;
	}
	else if(result<arg[0].var && result<arg[1].var)
	{
		eflag=eflag | 0x00000001;
	}

	*out_eflag=eflag;
	return 1;
}

int calc_eflag_of(struct ITEM_VAR *arg,unsigned int result,unsigned int in_eflag,unsigned int *out_eflag)
{
	unsigned int var=0;
	unsigned int eflag=0;

	var=result;
	eflag=in_eflag;


	*out_eflag=eflag;
	return 1;
}
#endif


__declspec(noinline) int func_op_push(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));
	out_context->r[REG_ESP] = out_context->r[REG_ESP] - var[0].size;

	/*写入数据*/
	iret = xx_write_fmem(out_context->r[REG_ESP], (unsigned char*)&var[0].var, var[0].size);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_pop(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*取数据*/
	iret = xx_read_fmem(out_context->r[REG_ESP], (unsigned char*)&var1, var[0].size);
	if (iret == 0)
	{
		return 0;
	}

	/*sp+4*/
	out_context->r[REG_ESP] = out_context->r[REG_ESP] + var[0].size;

	/*赋值*/
	iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_pushad(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	out_context->r[REG_ESP] = out_context->r[REG_ESP] - 4;
	iret = xx_write_fmem(out_context->r[REG_ESP], (unsigned char*)&out_context->r[REG_EAX], sizeof(out_context->r[REG_EAX]));
	if (iret == 0)
	{
		return 0;
	}

	out_context->r[REG_ESP] = out_context->r[REG_ESP] - 4;
	iret = xx_write_fmem(out_context->r[REG_ESP], (unsigned char*)&out_context->r[REG_ECX], sizeof(out_context->r[REG_ECX]));
	if (iret == 0)
	{
		return 0;
	}

	out_context->r[REG_ESP] = out_context->r[REG_ESP] - 4;
	iret = xx_write_fmem(out_context->r[REG_ESP], (unsigned char*)&out_context->r[REG_EDX], sizeof(out_context->r[REG_EDX]));
	if (iret == 0)
	{
		return 0;
	}

	out_context->r[REG_ESP] = out_context->r[REG_ESP] - 4;
	iret = xx_write_fmem(out_context->r[REG_ESP], (unsigned char*)&out_context->r[REG_EBX], sizeof(out_context->r[REG_EBX]));
	if (iret == 0)
	{
		return 0;
	}

	out_context->r[REG_ESP] = out_context->r[REG_ESP] - 4;
	iret = xx_write_fmem(out_context->r[REG_ESP], (unsigned char*)&in_context->r[REG_ESP], sizeof(in_context->r[REG_ESP]));
	if (iret == 0)
	{
		return 0;
	}

	out_context->r[REG_ESP] = out_context->r[REG_ESP] - 4;
	iret = xx_write_fmem(out_context->r[REG_ESP], (unsigned char*)&out_context->r[REG_EBP], sizeof(out_context->r[REG_EBP]));
	if (iret == 0)
	{
		return 0;
	}

	out_context->r[REG_ESP] = out_context->r[REG_ESP] - 4;
	iret = xx_write_fmem(out_context->r[REG_ESP], (unsigned char*)&out_context->r[REG_ESI], sizeof(out_context->r[REG_ESI]));
	if (iret == 0)
	{
		return 0;
	}

	out_context->r[REG_ESP] = out_context->r[REG_ESP] - 4;
	iret = xx_write_fmem(out_context->r[REG_ESP], (unsigned char*)&out_context->r[REG_EDI], sizeof(out_context->r[REG_EDI]));
	if (iret == 0)
	{
		return 0;
	}


	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}




__declspec(noinline) int func_op_popad(struct XX_INST *xx_inst, struct ITEM_VAR *var, struct XX_CONTEXT *in_context, struct XX_CONTEXT *out_context)
{
	int iret = 0;
	unsigned int var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT));

	/*取数据*/
	iret = xx_read_fmem(out_context->r[REG_ESP] + sizeof(out_context->r[REG_ESP]) * 0, \
		(unsigned char*)&out_context->r[REG_EDI], sizeof(out_context->r[REG_EDI]));
	if (iret == 0)
	{
		return 0;
	}

	iret = xx_read_fmem(out_context->r[REG_ESP] + sizeof(out_context->r[REG_ESP]) * 1, \
		(unsigned char*)&out_context->r[REG_ESI], sizeof(out_context->r[REG_ESI]));
	if (iret == 0)
	{
		return 0;
	}

	iret = xx_read_fmem(out_context->r[REG_ESP] + sizeof(out_context->r[REG_ESP]) * 2, \
		(unsigned char*)&out_context->r[REG_EBP], sizeof(out_context->r[REG_EBP]));
	if (iret == 0)
	{
		return 0;
	}

	iret = xx_read_fmem(out_context->r[REG_ESP] + sizeof(out_context->r[REG_ESP]) * 4, \
		(unsigned char*)&out_context->r[REG_EBX], sizeof(out_context->r[REG_EBX]));
	if (iret == 0)
	{
		return 0;
	}

	iret = xx_read_fmem(out_context->r[REG_ESP] + sizeof(out_context->r[REG_ESP]) * 5, \
		(unsigned char*)&out_context->r[REG_EDX], sizeof(out_context->r[REG_EDX]));
	if (iret == 0)
	{
		return 0;
	}

	iret = xx_read_fmem(out_context->r[REG_ESP] + sizeof(out_context->r[REG_ESP]) * 6, \
		(unsigned char*)&out_context->r[REG_ECX], sizeof(out_context->r[REG_ECX]));
	if (iret == 0)
	{
		return 0;
	}

	iret = xx_read_fmem(out_context->r[REG_ESP] + sizeof(out_context->r[REG_ESP]) * 7, \
		(unsigned char*)&out_context->r[REG_EAX], sizeof(out_context->r[REG_EAX]));
	if (iret == 0)
	{
		return 0;
	}

	out_context->r[REG_ESP] = out_context->r[REG_ESP] + 0x20;

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_call(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int ret_addr=0;
	

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	//out_context->ip=*(unsigned int*)xx_inst->xx_inst_items.xx_inst_items_var[0].item_const;
	out_context->ip=var[0].var;
	
	out_context->r[REG_ESP]=out_context->r[REG_ESP]-4;

	ret_addr=in_context->ip+xx_inst->xx_inst_code.disasm_length;
	/*写入数据*/
	iret=xx_write_fmem(out_context->r[REG_ESP],(unsigned char*)&ret_addr,sizeof(ret_addr));
	if(iret==0)
	{
		return 0;
	}
	
	return 1;
}


__declspec(noinline) int func_op_retn(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;

	/*有立即数和无立即数*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*id=[sp]*/
	iret=xx_read_fmem(out_context->r[REG_ESP],(unsigned char*)&var1,sizeof(out_context->ip));
	if(iret==0)
	{
		return 0;
	}
	out_context->ip=var1;

	/*sp+4*/
	out_context->r[REG_ESP]=out_context->r[REG_ESP]+4;

	/*sp+imm*/
	out_context->r[REG_ESP]=out_context->r[REG_ESP]+var[0].var;
	
	return 1;
}


__declspec(noinline) int func_op_movsx(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int index=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));
	/*处理过寄存器，按大小取值*/
	var1=0;
	if(var[1].size==1)
	{
		if(var[1].var>=0x80)
		{
			var1=0xffffff00;
		}
		var1=var1 | var[1].var;
	}
	else if(var[1].size==2)
	{
		if(var[1].var>=0x8000)
		{
			var1=0xffff0000;
		}
		var1=var1 | var[1].var;
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

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_movzx(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int index=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));
	/*处理过寄存器，按大小取值*/
	var1=0;
	var1=var[1].var;

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_bswap(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	int index=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));


	if(var[0].size==2)
	{
		var1=var[0].var&0xffff0000;
	}
	else
	{
		byte_opposite((unsigned char *)&var[0].var,var[0].size,(unsigned char *)&var1);
	}
	
	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_xchg(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	int index=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));


	var1=var[1].var;
	var2=var[0].var;

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	iret=calc_item(xx_inst,1,var,out_context,var2,0);
	if(iret==0)
	{
		return 0;
	}
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


/* cdq  66 cwd*/
__declspec(noinline) int func_op_cdq(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	int index=0;
	int prefix_flag=0;
	int n=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	for(n=0;n<xx_inst->xx_inst_code.prefix_len;n++)
	{
		if(xx_inst->xx_inst_code.prefix[n]==0x66)
		{
			prefix_flag=1;
		}
	}

	if(prefix_flag==1)
	{
		var1=in_context->r[REG_EAX];
		
		var1=var1 & 0x0000ffff;
		
		if(var1>=0x8000)
		{
			var2=0x0000ffff;
		}
		else
		{
			var2=0x00000000;
		}
		memcpy((char*)&out_context->r[REG_EDX],(char*)&var2,2);
	}
	else
	{
		var1=in_context->r[REG_EAX];
		
		//var1=var1 & 0x0000ffff;
		
		if(var1>=0x80000000)
		{
			var2=0xffffffff;
		}
		else
		{
			var2=0x00000000;
		}
		memcpy((char*)&out_context->r[REG_EDX],(char*)&var2,4);
	}

	

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

/*cwde  66 cbw */
__declspec(noinline) int func_op_cwde(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	int index=0;
	int prefix_flag=0;
	int n=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	for(n=0;n<xx_inst->xx_inst_code.prefix_len;n++)
	{
		if(xx_inst->xx_inst_code.prefix[n]==0x66)
		{
			prefix_flag=1;
		}
	}

	if(prefix_flag==1)
	{
		var1=in_context->r[REG_EAX];
		
		var1=var1 & 0x000000ff;
		
		if(var1>=0x80)
		{
			var2=0x000000ff;
		}
		else
		{
			var2=0x00000000;
		}
		memcpy((char*)&out_context->r[REG_EAX]+1,(char*)&var2,1);
	}
	else
	{
		var1=in_context->r[REG_EAX];
		
		var1=var1 & 0x0000ffff;
		
		if(var1>=0x8000)
		{
			var2=0x0000ffff;
		}
		else
		{
			var2=0x00000000;
		}
		memcpy((char*)&out_context->r[REG_EAX]+2,(char*)&var2,2);
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_pushfd(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));
	out_context->r[REG_ESP]=out_context->r[REG_ESP]-4;

	/*写入数据*/
	iret=xx_write_fmem(out_context->r[REG_ESP],(unsigned char*)&in_context->eflag,sizeof(in_context->eflag));
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_popfd(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	/*取数据*/
	iret=xx_read_fmem(out_context->r[REG_ESP],(unsigned char*)&var1,sizeof(in_context->eflag));
	if(iret==0)
	{
		return 0;
	}

	/*赋值*/
	out_context->eflag=var1;

	/*sp+4*/
	out_context->r[REG_ESP]=out_context->r[REG_ESP]+4;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_mov(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int var1=0;
	unsigned int var2=0;
	int index=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));


	var1=var[1].var;


	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_dec(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int var1=0;
	int var2=0;
	int index=0;
	unsigned int eflag=0;


	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	var1=var[0].var;

	var1=var1-1;

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}

	/*eflag*/
	var1=var[0].var;
	
	eflag=in_context->eflag;
	if(var[0].size==1)
	{
		__asm
		{
			pushad
			mov eax,var1
			push eflag
			popfd
			dec al
			pushfd
			pop eflag
			popad
		}
	}
	else if(var[0].size==2)
	{
		__asm
		{
			pushad
			mov eax,var1
			push eflag
			popfd
			dec ax
			pushfd
			pop eflag
			popad
		}
	}
	else if(var[0].size==4)
	{
		__asm
		{
			pushad
			mov eax,var1
			push eflag
			popfd
			dec eax
			pushfd
			pop eflag
			popad
		}
	}
	else
	{
		return 0;
	}

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_inc(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int var1=0;
	int var2=0;
	int index=0;
	unsigned int eflag=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	var1=var[0].var;

	var1=var1+1;

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}

	/*eflag*/
	var1=var[0].var;
	eflag=in_context->eflag;
	if(var[0].size==1)
	{
		__asm
		{
			pushad
			mov eax,var1
			push eflag
			popfd
			inc al
			pushfd
			pop eflag
			popad
		}
	}
	else if(var[0].size==2)
	{
		__asm
		{
			pushad
			mov eax,var1
			push eflag
			popfd
			inc ax
			pushfd
			pop eflag
			popad
		}
	}
	else if(var[0].size==4)
	{
		__asm
		{
			pushad
			mov eax,var1
			push eflag
			popfd
			inc eax
			pushfd
			pop eflag
			popad
		}
	}
	else
	{
		return 0;
	}

	out_context->eflag=eflag;
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}




__declspec(noinline) int func_op_sbb(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			mov ecx,var2
			push eflag
			popfd
			sbb al,cl
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
			sbb ax,cx
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
			sbb eax,ecx
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


__declspec(noinline) int func_op_lea(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	int var1=0;
	int var2=0;
	int index=0;


	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	iret=calc_item(xx_inst,0,var,out_context,var[1].addr,0);
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_add(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			add al,bl
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
			add ax,bx
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
			add eax,ebx
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



__declspec(noinline) int func_op_sub(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
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
			sub al,bl
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
			sub ax,bx
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
			sub eax,ebx
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
