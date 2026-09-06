#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm64.h"




int xx_prefix_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int iret;
	int row_num=0;
	int n=0;
	int b_flag=0;
	uchar fbyte[100];
	uchar tmp[100];
	struct XX_TBL_PREFIX  *tbl_prefix;



	tbl_prefix=get_sql_struct(TBL_PREFIX);
	if(!tbl_prefix)
	{
		return -1;
	}
	row_num=get_sql_rows_num(TBL_PREFIX);
	if(!row_num)
	{
		return -1;
	}

	b_flag=0;
	while(1)
	{
		memset(tmp,0,sizeof(tmp));
		memset(fbyte,0,sizeof(fbyte));	

		memcpy(fbyte,codes+xx_inst->xx_inst_code.disasm_length,1);

		iret=nhex_str(fbyte,1,tmp,"");
		if(iret)
		{
			return -1;
		}

		for(n=0;n<row_num;n++)
		{
			if(strcmp(tmp,tbl_prefix[n].prefix)==0)
			{
				xx_inst->xx_inst_table.tbl_prefix=(void*)&tbl_prefix[n];

				memcpy(xx_inst->xx_inst_code.prefix+xx_inst->xx_inst_code.prefix_len,fbyte,1);
				xx_inst->xx_inst_code.prefix_len++;
				xx_inst->xx_inst_code.nprefix++;

				memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,fbyte,1);
				xx_inst->xx_inst_code.disasm_length++;
				break;
			}
		}
		if(n>=row_num)
		{
			break;
		}
	}
	if(xx_inst->xx_inst_code.nprefix==0)
	{
		xx_inst->xx_inst_exist.prefix_flag=INST_FLAG_UNEXIST;
	}
	else
	{
		xx_inst->xx_inst_exist.prefix_flag=INST_FLAG_EXIST;
	}

 	if(xx_inst->xx_inst_exist.prefix_flag==INST_FLAG_EXIST)
	{
		if(*xx_inst->xx_inst_code.prefix==0xf0)
		{
			memcpy(xx_inst->xx_inst_mic.prefix,PREFIX_LOCK,strlen(PREFIX_LOCK));
		}
		else if(*xx_inst->xx_inst_code.prefix==0xf2)
		{
			memcpy(xx_inst->xx_inst_mic.prefix,PREFIX_REPNE,strlen(PREFIX_REPNE));
		}
		else if(*xx_inst->xx_inst_code.prefix==0xf3)
		{
			memcpy(xx_inst->xx_inst_mic.prefix,PREFIX_REPE,strlen(PREFIX_REPE));
		}
	}

	xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_REX;
	return 0;
}




int subprefix(int nprefix,char *prefix,int sub_prefix)
{
	int n=0;
	int var=0;
	for(n=0;n<nprefix;n++)
	{
		var=prefix[n];
		if(var==sub_prefix)
		{
			return 1;
		}
	}
	return 0;
}
