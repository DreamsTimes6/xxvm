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


extern int xx_read_fmem(unsigned long long addr,char* pdata,unsigned int datasize);
extern int xx_write_fmem(unsigned long long addr,char* pdata,unsigned int datasize);
extern int calc_item(struct XX_INST *inst,int nitem,struct ITEM_VAR64 *out_var,struct XX_CONTEXT64 *out_context,unsigned long long result, int);
extern void  item_sign_ext(unsigned long long size1, unsigned long long size2, unsigned char *pvar2);

extern char debug_buf[500];
extern int debug_log(char *tmp);
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
int func_op_push(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_pop(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_call(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_retn(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_xchg(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_movsx(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_movzx(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_bswap(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cdq(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_cwde(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_pushfd(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_popfd(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_mov(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_inc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_dec(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_sbb(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_lea(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_add(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_sub(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern int asm_dec(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_inc(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_sbb(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_add(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
extern int asm_sub(struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct OUT_VAR64 *out_var);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


int func_op_push(struct XX_INST *xx_inst, struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct XX_CONTEXT64 *out_context)
{
	int iret = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));
	out_context->r[REG_RSP] = out_context->r[REG_RSP] - var[0].size;

	/*写入数据*/
	iret = xx_write_fmem(out_context->r[REG_RSP], (unsigned char*)&var[0].var, (unsigned long)var[0].size);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}

int func_op_pop(struct XX_INST *xx_inst, struct ITEM_VAR64 *var, struct XX_CONTEXT64 *in_context, struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));


	/*取数据*/
	iret = xx_read_fmem(out_context->r[REG_RSP], (unsigned char*)&var1, var[0].size);
	if (iret == 0)
	{
		return 0;
	}

	/*sp+4*/
	out_context->r[REG_RSP] = out_context->r[REG_RSP] + var[0].size;

	/*赋值*/
	iret = calc_item(xx_inst, 0, var, out_context, var1, 0);
	if (iret == 0)
	{
		return 0;
	}

	out_context->ip = out_context->ip + xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_call(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 ret_addr=0;
	
	//只处理call offset

	if (xx_inst->xx_inst_items.xx_inst_items_flag[0].item_type != INST_ITEM_TYPE_DIS)
	{
		return 0;
	}

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	//out_context->ip=*(unsigned int*)xx_inst->xx_inst_items.xx_inst_items_var[0].item_const;
	out_context->ip=var[0].var;
	
	out_context->r[REG_RSP]=out_context->r[REG_RSP]-8;

	ret_addr=in_context->ip+xx_inst->xx_inst_code.disasm_length;
	/*写入数据*/
	iret=xx_write_fmem(out_context->r[REG_RSP],(unsigned char*)&ret_addr,sizeof(ret_addr));
	if(iret==0)
	{
		return 0;
	}
	
	return 1;
}


int func_op_retn(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;

	/*有立即数和无立即数*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	/*id=[sp]*/
	iret=xx_read_fmem(out_context->r[REG_RSP],(unsigned char*)&var1,sizeof(out_context->ip));
	if(iret==0)
	{
		return 0;
	}
	out_context->ip=var1;

	/*sp+4*/
	out_context->r[REG_RSP]=out_context->r[REG_RSP]+8;

	/*sp+imm*/
	out_context->r[REG_RSP]=out_context->r[REG_RSP]+var[0].var;
	
	return 1;
}


int func_op_movsx(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	int index=0;
	ulong64 var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));
	/*处理过寄存器，按大小取值*/
	var1=0;
	if (var[0].size != 8)
	{
		if (var[1].size == 1)
		{
			if ((var[1].var & 0x00000000000000ff) >= 0x80)
			{
				var1 = 0xffffff00;
			}
			var1 = var1 | (var[1].var & 0x00000000000000ff);
		}
		else if (var[1].size == 2)
		{
			if ((var[1].var & 0x000000000000ffff) >= 0x8000)
			{
				var1 = 0xffff0000;
			}
			var1 = var1 | (var[1].var & 0x000000000000ffff);
		}
		else if (var[1].size == 4)
		{
			if ((var[1].var & 0x00000000ffffffff) >= 0x80000000)
			{
				var1 = 0xffffffff00000000;
			}
			var1 = var1 | (var[1].var & 0x00000000ffffffff);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		
		if (*xx_inst->xx_inst_code.first_opcode == 0x63)
		{
			var[1].size = 4;
			if ((var[1].var & 0x00000000ffffffff) >= 0x80000000)
			{
				var1 = 0xffffffff00000000;
			}
			var1 = var1 | (var[1].var & 0x00000000ffffffff);
		}
		else if (*xx_inst->xx_inst_code.first_opcode == 0x0f && \
			*xx_inst->xx_inst_code.second_opcode == 0xbe)
		{
			var[1].size = 1;
			if ((var[1].var & 0x00000000000000ff) >= 0x80)
			{
				var1 = 0xffffffffffffff00;
			}
			var1 = var1 | (var[1].var & 0x00000000000000ff);
		}
		else if (*xx_inst->xx_inst_code.first_opcode == 0x0f && \
			*xx_inst->xx_inst_code.second_opcode == 0xbf)
		{
			var[1].size = 2;
			if ((var[1].var & 0x000000000000ffff)>= 0x8000)
			{
				var1 = 0xffffffffffff0000;
			}
			var1 = var1 | (var[1].var & 0x000000000000ffff);
		}
		else
		{
			return 0;
		}
	}

	iret=calc_item(xx_inst,0,var,out_context,var1, 2);
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_movzx(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	int index=0;
	ulong64 var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));
	/*处理过寄存器，按大小取值*/
	var1=0;
	var1=var[1].var;

	if (var[0].size == 8)
	{
		var1 = 0;
		if (*xx_inst->xx_inst_code.first_opcode == 0x0f && \
			*xx_inst->xx_inst_code.second_opcode == 0xb6)
		{
			var[1].size = 1;
			var1 = 0x00000000000000ff & var[1].var;
		}
		else if (*xx_inst->xx_inst_code.first_opcode == 0x0f && \
			*xx_inst->xx_inst_code.second_opcode == 0xb7)
		{
			var[1].size = 2;
			var1 = 0x000000000000ffff & var[1].var;
		}
		else
		{
			return 0;
		}
	}

	iret=calc_item(xx_inst,0,var,out_context,var1, 2);
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_bswap(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	int index=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));


	if (var[0].size == INST_SYSTEM_16 || var[0].size ==1)
	{
		var1 = var[0].var & 0xffffffffffff0000;
	}
	else
	{
		byte_opposite((unsigned char *)&var[0].var, (unsigned long)var[0].size, (unsigned char *)&var1);
	}
	
	iret=calc_item(xx_inst,0,var,out_context,var1, 2);
	if(iret==0)
	{
		return 0;
	}
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_xchg(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));


	var1=var[1].var;
	var2=var[0].var;

	iret=calc_item(xx_inst,0,var,out_context,var1, 2);
	if(iret==0)
	{
		return 0;
	}
	iret=calc_item(xx_inst,1,var,out_context,var2, 2);
	if(iret==0)
	{
		return 0;
	}
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


/* cdq  66 cwd*/
int func_op_cdq(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64  var1=0;
	ulong64  var2=0;
	int index=0;
	int prefix_flag=0;
	int n=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	for(n=0;n<xx_inst->xx_inst_code.prefix_len;n++)
	{
		if(xx_inst->xx_inst_code.prefix[n]==0x66)
		{
			prefix_flag=1;
		}
	}

	var2 = 0;
	if(prefix_flag==1)
	{
		var1=in_context->r[REG_RAX];
		
		var1=var1 & 0x0000ffff;
		
		if(var1>=0x8000)
		{
			var2=0x0000ffff;
		}
		else
		{
			var2=0x00000000;
		}
		memcpy((char*)&out_context->r[REG_RDX],(char*)&var2,2);
	}
	else
	{
		if (xx_inst->xx_inst_exist.rexw_flag != INST_FLAG_EXIST)
		{
			var1 = in_context->r[REG_RAX];

			//var1=var1 & 0x0000ffff;

			if ((var1 & 0x00000000ffffffff)>= 0x80000000)
			{
				var2 = 0x00000000ffffffff;
			}
			else
			{
				var2 = 0x0000000000000000;
			}
			memcpy((char*)&out_context->r[REG_RDX], (char*)&var2, INST_SYSTEM_64);
		}
		else if(xx_inst->xx_inst_exist.rexw_flag == INST_FLAG_EXIST)
		{
			var1 = in_context->r[REG_RAX];

			//var1=var1 & 0x0000ffff;

			if (var1 >= 0x8000000000000000)
			{
				var2 = 0xffffffffffffffff;
			}
			else
			{
				var2 = 0x0000000000000000;
			}
			memcpy((char*)&out_context->r[REG_RDX], (char*)&var2, INST_SYSTEM_64);
		}
	}

	

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

/*cwde  66 cbw */
int func_op_cwde(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;
	int prefix_flag=0;
	int n=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	for(n=0;n<xx_inst->xx_inst_code.prefix_len;n++)
	{
		if(xx_inst->xx_inst_code.prefix[n]==0x66)
		{
			prefix_flag=1;
		}
	}

	if(prefix_flag==1)
	{
		var1=in_context->r[REG_RAX];
		
		var1=var1 & 0x000000ff;
		
		if(var1>=0x80)
		{
			var2=0x000000ff;
		}
		else
		{
			var2=0x00000000;
		}
		memcpy((char*)&out_context->r[REG_RAX]+1,(char*)&var2,1);
	}
	else
	{
		if (xx_inst->xx_inst_exist.rexw_flag != INST_FLAG_EXIST)
		{
			var1 = in_context->r[REG_RAX];

			var1 = var1 & 0x000000000000ffff;

			if (var1 >= 0x8000)
			{
				//var2 = 0x000000000000ffff;
				out_context->r[REG_RAX] = var1 | 0x00000000ffff0000;
			}
			else
			{
				//var2 = 0x0000000000000000;
				out_context->r[REG_RAX] = var1 | 0x0000000000000000;
			}
			//memcpy((char*)&out_context->r[REG_RAX] + 2, (char*)&var2, 2);
		}
		else if (xx_inst->xx_inst_exist.rexw_flag == INST_FLAG_EXIST)
		{
			var1 = in_context->r[REG_RAX];

			var1=var1 & 0x00000000ffffffff;

			if (var1 >= 0x0000000080000000)
			{
				out_context->r[REG_RAX] = var1 | 0xffffffff00000000;
			}
			else
			{
				out_context->r[REG_RAX] = var1 | 0x0000000000000000;
			}
			//memcpy((char*)&out_context->r[REG_RDX], (char*)&var2, INST_SYSTEM_64);
		}
			
		
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_pushfd(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));
	out_context->r[REG_RSP]=out_context->r[REG_RSP]-8;

	/*写入数据*/
	iret=xx_write_fmem(out_context->r[REG_RSP],(unsigned char*)&in_context->eflag,sizeof(in_context->eflag));
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_popfd(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	/*取数据*/
	iret=xx_read_fmem(out_context->r[REG_RSP],(unsigned char*)&var1,sizeof(in_context->eflag));
	if(iret==0)
	{
		return 0;
	}

	/*赋值*/
	out_context->eflag=var1;

	/*sp+4*/
	out_context->r[REG_RSP]=out_context->r[REG_RSP]+8;

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_mov(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));


	var1=var[1].var;


	iret=calc_item(xx_inst,0,var,out_context,var1, 2);
	if(iret==0)
	{
		return 0;
	}
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


#if 0

int test_func(ulong64 *result)
{
	struct ITEM_VAR64 in_var[4];
	struct OUT_VAR64 out_var;
	struct XX_CONTEXT64 in_context;

	memset(in_var, 0, sizeof(in_var));
	memset(&out_var, 0, sizeof(out_var));
	memset(&in_context,0,sizeof(in_context));

	in_var[0].var = 3;
	in_var[0].size = 8;
	in_var[0].addr = 0x0000000011223344;
	in_context.eflag = 0x204;
	dec_1(in_var,&in_context,&out_var);

	*result = out_var.o_var[0];
	return 1;
}
#endif


int func_op_dec(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;
	ulong64 eflag=0;
	struct OUT_VAR64 out_var;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret=asm_dec(var,in_context,&out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	iret=calc_item(xx_inst,0,var,out_context,var1, 2);
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_inc(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret = 0;
	ulong64 var1 = 0;
	ulong64 var2 = 0;
	int index = 0;
	ulong64 eflag = 0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_inc(var, in_context, &out_var);
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



int func_op_sbb(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;
	ulong64 eflag=0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	item_sign_ext(var[0].size, var[1].size, (char*)&var[1].var);

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_sbb(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	iret=calc_item(xx_inst,0,var,out_context,var1, 2);
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


int func_op_lea(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;


	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	iret=calc_item(xx_inst,0,var,out_context,var[1].addr, 2);
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_add(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;
	ulong64 var2=0;
	int index=0;
	ulong64 eflag=0;
	struct OUT_VAR64 out_var;

	memcpy(out_context, in_context, sizeof(struct XX_CONTEXT64));

	item_sign_ext(var[0].size, var[1].size, (char*)&var[1].var);

	memset(&out_var, 0, sizeof(out_var));
	iret = asm_add(var, in_context, &out_var);
	if (iret == 0)
	{
		return 0;
	}

	out_context->eflag = out_var.o_flag;
	var1 = out_var.o_var[0];

	iret=calc_item(xx_inst,0,var,out_context,var1, 2);
	if(iret==0)
	{
		return 0;
	}

	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



int func_op_sub(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
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
	iret = asm_sub(var, in_context, &out_var);
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



