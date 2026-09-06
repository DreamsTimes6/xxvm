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
///////////////////////////////////////////////////////////////////////////
int func_op_sete(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setne(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_seta(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setae(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setb(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setbe(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setg(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setge(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setl(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setle(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_sets(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setns(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_seto(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setno(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setp(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
int func_op_setnp(struct XX_INST *,struct ITEM_VAR *,struct XX_CONTEXT *,struct XX_CONTEXT *);
////////////////////////////////////////////////////////////////////////////


__declspec(noinline) int func_op_sete(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//zf=1
	if((eflag & 0x00000040)==0x00000040)
	{
		var1=1;
	}

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_setne(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//zf=0
	if((eflag & 0x00000040)==0)
	{
		var1=1;
	}

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_seta(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//cf=0  zf=0
	if((eflag & 0x00000001)==0 && (eflag & 0x00000040)==0)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_setae(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//cf=0
	if((eflag & 0x00000001)==0)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}



__declspec(noinline) int func_op_setb(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//cf=1
	if((eflag & 0x00000001)==0x00000001)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_setbe(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//cf=1 or zf=1
	if((eflag & 0x00000001)==0x00000001 || (eflag & 0x00000040)==0x00000040)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_setg(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//zf=0 and sf=of
	if((eflag & 0x00000040)==0 )
	{
		if(((eflag & 0x00000880)==0x00000880) || ((eflag & 0x00000880)==0))
		{
			var1=1;
		}
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_setge(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//sf=of
	if(((eflag & 0x00000880)==0x00000880) || ((eflag & 0x00000880)==0))
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_setl(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//sf!=of
	if(((eflag & 0x00000880)!=0x00000880) && ((eflag & 0x00000880)!=0))
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_setle(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//zf=1 or sf!=of 
	if((eflag & 0x00000040)==0x00000040 )
	{
		var1=1;
	}
	else if(((eflag & 0x00000880)!=0x00000880) && ((eflag & 0x00000880)!=0))
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_sets(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//sf=1
	if((eflag & 0x00000080)==0x00000080)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_setns(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//sf=0
	if((eflag & 0x00000080)==0)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_seto(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//of=1
	if((eflag & 0x00000800)==0x00000800)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_setno(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//of=0
	if((eflag & 0x00000800)==0)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}


__declspec(noinline) int func_op_setp(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//pf=1
	if((eflag & 0x00000004)==0x00000004)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}

__declspec(noinline) int func_op_setnp(struct XX_INST *xx_inst,struct ITEM_VAR *var,struct XX_CONTEXT *in_context,struct XX_CONTEXT *out_context)
{
	int iret=0;
	unsigned int eflag=0;
	unsigned int var1=0;

	memcpy(out_context,in_context,sizeof(struct XX_CONTEXT));

	eflag=in_context->eflag;
	var1=0;
	//pf=0
	if((eflag & 0x00000004)==0)
	{
		var1=1;
	}
	

	iret=calc_item(xx_inst,0,var,out_context,var1,0);
	if(iret==0)
	{
		return 0;
	}
	
	out_context->ip=out_context->ip+xx_inst->xx_inst_code.disasm_length;
	return 1;
}







