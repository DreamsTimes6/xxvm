
#include <stdio.h>
#include <string.h>

#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm64.h"


extern int reg_def(uchar* src_def,uchar* des_def,int rexflag,int dsize);



extern int xx_get_reg_mic(unsigned char *reg_def,char* reg_mic);


int item_type_reg(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	uchar des_tmp[200];
	int length=0;
	int p2;
	int pi;
	char** pc;
	char** p6;
	char sub_reg[100];
	int index;

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;

	if (xx_inst->xx_inst_code.inst_system == INST_SYSTEM_64)
	{
		if (memcmp(xx_inst->xx_inst_code.sups, SUPS_D64, strlen(SUPS_D64)) == 0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size == INST_SYSTEM_32)
		{
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = INST_SYSTEM_64;
		}
	}

	memset(des_tmp,0,sizeof(des_tmp));
	length=reg_def(xx_inst->xx_inst_table.tbl_item_type->reg_def,des_tmp,\
			xx_inst->xx_inst_exist.rex_flag,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	if(!length)
	{
		return -1;
	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			des_tmp,length);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;
	
	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));


	return 0;
}






