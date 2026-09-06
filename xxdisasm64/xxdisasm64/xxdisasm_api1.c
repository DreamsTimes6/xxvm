#include <string.h>
#include <stdlib.h>
#include "xx_inst.h"
#include "xx_err.h"
//////////////////////////////////

ulong g_count=0;
struct XX_INST *g_inst=0;
ulong err_code=0;


extern void xx_disasm_free();
extern void xx_disasm(struct XX_INST* xx_inst,uchar *codes);
////////////////////////////////
  DLL_PUBLIC int xxdisasm(uchar *pdata,int inst_system,ulong mod_base,ulong inst_addr,ulong seq)
{
	struct XX_INST *xx_inst;
	uchar tmp[200];

	if(g_inst==0 || g_count==0)
	{
		err_code=INST_ERR_UNINIT;
		return false;
	}

	xx_inst=&g_inst[seq];

	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}

	memset(xx_inst,0,sizeof(struct XX_INST));

	if(inst_system!=INST_SYSTEM_32 && inst_system!=INST_SYSTEM_64)
	{
		err_code=INST_ERR_ARGUMENT;
		return false;
	}

	xx_inst->xx_inst_code.inst_system=inst_system;

	memcpy(xx_inst->xx_inst_code.base,(uchar*)&mod_base,sizeof(ulong));
	
	memcpy(xx_inst->xx_inst_code.current_addr,(uchar*)&inst_addr,sizeof(ulong));
	
	xx_disasm(xx_inst,pdata);
	if(xx_inst->xx_inst_exist.next_op_flag==INST_NEXTOP_FINISH && \
		xx_inst->xx_inst_exist.finish_flag==INST_FLAG_EXIST)
	{
		err_code=INST_ERR_SUCCESS;
		return true;
	}
	else if(xx_inst->xx_inst_exist.next_op_flag!=INST_NEXTOP_FINISH && \
		xx_inst->xx_inst_exist.finish_flag==INST_FLAG_UNEXIST)
	{
		err_code=INST_ERR_UNDEFINE;
		return false;
	}
	else
	{
		err_code=INST_ERR_UNKNOW;
		return false;
	}
		err_code=INST_ERR_UNKNOW;
	return false;
}

  DLL_PUBLIC int xxdisasm2(uchar *pdata,int inst_system,uchar *pmod_base,uchar *pinst_addr,ulong seq)
{
	struct XX_INST *xx_inst;
	uchar tmp[200];

	if(g_inst==0 || g_count==0)
	{
		err_code=INST_ERR_UNINIT;
		return false;
	}

	xx_inst=&g_inst[seq];

	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}

	memset(xx_inst,0,sizeof(struct XX_INST));

	if(inst_system!=INST_SYSTEM_32 && inst_system!=INST_SYSTEM_64)
	{
		err_code=INST_ERR_ARGUMENT;
		return false;
	}
	xx_inst->xx_inst_code.inst_system=inst_system;

	memcpy(xx_inst->xx_inst_code.base,pmod_base,xx_inst->xx_inst_code.inst_system);
	
	memcpy(xx_inst->xx_inst_code.current_addr,pinst_addr,xx_inst->xx_inst_code.inst_system);
	
	xx_disasm(xx_inst,pdata);
	if(xx_inst->xx_inst_exist.next_op_flag==INST_NEXTOP_FINISH && \
		xx_inst->xx_inst_exist.finish_flag==INST_FLAG_EXIST)
	{
		err_code=INST_ERR_SUCCESS;
		return true;
	}
	else if(xx_inst->xx_inst_exist.next_op_flag!=INST_NEXTOP_FINISH && \
		xx_inst->xx_inst_exist.finish_flag==INST_FLAG_UNEXIST)
	{
		err_code=INST_ERR_UNDEFINE;
		return false;
	}
	else
	{
		err_code=INST_ERR_UNKNOW;
		return false;
	}
		err_code=INST_ERR_UNKNOW;
	return false;
}

  DLL_PUBLIC void xx_free()
{

	if(g_inst)
	{
		free(g_inst);
		g_inst=0;
	}
	xx_disasm_free();
	err_code=INST_ERR_SUCCESS;
	return ;
}




  DLL_PUBLIC int xx_inst_init(ulong inst_count)
{
	if(g_inst)
	{
		free(g_inst);
		g_inst=0;
	}
	g_inst=malloc(sizeof(struct XX_INST)*inst_count);
	if(g_inst)
	{
		memset(g_inst,0,sizeof(struct XX_INST)*inst_count);
		g_count=inst_count;
		err_code=INST_ERR_SUCCESS;
		return true;
	}
	else
	{
		g_inst=0;
		g_count=0;
		err_code=INST_ERR_MALLOC;
		return false;
	}
}


  DLL_PUBLIC void xx_inst_free()
{
	if(g_inst)
	{
		free(g_inst);
		g_inst=0;
	}
	err_code=INST_ERR_SUCCESS;
	return ;
}


  DLL_PUBLIC int xx_inst_init_count()
{
	if(g_count==0 || g_inst==0)
	{
		err_code=INST_ERR_UNINIT;
		return 0;
	}
	err_code=INST_ERR_SUCCESS;
	return g_count;
}



  DLL_PUBLIC int xx_get_inst_blength(ulong seq)
{
	struct XX_INST *xx_inst=0;

	if(g_count==0 || g_inst==0)
	{
		err_code=INST_ERR_UNINIT;
		return 0;
	}
	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	err_code=INST_ERR_SUCCESS;
	return xx_inst->xx_inst_code.disasm_length;
}


  DLL_PUBLIC int xx_get_inst_data(ulong seq,uchar *buf,int bufsize)
{
	struct XX_INST *xx_inst=0;
	if(buf==0 || bufsize==0)
	{
		err_code=INST_ERR_ARGUMENT;
		return false;
	}

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}
	if(xx_inst->xx_inst_code.disasm_length>bufsize)
	{
		err_code=INST_ERR_BUFSIZE;
		return 0;
	}
	memset(buf,0,bufsize);
	memcpy(buf,xx_inst->xx_inst_code.disasm,xx_inst->xx_inst_code.disasm_length);
	err_code=INST_ERR_SUCCESS;
	return true;
}


  DLL_PUBLIC int xx_get_inst_slength(ulong seq)
{
	struct XX_INST *xx_inst=0;

	if(g_count==0 || g_inst==0)
	{
		err_code=INST_ERR_ARGUMENT;
		return 0;
	}
	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	err_code=INST_ERR_SUCCESS;
	return strlen(xx_inst->xx_inst_mic.disasm);
}


  DLL_PUBLIC int xx_get_inst_mic(ulong seq,uchar *buf,int bufsize)
{
	struct XX_INST *xx_inst=0;
	if(buf==0 || bufsize==0)
	{
		err_code=INST_ERR_ARGUMENT;
		return false;
	}

	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return false;
	}
	if(strlen(xx_inst->xx_inst_mic.disasm)>bufsize)
	{
		err_code=INST_ERR_BUFSIZE;
		return 0;
	}
	memset(buf,0,bufsize);
	memcpy(buf,xx_inst->xx_inst_mic.disasm,strlen(xx_inst->xx_inst_mic.disasm));
	err_code=INST_ERR_SUCCESS;
	return true;
}



  DLL_PUBLIC int xx_get_inst_lbase(ulong seq,ulong *plbase)
{
	struct XX_INST *xx_inst=0;
	ulong lbase=0;
	ulong *pul=0;

	if(g_count==0 || g_inst==0)
	{
		err_code=INST_ERR_ARGUMENT;
		return 0;
	}
	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}

	pul=(ulong*)xx_inst->xx_inst_code.base;
	lbase=pul[0];
	*plbase=lbase;
	err_code=INST_ERR_SUCCESS;
	return 1;
}





  DLL_PUBLIC int xx_get_inst_hbase(ulong seq,ulong *phbase)
{
	struct XX_INST *xx_inst=0;
	ulong hbase=0;
	ulong *pul=0;

	if(g_count==0 || g_inst==0)
	{
		err_code=INST_ERR_ARGUMENT;
		return 0;
	}
	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}

	pul=(ulong*)xx_inst->xx_inst_code.base;
	hbase=pul[1];
	*phbase=hbase;
	err_code=INST_ERR_SUCCESS;
	return 1;
}


  DLL_PUBLIC int xx_get_inst_laddr(ulong seq,ulong *pladdr)
{
	struct XX_INST *xx_inst=0;
	ulong laddr=0;
	ulong *pul=0;

	if(g_count==0 || g_inst==0)
	{
		err_code=INST_ERR_ARGUMENT;
		return 0;
	}
	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}

	pul=(ulong*)xx_inst->xx_inst_code.current_addr;
	laddr=pul[0];
	*pladdr=laddr;
	err_code=INST_ERR_SUCCESS;
	return 1;
}

  DLL_PUBLIC int xx_get_inst_haddr(ulong seq,ulong *phaddr)
{
	struct XX_INST *xx_inst=0;
	ulong haddr=0;
	ulong *pul=0;

	if(g_count==0 || g_inst==0)
	{
		err_code=INST_ERR_ARGUMENT;
		return 0;
	}
	if(seq>=g_count)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}
	xx_inst=&g_inst[seq];
	if(xx_inst==0)
	{
		err_code=INST_ERR_SEQ;
		return 0;
	}

	pul=(ulong*)xx_inst->xx_inst_code.current_addr;
	haddr=pul[1];
	*phaddr=haddr;
	err_code=INST_ERR_SUCCESS;
	return 1;
}













