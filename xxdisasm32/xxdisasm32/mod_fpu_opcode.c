#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm32.h"




#define FPU_MODRM_FLAG1   "1"
#define FPU_MODRM_FLAG2   "2"

struct FPU_ITEM_TYPE
{
	uchar item_type[60];
	uchar item_ltype[60];
};



extern int reg_def(uchar* src_def,uchar* des_def,int rexflag,int dsize);
extern int xx_get_reg_mic(unsigned char *reg_def,char* reg_mic);

int fpu_modrm_flag2(struct XX_INST* xx_inst,uchar* codes);
int fpu_modrm_flag1(struct XX_INST* xx_inst,uchar* codes);

///////////////////////////////////////////////////////////////////////////
int xx_fpu_opcode_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int n=0;
	int iret=0;
	int rows=0;
	uchar bbyte[10];
	uchar bmodrm[10];
	uchar sbyte[10];
	uchar smodrm[10];
	uchar sfirst_opcode[20];
	struct XX_TBL_FPU_OPCODE *tbl_fpu_opcode;

	memset(bbyte,0,sizeof(bbyte));	
 	
	if(xx_inst->xx_inst_exist.prefix_flag==INST_FLAG_NOTOP )
	{
		return -2;
	}

	tbl_fpu_opcode=get_sql_struct(TBL_FPU_OPCODE);
	if(!tbl_fpu_opcode)
	{
		return -1;
	}

	rows=get_sql_rows_num(TBL_FPU_OPCODE);
	if(!rows)
	{
		return -1;
	}

	memcpy(bbyte,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.first_opcode));
	memcpy(bmodrm,codes+xx_inst->xx_inst_code.disasm_length+sizeof(xx_inst->xx_inst_code.first_opcode),\
			sizeof(xx_inst->xx_inst_code.modrm));
	

	memset(sbyte,0,sizeof(sbyte));	
	nhex_str(bbyte,1,sbyte,"");
	memset(smodrm,0,sizeof(smodrm));	
	nhex_str(bmodrm,1,smodrm,"");
	n=seek_fpu_opcode_tbl(tbl_fpu_opcode,rows,sbyte,smodrm);
	if(n==-1)
	{
		return -1;
	}

	xx_inst->xx_inst_table.tbl_fpu_opcode=(void*)&tbl_fpu_opcode[n];

	memcpy(xx_inst->xx_inst_code.first_opcode,bbyte,\
			sizeof(xx_inst->xx_inst_code.first_opcode));

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,bbyte,\
			sizeof(xx_inst->xx_inst_code.first_opcode));
	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+sizeof(xx_inst->xx_inst_code.first_opcode);


	memcpy(xx_inst->xx_inst_code.opcode_type,xx_inst->xx_inst_table.tbl_fpu_opcode->opcode_type,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->opcode_type));

	
	memcpy(xx_inst->xx_inst_mic.opcode_mic,\
			xx_inst->xx_inst_table.tbl_fpu_opcode->opcode_mic_1,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->opcode_mic_1));

	xx_inst->xx_inst_exist.one_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.two_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.three_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.ext_one_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.ext_two_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.ext_three_opcode_flag=INST_FLAG_UNEXIST;

	xx_inst->xx_inst_exist.fpu_opcode_flag=INST_FLAG_EXIST;

	xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_EXIST;

	if(strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->modrm_flag)!=1)
	{
		return -1;
	}



	if(memcmp(xx_inst->xx_inst_table.tbl_fpu_opcode->modrm_exist_flag,"1",1)==0 && \
	        memcmp(xx_inst->xx_inst_table.tbl_fpu_opcode->modrm_flag,FPU_MODRM_FLAG1,strlen(FPU_MODRM_FLAG1))==0)
	{
		xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_EXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_MODRM;
		return 0;
	}
	else if(memcmp(xx_inst->xx_inst_table.tbl_fpu_opcode->modrm_exist_flag,"1",1)==0 && \
	        memcmp(xx_inst->xx_inst_table.tbl_fpu_opcode->modrm_flag,FPU_MODRM_FLAG2,strlen(FPU_MODRM_FLAG2))==0)
	{
		iret=fpu_modrm_flag2(xx_inst,codes);
		if(iret==-1)
		{
			return -1;
		}
		xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_EXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_FINISH;
		return 0;
	}
	else
	{
		return -1;
	}

	return 0;


}



