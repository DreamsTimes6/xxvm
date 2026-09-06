#include <string.h>
#include <stdlib.h>
#include "xx_comm64.h"
#include "xx_inst.h"
#include "xx_err.h"



////////////////////////////////////////
extern struct XX_INST *g_inst;
extern int g_count;
extern ulong err_code;
//////////////////////////////////////////

  DLL_PUBLIC int xx_get_op_type(ulong seq,ulong *pop_type)
{
	struct XX_INST *xx_inst=0;
	ulong var=0;
	char tmp[200];

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
	
	memset(tmp,0,sizeof(tmp));
	nstr_hex(xx_inst->xx_inst_code.opcode_type,strlen(xx_inst->xx_inst_code.opcode_type),tmp);	
	
	*pop_type=0;

	var=0xff000000 & (tmp[0]<<24);
	*pop_type=*pop_type | var;

	var=0x00ff0000 & (tmp[1]<<16);
	*pop_type=*pop_type | var;

	var=0x0000ff00 & (tmp[2]<<8);
	*pop_type=*pop_type | var;

	err_code=INST_ERR_SUCCESS;
	return 1;
}



  DLL_PUBLIC int xx_get_op_mic(ulong seq,uchar *buf,int bufsize)
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
	if(strlen(xx_inst->xx_inst_mic.opcode_mic)>bufsize)
	{
		err_code=INST_ERR_BUFSIZE;
		return 0;
	}
	memset(buf,0,bufsize);
	memcpy(buf,xx_inst->xx_inst_mic.opcode_mic,strlen(xx_inst->xx_inst_mic.opcode_mic));
	err_code=INST_ERR_SUCCESS;
	return true;
}







