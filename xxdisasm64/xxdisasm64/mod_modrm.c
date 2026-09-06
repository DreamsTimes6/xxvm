#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm64.h"








int xx_modrm_mod(struct XX_INST* xx_inst,uchar* codes)
{
	uchar fbyte[100];
	int offset=0;
	struct XX_TBL_MODRM *tbl_modrm;

	memset(fbyte,0,sizeof(fbyte));	

	if(xx_inst->xx_inst_exist.opcode_flag==INST_FLAG_NOTOP &&
			xx_inst->xx_inst_exist.modrm_flag==INST_FLAG_UNEXIST)
	{
		return -2;
	}
	
	memcpy(fbyte,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));
        	
	memcpy(xx_inst->xx_inst_code.modrm,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));
	
	tbl_modrm=get_sql_struct(TBL_MODRM);
	if(!tbl_modrm)
	{
		return -1;
	}
	xx_inst->xx_inst_table.tbl_modrm=(void*)&tbl_modrm[*xx_inst->xx_inst_code.modrm];

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,fbyte,1);
	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
					    sizeof(xx_inst->xx_inst_code.modrm);


	if(memcmp(xx_inst->xx_inst_table.tbl_modrm->sib_exist_flag,"1",1)==0)
	{
		xx_inst->xx_inst_exist.sib_flag=INST_FLAG_EXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_SIB;
		return 0;
	}
	else
	{
		xx_inst->xx_inst_exist.sib_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ITEMS;
		return 0;
	}
	return -1;
}






