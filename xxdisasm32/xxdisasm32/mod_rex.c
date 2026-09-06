#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm32.h"

  

/////////////////////////////////////


int xx_rex_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int n;
	int iret=0;
	int rows;
	uchar fbyte[100];
	uchar sfirst_opcode[20];
	struct XX_TBL_ONE_OPCODE *tbl_one_opcode;

	//printf("===xx_rex_mod start===\n");

	memset(fbyte,0,sizeof(fbyte));	
 	
	if(xx_inst->xx_inst_exist.prefix_flag==INST_FLAG_NOTOP )
	{
		return -2;
	}

	if(xx_inst->xx_inst_code.inst_system!=INST_SYSTEM_64 )
	{
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ONE_OPCODE;
		return 0;
	}


	tbl_one_opcode=get_sql_struct(TBL_ONE_OPCODE);
	if(!tbl_one_opcode)
	{
		return -1;
	}

	rows=get_sql_rows_num(TBL_ONE_OPCODE);
	if(!rows)
	{
		return -1;
	}

	memcpy(fbyte,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.first_opcode));
	

	memset(sfirst_opcode,0,sizeof(sfirst_opcode));	
	nhex_str(fbyte,1,sfirst_opcode,"");
	n=seek_one_opcode_tbl(tbl_one_opcode,rows,sfirst_opcode, xx_inst->xx_inst_code.inst_system);
	if(n==-1)
	{
		return -1;
	}

	xx_inst->xx_inst_table.tbl_one_opcode=(void*)&tbl_one_opcode[n];

	if(memcmp(xx_inst->xx_inst_table.tbl_one_opcode->opcode_define_flag,"0",1)==0)
	{
		xx_inst->xx_inst_exist.one_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.opcode_flag=INST_FLAG_NOTOP;
		xx_inst->xx_inst_exist.finish_flag=INST_ERROR_UNDEFINE;
		return -1;
	}

	if(memcmp(xx_inst->xx_inst_table.tbl_one_opcode->rex_flag,"1",1)!=0)
	{
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ONE_OPCODE;
		return 0;
	}


	xx_inst->xx_inst_exist.rex_flag=INST_FLAG_EXIST;

	if((*fbyte & 0x08)==0x08)
	{
		xx_inst->xx_inst_exist.rexw_flag=INST_FLAG_EXIST;
	}
	else
	{
		xx_inst->xx_inst_exist.rexw_flag=INST_FLAG_UNEXIST;
	}

	if((*fbyte & 0x04)==0x04)
	{
		xx_inst->xx_inst_exist.rexr_flag=INST_FLAG_EXIST;
	}
	else
	{
		xx_inst->xx_inst_exist.rexr_flag=INST_FLAG_UNEXIST;
	}

	if((*fbyte & 0x02)==0x02)
	{
		xx_inst->xx_inst_exist.rexx_flag=INST_FLAG_EXIST;
	}
	else
	{
		xx_inst->xx_inst_exist.rexx_flag=INST_FLAG_UNEXIST;
	}

	if((*fbyte & 0x01)==0x01)
	{
		xx_inst->xx_inst_exist.rexb_flag=INST_FLAG_EXIST;
	}
	else
	{
		xx_inst->xx_inst_exist.rexb_flag=INST_FLAG_UNEXIST;
	}

	memcpy(xx_inst->xx_inst_code.rex,fbyte,1);
	xx_inst->xx_inst_code.rex_len=1;;

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,fbyte,1);

	xx_inst->xx_inst_code.disasm_length++;

	xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ONE_OPCODE;

	//printf("===xx_rex_mod end===\n");

	return 0;
}



