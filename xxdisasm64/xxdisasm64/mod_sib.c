#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm64.h"



/////////////////////////////////////



int xx_sib_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int tmp=0;
	uchar fbyte[100];
	
	struct XX_TBL_SIB *tbl_sib;

	memset(fbyte, 0, sizeof(fbyte));
	if(xx_inst->xx_inst_exist.modrm_flag==INST_FLAG_NOTOP &&
		xx_inst->xx_inst_exist.sib_flag==INST_FLAG_UNEXIST) 
	{
		return -2;
	}

	
	memcpy(fbyte,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.sib));
        	
	memcpy(xx_inst->xx_inst_code.sib,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));

	tbl_sib=get_sql_struct(TBL_SIB);
	if(!tbl_sib)
	{
		return -1;
	}
	xx_inst->xx_inst_table.tbl_sib=(void*)&tbl_sib[*fbyte];

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,xx_inst->xx_inst_code.sib,1);
	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
					    sizeof(xx_inst->xx_inst_code.sib);

	xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ITEMS;

	return 0;
}









