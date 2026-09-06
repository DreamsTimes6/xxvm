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
int func_op_jmp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_je(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jne(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_ja(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jae(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jb(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jbe(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jg(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jge(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jl(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jle(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jnc(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jo(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jno(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_js(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jns(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jnp(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
int func_op_jecxz(struct XX_INST *,struct ITEM_VAR64 *,struct XX_CONTEXT64 *,struct XX_CONTEXT64 *);
////////////////////////////////////////////////////////////////////////

int func_op_jmp(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!=INST_SYSTEM_64)
	{
		return 0;
	}

	out_context->ip=var[0].var;
	
	return 1;
}


int func_op_je(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//zf=1
	if((eflag & 0x00000040)==0x00000040)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


int func_op_jne(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//zf=0
	if((eflag & 0x00000040)==0)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}



int func_op_ja(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//cf=0  zf=0
	if((eflag & 0x00000001)==0 && (eflag & 0x00000040)==0)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


int func_op_jae(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//cf=0
	if((eflag & 0x00000001)==0)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}



int func_op_jb(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//cf=1
	if((eflag & 0x00000001)==0x00000001)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


int func_op_jbe(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//cf=1 or zf=1
	if((eflag & 0x00000001)==0x00000001 || (eflag & 0x00000040)==0x00000040)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}




int func_op_jg(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;


	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//zf=0 and sf=of
	if((eflag & 0x00000040)==0 )
	{
		if(((eflag & 0x00000880)==0x00000880) || ((eflag & 0x00000880)==0))
		{
			out_context->ip=var[0].var;
		}
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


int func_op_jge(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//sf=of
	if(((eflag & 0x00000880)==0x00000880) || ((eflag & 0x00000880)==0))
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}




int func_op_jl(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//sf!=of
	if(((eflag & 0x00000880)!=0x00000880) && ((eflag & 0x00000880)!=0))
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


int func_op_jle(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//zf=1 or sf!=of 
	if((eflag & 0x00000040)==0x00000040 )
	{
		out_context->ip=var[0].var;
	}
	else if(((eflag & 0x00000880)!=0x00000880) && ((eflag & 0x00000880)!=0))
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}



int func_op_jc(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//cf=1
	if((eflag & 0x00000001)==0x00000001)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


int func_op_jnc(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//cf=0
	if((eflag & 0x00000001)==0)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}



int func_op_jo(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//of=1
	if((eflag & 0x00000800)==0x00000800)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


int func_op_jno(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//of=0
	if((eflag & 0x00000800)==0)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}




int func_op_js(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//sf=1
	if((eflag & 0x00000080)==0x00000080)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


int func_op_jns(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//sf=0
	if((eflag & 0x00000080)==0)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}

//jp  jpe
int func_op_jp(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//pf=1
	if((eflag & 0x00000004)==0x00000004)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


//jpo jnp
int func_op_jnp(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 eflag=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	eflag=in_context->eflag;
	//pf=0
	if((eflag & 0x00000004)==0)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}


int func_op_jecxz(struct XX_INST *xx_inst,struct ITEM_VAR64 *var,struct XX_CONTEXT64 *in_context,struct XX_CONTEXT64 *out_context)
{
	int iret=0;
	ulong64 var1=0;

	/*api则退出*/

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT64));

	if(var[0].size!= INST_SYSTEM_64)
	{
		return 0;
	}

	var1=in_context->r[REG_RCX];
	//ecx=0
	if(var1==0)
	{
		out_context->ip=var[0].var;
	}
	else
	{
		out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	}
	
	return 1;
}