int fpu_modrm_flag1(struct XX_INST* xx_inst,uchar* codes)
{
	int n=0;
	int iret=0;
	uchar tmp[200];
	struct FPU_ITEM_TYPE item_type[5];
	int nitems=0;
	int itype=0;
	int iltype=0;

	memset(item_type,0,sizeof(item_type));

	memcmp(item_type[0].item_type,xx_inst->xx_inst_table.tbl_fpu_opcode->item1_type,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item1_type));
	memcmp(item_type[0].item_ltype,xx_inst->xx_inst_table.tbl_fpu_opcode->item1_ltype,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item1_ltype));

	memcmp(item_type[1].item_type,xx_inst->xx_inst_table.tbl_fpu_opcode->item2_type,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item2_type));
	memcmp(item_type[1].item_ltype,xx_inst->xx_inst_table.tbl_fpu_opcode->item2_ltype,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item2_ltype));

	memcmp(item_type[2].item_type,xx_inst->xx_inst_table.tbl_fpu_opcode->item3_type,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item3_type));
	memcmp(item_type[2].item_ltype,xx_inst->xx_inst_table.tbl_fpu_opcode->item3_ltype,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item3_ltype));

	memcmp(item_type[3].item_type,xx_inst->xx_inst_table.tbl_fpu_opcode->item4_type,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item4_type));
	memcmp(item_type[3].item_ltype,xx_inst->xx_inst_table.tbl_fpu_opcode->item4_ltype,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item4_ltype));

	nitems=atoi(xx_inst->xx_inst_table.tbl_fpu_opcode->op_items);

	xx_inst->xx_inst_items.nitem=nitems;

	for(n=0;n<nitems;n++)
	{

		itype=atoi(item_type[n].item_type);
		iltype=atoi(item_type[n].item_ltype);
		if(itype==0 )
		{
			return -1;
		}

		xx_inst->xx_inst_items.xx_inst_items_flag[n].item_type=itype;

		xx_inst->xx_inst_items.xx_inst_items_var[n].item_addr_size=xx_inst->xx_inst_code.inst_system;

		xx_inst->xx_inst_items.xx_inst_items_var[n].item_data_size=iltype;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[n].item_const,\
				codes+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[n].item_data_size);

		xx_inst->xx_inst_items.xx_inst_items_var[n].const_size=xx_inst->xx_inst_items.xx_inst_items_var[n].item_data_size;

	}

	return 0;
}


int fpu_modrm_flag2(struct XX_INST* xx_inst,uchar* codes)
{
	int n=0;
	int iret=0;
	uchar tmp[200];
	struct FPU_ITEM_TYPE item_type[5];
	int nitems=0;
	int itype=0;
	int iltype=0;
	int length=0;

	memcpy(xx_inst->xx_inst_code.modrm,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_code.modrm,\
			sizeof(xx_inst->xx_inst_code.modrm));
	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+sizeof(xx_inst->xx_inst_code.modrm);

	memset(item_type,0,sizeof(item_type));

	memcpy(item_type[0].item_type,xx_inst->xx_inst_table.tbl_fpu_opcode->item1_type,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item1_type));
	memcpy(item_type[0].item_ltype,xx_inst->xx_inst_table.tbl_fpu_opcode->item1_ltype,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item1_ltype));

	memcpy(item_type[1].item_type,xx_inst->xx_inst_table.tbl_fpu_opcode->item2_type,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item2_type));
	memcpy(item_type[1].item_ltype,xx_inst->xx_inst_table.tbl_fpu_opcode->item2_ltype,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item2_ltype));

	memcpy(item_type[2].item_type,xx_inst->xx_inst_table.tbl_fpu_opcode->item3_type,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item3_type));
	memcpy(item_type[2].item_ltype,xx_inst->xx_inst_table.tbl_fpu_opcode->item3_ltype,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item3_ltype));

	memcpy(item_type[3].item_type,xx_inst->xx_inst_table.tbl_fpu_opcode->item4_type,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item4_type));
	memcpy(item_type[3].item_ltype,xx_inst->xx_inst_table.tbl_fpu_opcode->item4_ltype,\
			strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item4_ltype));

	nitems=atoi(xx_inst->xx_inst_table.tbl_fpu_opcode->op_items);

	xx_inst->xx_inst_items.nitem=nitems;

	for(n=0;n<nitems;n++)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[n].item_type=INST_ITEM_TYPE_REG;

		xx_inst->xx_inst_items.xx_inst_items_var[n].item_addr_size=xx_inst->xx_inst_code.inst_system;

		xx_inst->xx_inst_items.xx_inst_items_var[n].item_data_size=0x0a;

		memset(tmp,0,sizeof(tmp));
		length=reg_def(item_type[n].item_type,tmp,\
				xx_inst->xx_inst_exist.rex_flag, \
				xx_inst->xx_inst_items.xx_inst_items_var[n].item_data_size);
		if(length==0)
		{
			return -1;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[n].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[n].reg_def_len=length;
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[n].item_base_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[n].base_def_len=length;

		memset(tmp,0,sizeof(tmp));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[n].item_reg_def,tmp);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[n].item_self_mic,\
				tmp,strlen(tmp));

	}

	return 0;
}















