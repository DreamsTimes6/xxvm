#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm32.h"





extern int reg_def(uchar* src_def,uchar* des_def, int rexflag ,int dsize);
extern int xx_get_reg_mic(unsigned char *reg_def,char* reg_mic);


int type_sib_displacement(int nitem,struct XX_INST *xx_inst,uchar *code);
int type_sib(int nitem,struct XX_INST *xx_inst,uchar *code);
int type_sib_mdsp(int nitem,struct XX_INST *xx_inst,uchar *code);
int  get_modrm_reg(struct XX_INST *xx_inst,int mod_flag,int reg_size,char *buf,int bufsize);
int  get_sib_reg(struct XX_INST *xx_inst,int sib_flag,int reg_size,char *buf,int bufsize);

int item_type_A(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int tmp_offset=0;
	uchar tmp[200];
	uchar tmp1[200];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);


	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_EXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
	

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_IADDR;

	xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_seg_size=2;
		

	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
			code+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);


	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);


	memcpy(xx_inst->xx_inst_code.immediate,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);


	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
					    xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;

	
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_segment,\
			code+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_seg_size);

	
	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_segment,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_seg_size);


	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
					    xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_seg_size;
	
	
	memset(tmp1,0,sizeof(tmp1));
	byte_opposite(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_segment,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_seg_size,\
			tmp1);

	memset(tmp,0,sizeof(tmp));
	nhex_str(tmp1,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_seg_size,tmp,"");

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
			tmp,strlen(tmp));

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_segment,\
			tmp,strlen(tmp));
	tmp_offset=tmp_offset+strlen(tmp);

	strcat(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,":");
	tmp_offset=tmp_offset+strlen(":");

	memset(tmp1,0,sizeof(tmp1));
	byte_opposite(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
			tmp1);

	memset(tmp,0,sizeof(tmp));
	nhex_str(tmp1,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,tmp,"");

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
			tmp,strlen(tmp));

	return 0;
}


int item_type_B(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	return 0;
}


int item_type_C(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int iret=0;
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];
	uchar tmp1[200];
	char sub_reg[100];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	
	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_REG;

	
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_REG)
	{
	
		return -1;
	}

	memset(tmp,0,sizeof(tmp));
	
	length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_cr,tmp,\
		xx_inst->xx_inst_exist.rex_flag, \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	if(length==0)
	{
		
		return -1;
	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

	
	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));
	
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_C;

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;
	return 0;
}


int item_type_D(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	
	int iret=0;
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];
	uchar tmp1[200];
	
	char sub_reg[100];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_REG)
	{
		return -1;
	}
	
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_REG;

	
	memset(tmp,0,sizeof(tmp));
	
	length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_dr,tmp,\
		xx_inst->xx_inst_exist.rex_flag, \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	if(length==0)
	{
		return -1;
	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;


	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_D;

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;
	return 0;
}



int item_type_E(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int iret=0;
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];
	uchar tmp1[200];
	char sub_reg[100];


	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	
	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_RM)
	{
		return -1;
	}

	if (xx_inst->xx_inst_code.inst_system == INST_SYSTEM_64)
	{
		if (strcmp(xx_inst->xx_inst_code.sups, "d64") == 0) 
		{
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = INST_SYSTEM_64;
		}
	}
	
	if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==1)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_REG;

		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
				sub_reg,strlen(sub_reg));
		
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==2)
	{
		tmp_offset=0;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREG;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
		tmp_offset=tmp_offset+LENGTH_S_BRACKET;

		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));

		tmp_offset=tmp_offset+strlen(sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);
	
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==3)
	{
		tmp_offset=0;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGDIS;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
		tmp_offset=tmp_offset+LENGTH_S_BRACKET;

		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));
		tmp_offset=tmp_offset+strlen(sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				xx_inst->xx_inst_table.tbl_modrm->rm_sign,\
				strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
		tmp_offset=tmp_offset+strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);

		memset(tmp1,0,sizeof(tmp1));
		byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),\
				tmp1);

		memset(tmp,0,sizeof(tmp));
		nhex_str(tmp1,atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),tmp,"");

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
		tmp_offset=tmp_offset+strlen(tmp);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

	
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_EXIST;

	
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);

	
		memcpy(xx_inst->xx_inst_code.displacement,\
			code+xx_inst->xx_inst_code.disasm_length,\
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));

		
		memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			code+xx_inst->xx_inst_code.disasm_length,\
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));

		
		xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);

		
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==4)
	{
		type_sib(nitem,xx_inst,code);
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==5)
	{
		type_sib_displacement(nitem,xx_inst,code);
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==6)
	{
		if (xx_inst->xx_inst_code.inst_system == INST_SYSTEM_64)
		{
			type_sib_mdsp(nitem, xx_inst, code);
		}
		else
		{
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MDIS;


			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_EXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
			tmp_offset=tmp_offset+LENGTH_S_BRACKET;

			memset(tmp1,0,sizeof(tmp1));
			byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp1);

			memset(tmp,0,sizeof(tmp));
			nhex_str(tmp1,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,"");

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
			tmp_offset=tmp_offset+strlen(tmp);

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);


			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;


			memcpy(xx_inst->xx_inst_code.immediate,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);


			memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);


			xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;

		}
	}
	else 
	{
		return -1;
	}

	return 0;
}


int item_type_F(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	return 0;
}


int item_type_G(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int iret=0;
	int length=0;
	uchar tmp[200];
	char sub_reg[100];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	
	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_REG)
	{
		return -1;
	}

	memset(tmp,0,sizeof(tmp));
	length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_REG,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size,\
			tmp,sizeof(tmp));
        if(length==0)
	{
		return -1;
	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));

	return 0;
}

int item_type_H(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	return 0;
}

int item_type_I(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	uchar tmp[200];
	uchar tmp1[200];
	int int1 = 0;
	int int2 = 0xffffffff;

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	/*操作项确定，内存地址*/
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag = INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag = INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag = INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag = INST_FLAG_EXIST;

	/*特殊字节码的操作项大小计算*/
	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp, xx_inst->xx_inst_code.first_opcode, sizeof(xx_inst->xx_inst_code.first_opcode));
	if (xx_inst->xx_inst_exist.rexw_flag == INST_FLAG_EXIST && \
		*tmp >= 0xb8 && *tmp <= 0xbf)
	{
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = INST_SYSTEM_64;
	}
	/*给项常数赋值*/
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const, \
		code + xx_inst->xx_inst_code.disasm_length, \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size = xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size;

	/*给指令字节码赋值*/
	memcpy(xx_inst->xx_inst_code.disasm + xx_inst->xx_inst_code.disasm_length, \
		code + xx_inst->xx_inst_code.disasm_length, \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	/*指令长度增加*/
	xx_inst->xx_inst_code.disasm_length = xx_inst->xx_inst_code.disasm_length + \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size;

	/*检查上标*/
	if (xx_inst->xx_inst_code.inst_system == INST_SYSTEM_64)
	{
		if (memcmp(xx_inst->xx_inst_code.sups, SUPS_D64, strlen(SUPS_D64)) == 0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size == INST_SYSTEM_32)
		{
			if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const[3] >= 0x80)
			{
				memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const + INST_SYSTEM_32, \
					&int2, sizeof(int2));
			}
			else
			{
				memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const + INST_SYSTEM_32, \
					&int1, sizeof(int1));
			}
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = INST_SYSTEM_64;
		}
	}

	memcpy(xx_inst->xx_inst_code.immediate, \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const, \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	/*64位指令如果操作数大小为32位，item_const符号扩展到64位*/
	if (xx_inst->xx_inst_exist.rexw_flag == INST_FLAG_EXIST  &&  \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size == INST_SYSTEM_32)
	{
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = INST_SYSTEM_64;
		if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const[3] >= 0x80)
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const + INST_SYSTEM_32, \
				&int2, sizeof(int2));
		}
		else
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const + INST_SYSTEM_32, \
				&int1, sizeof(int1));
		}
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = INST_SYSTEM_64;
	}


	memset(tmp1, 0, sizeof(tmp1));
	byte_opposite(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const, \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size, \
		tmp1);

	memset(tmp, 0, sizeof(tmp));
	nhex_str(tmp1, xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size, tmp, "");


	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic, \
		tmp, strlen(tmp));


	return 0;
}

int item_type_J(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	uchar tmp[200];
	uchar tmp1[200];
	ulong32 addr32=0;	
	ulong64 addr64=0;	
	ulong32   dsp32=0;
	ulong64   dsp64=0;
	

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_EXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		
		
	memcpy(xx_inst->xx_inst_code.displacement,\
			code+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	
	
	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			code+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	
	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
					    xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size;
	
	
	if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size==INST_ITEM_SIZE_8)
	{
		dsp32=*(ulong*)xx_inst->xx_inst_code.displacement;
		addr64=*(ulong64*)xx_inst->xx_inst_code.current_addr;
		if(dsp32>0x00000080)
		{
			dsp32=(0x00000100-dsp32)  ;
			addr64=addr64-dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		else
		{
			addr64=addr64+dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				&addr64,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size==INST_ITEM_SIZE_16)
	{
		dsp32=*(ulong*)xx_inst->xx_inst_code.displacement;
		addr64=*(ulong64*)xx_inst->xx_inst_code.current_addr;
		if(dsp32>0x00008000)
		{
			dsp32=(0x00010000-dsp32)  ;
			addr64=addr64-dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		else
		{
			addr64=addr64+dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				&addr64,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size==INST_ITEM_SIZE_32)
	{
		dsp32=*(ulong*)xx_inst->xx_inst_code.displacement;
		addr64=*(ulong64*)xx_inst->xx_inst_code.current_addr;
		if(dsp32>0x80000000)
		{
			dsp32=0xffffffff-dsp32+1;
			addr64=addr64-dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		else
		{
			addr64=addr64+dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				&addr64,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size==INST_ITEM_SIZE_64)
	{
		dsp64=*(ulong64*)xx_inst->xx_inst_code.displacement;
		addr64=*(ulong64*)xx_inst->xx_inst_code.current_addr;
		if(dsp64>0x8000000000000000)
		{
			dsp64=0xffffffffffffffff-dsp64+1;
			addr64=addr64-dsp64+xx_inst->xx_inst_code.disasm_length;
		}
		else
		{
			addr64=addr64+dsp64+xx_inst->xx_inst_code.disasm_length;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				&addr64,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	}
	else
	{
	}

	
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;

	memset(tmp,0,sizeof(tmp));
	byte_opposite(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,tmp);
	
	memset(tmp1,0,sizeof(tmp1));
	nhex_str(tmp,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,tmp1,"");

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			tmp1,strlen(tmp1));

	return 0;
}

int item_type_L(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	return 0;
}

int item_type_M(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];	
	uchar tmp1[200];	
	char sub_reg[100];


	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	
	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		
	}
	
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_RM_M)
	{
	}


	if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==1)
	{
		return -1;
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==2)
	{
		tmp_offset=0;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREG;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
		tmp_offset=tmp_offset+LENGTH_S_BRACKET;

		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));
		tmp_offset=tmp_offset+strlen(sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

		
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==3)
	{
		tmp_offset=0;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGDIS;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
		tmp_offset=tmp_offset+LENGTH_S_BRACKET;

		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));
		tmp_offset=tmp_offset+strlen(sub_reg);

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				xx_inst->xx_inst_table.tbl_modrm->rm_sign,\
				strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
		tmp_offset=tmp_offset+strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);
		memset(tmp1,0,sizeof(tmp1));
		byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),\
				tmp1);

		memset(tmp,0,sizeof(tmp));
		nhex_str(tmp1,atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),tmp,"");

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
		tmp_offset=tmp_offset+strlen(tmp);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);
	
		
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_EXIST;

	
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);

		
		memcpy(xx_inst->xx_inst_code.displacement,\
			code+xx_inst->xx_inst_code.disasm_length,\
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));


		
		memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			code+xx_inst->xx_inst_code.disasm_length,\
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));

		
		xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);
		
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==4)
	{
		type_sib(nitem,xx_inst,code);
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==5)
	{
		type_sib_displacement(nitem,xx_inst,code);
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==6)
	{
		if (xx_inst->xx_inst_code.inst_system == INST_SYSTEM_64)
		{
			type_sib_mdsp(nitem, xx_inst, code);
		}
		else
		{
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MDIS;

			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_EXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
			tmp_offset=tmp_offset+LENGTH_S_BRACKET;

			memset(tmp1,0,sizeof(tmp1));
			byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp1);


			memset(tmp,0,sizeof(tmp));
			nhex_str(tmp1,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,"");

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
			tmp_offset=tmp_offset+strlen(tmp);

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);


			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;


			memcpy(xx_inst->xx_inst_code.immediate,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);


			memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);


			xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;
		}

	}
	else 
	{
		return -1;
	}
	return 0;
}

int item_type_N(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	
	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
	}
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_RM)
	{
	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			xx_inst->xx_inst_table.tbl_modrm->rm_def_mmx,\
			strlen(xx_inst->xx_inst_table.tbl_modrm->rm_def_mmx));
	return 0;
}

int item_type_O(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int tmp_offset=0;
	uchar tmp[200];
	uchar tmp1[200];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
			code+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;

	memcpy(xx_inst->xx_inst_code.immediate,\
			code+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);


	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
	tmp_offset=tmp_offset+LENGTH_S_BRACKET;

	memset(tmp1,0,sizeof(tmp1));
	byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
			tmp1);

	memset(tmp,0,sizeof(tmp));
	nhex_str(tmp1,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,tmp,"");

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
			strlen(tmp));
	tmp_offset=tmp_offset+strlen(tmp);

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			code+xx_inst->xx_inst_code.disasm_length,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);

	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
					    xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;
	return 0;
}

int item_type_P(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int length=0;
	uchar tmp[200];
	char sub_reg[100];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_REG_MMX)
	{
		return -1;
	}


	memset(tmp,0,sizeof(tmp));
	length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_REG_MMX,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size,\
			tmp,sizeof(tmp));
        if(length==0)
	{
		return -1;
	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));
	
	
	return 0;
}

int item_type_Q(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int iret=0;
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];
	uchar tmp1[200];
	char sub_reg[100];


	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_RM)
	{
		return -1;
	}
	
	if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==1)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_REG;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_MMX;

		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM_REG_MMX,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size,\
			tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;
		
		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==2)
	{
		tmp_offset=0;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREG;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
		tmp_offset=tmp_offset+LENGTH_S_BRACKET;


		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));

		tmp_offset=tmp_offset+strlen(sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==3)
	{
		tmp_offset=0;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGDIS;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
		tmp_offset=tmp_offset+LENGTH_S_BRACKET;

		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));
		tmp_offset=tmp_offset+strlen(sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				xx_inst->xx_inst_table.tbl_modrm->rm_sign,\
				strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
		tmp_offset=tmp_offset+strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);
		memset(tmp1,0,sizeof(tmp1));
		byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),\
				tmp1);

		memset(tmp,0,sizeof(tmp));
		nhex_str(tmp1,atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),tmp,"");

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
		tmp_offset=tmp_offset+strlen(tmp);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_EXIST;


		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);

		memcpy(xx_inst->xx_inst_code.displacement,\
				code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));

		memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			code+xx_inst->xx_inst_code.disasm_length,\
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));

		xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);
		
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==4)
	{
		type_sib(nitem,xx_inst,code);
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==5)
	{
		type_sib_displacement(nitem,xx_inst,code);
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==6)
	{
		if (xx_inst->xx_inst_code.inst_system == INST_SYSTEM_64)
		{
			type_sib_mdsp(nitem, xx_inst, code);
		}
		else
		{
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MDIS;

			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_EXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
			tmp_offset=tmp_offset+LENGTH_S_BRACKET;

			memset(tmp1,0,sizeof(tmp1));
			byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp1);


			memset(tmp,0,sizeof(tmp));
			nhex_str(tmp1,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,"");

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
			tmp_offset=tmp_offset+strlen(tmp);

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;

			memcpy(xx_inst->xx_inst_code.immediate,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);

			memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);

			xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;

		}
	}
	else 
	{
		return -1;
	}
	return 0;
}

int item_type_R(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int iret=0;
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];
	uchar tmp1[200];
	char sub_reg[100];


	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_RM)
	{
		return -1;
	}

	if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)!=1)
	{
		return -1;
	}
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;


	memset(tmp,0,sizeof(tmp));
	length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size,\
			tmp,sizeof(tmp));
	if(length==0)
	{
		return -1;
	}

	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;

	return 0;
}

int item_type_S(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int iret=0;
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];
	uchar tmp1[200];
	char sub_reg[100];


	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_REG)
	{
		return -1;
	}
	memset(tmp,0,sizeof(tmp));
	length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_seg,tmp,\
		xx_inst->xx_inst_exist.rex_flag, \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	if(length==0)
	{
		return -1;
	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_S;

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;
	return 0;
}

int item_type_U(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int iret=0;
	int length=0;
	char tmp[200];
	char sub_reg[200];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_RM)
	{
		return -1;
	}

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_XMM;

	memset(tmp,0,sizeof(tmp));
	length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_xmm,tmp,\
		xx_inst->xx_inst_exist.rex_flag, \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	if(length==0)
	{
		return 0;
	}

	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);

	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));

	return 0;
}

int item_type_V(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int iret=0;
	int length=0;
	char tmp[200];
	char sub_reg[200];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_REG)
	{
		return -1;
	}

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_XMM;

	memset(tmp,0,sizeof(tmp));
	length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_xmm,tmp,\
		xx_inst->xx_inst_exist.rex_flag, \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);
	if(length==0)
	{
		return 0;
	}

	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);

	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));

	return 0;
}

int item_type_W(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int iret=0;
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];
	uchar tmp1[200];
	char sub_reg[100];


	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	if(strstr(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,\
				xx_inst->xx_inst_table.tbl_item_type->mod_range))
	{
		return -1;
	}
	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select!=INST_ITEM_MODRM_RM)
	{
		return -1;
	}
	
	if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==1)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_REG;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_XMM;

		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM_REG_XMM,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size,\
			tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;
		
		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			sub_reg,strlen(sub_reg));

	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==2)
	{
		tmp_offset=0;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREG;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
		tmp_offset=tmp_offset+LENGTH_S_BRACKET;


		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));

		tmp_offset=tmp_offset+strlen(sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==3)
	{
		tmp_offset=0;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=INST_ITEM_REGTYPE_P;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGDIS;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
		tmp_offset=tmp_offset+LENGTH_S_BRACKET;

		memset(tmp,0,sizeof(tmp));
		length=get_modrm_reg(xx_inst,INST_ITEM_MODRM_RM,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}


		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));
		tmp_offset=tmp_offset+strlen(sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				xx_inst->xx_inst_table.tbl_modrm->rm_sign,\
				strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
		tmp_offset=tmp_offset+strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);
		memset(tmp1,0,sizeof(tmp1));
		byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),\
				tmp1);

		memset(tmp,0,sizeof(tmp));
		nhex_str(tmp1,atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),tmp,"");

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
		tmp_offset=tmp_offset+strlen(tmp);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_EXIST;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);

		memcpy(xx_inst->xx_inst_code.displacement,\
			code+xx_inst->xx_inst_code.disasm_length,\
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));

		memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			code+xx_inst->xx_inst_code.disasm_length,\
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));

		xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
			atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);
		
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==4)
	{
		type_sib(nitem,xx_inst,code);
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==5)
	{
		type_sib_displacement(nitem,xx_inst,code);
	}
	else if(atoi(xx_inst->xx_inst_table.tbl_modrm->rm_type)==6)
	{
		if (xx_inst->xx_inst_code.inst_system == INST_SYSTEM_64)
		{
			type_sib_mdsp(nitem, xx_inst, code);
		}
		else
		{
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MDIS;

			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_EXIST;
			xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
			tmp_offset=tmp_offset+LENGTH_S_BRACKET;

			memset(tmp1,0,sizeof(tmp1));
			byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp1);


			memset(tmp,0,sizeof(tmp));
			nhex_str(tmp1,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,"");

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
			tmp_offset=tmp_offset+strlen(tmp);

			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;

			memcpy(xx_inst->xx_inst_code.immediate,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);

			memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
				code+xx_inst->xx_inst_code.disasm_length,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);

			xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;

		}
	}
	else 
	{
		return -1;
	}
	return 0;
}


int item_type_X(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];
	char sub_reg[100];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=atoi(xx_inst->xx_inst_table.tbl_item_type->reg_type);

	memset(tmp,0,sizeof(tmp));
	
	length=reg_def(xx_inst->xx_inst_table.tbl_item_type->reg_def,tmp,\
		xx_inst->xx_inst_exist.rex_flag, \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	if(!length)
	{
		return -1;
	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;
	
	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
	tmp_offset=tmp_offset+LENGTH_S_BRACKET;

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
			sub_reg,strlen(sub_reg));

	tmp_offset=tmp_offset+strlen(sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);
	return 0;
}

int item_type_Y(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int tmp_offset=0;
	int length=0;
	uchar tmp[200];
	char sub_reg[100];

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=atoi(xx_inst->xx_inst_table.tbl_item_type->reg_type);

	memset(tmp,0,sizeof(tmp));
	
	length=reg_def(xx_inst->xx_inst_table.tbl_item_type->reg_def,tmp,\
		xx_inst->xx_inst_exist.rex_flag, \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	if(!length)
	{
		return -1;
	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,\
			tmp,length);
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].reg_def_len=length;
	
	memset(sub_reg,0,sizeof(sub_reg));
	xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_reg_def,sub_reg);

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
	tmp_offset=tmp_offset+LENGTH_S_BRACKET;

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
			sub_reg,strlen(sub_reg));

	tmp_offset=tmp_offset+strlen(sub_reg);
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);
	return 0;

}

int item_type_CONST(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	int c=0;

	//printf("n:[%x] item_data_size:[%x]\n",nitem,xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size);

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_EXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_UNEXIST;

	c=atoi(xx_inst->xx_inst_table.tbl_item_type->type_mic);
	*(int *)xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const=c;
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size;

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic,\
			xx_inst->xx_inst_table.tbl_item_type->type_mic,\
			strlen(xx_inst->xx_inst_table.tbl_item_type->type_mic));

	return 0;
}


int type_sib(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	uchar tmp[200];
	uchar tmp1[200];
	int tmp_offset=0;
	int length=0;
	int tmp_size=0;
	char sub_reg[100];

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
	tmp_offset=tmp_offset+LENGTH_S_BRACKET;

	if(atoi(xx_inst->xx_inst_table.tbl_sib->base_special)!=1)
	{
		memset(tmp,0,sizeof(tmp));
		length=get_sib_reg(xx_inst,INST_ITEM_SIB_BASE,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));
		tmp_offset=tmp_offset+strlen(sub_reg);

		if (strcmp(xx_inst->xx_inst_table.tbl_sib->index_type, "1") == 0)
		{

			memset(tmp,0,sizeof(tmp));
			length=get_sib_reg(xx_inst,INST_ITEM_SIB_INDEX,\
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
					tmp,sizeof(tmp));
			if (length != 0)
			{

				if (tmp_offset != LENGTH_S_BRACKET)
				{
					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_modrm->rm_sign, \
						strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);
				}

				memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def, \
					tmp, length);
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len = length;

				memset(sub_reg, 0, sizeof(sub_reg));
				xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def, sub_reg);
				memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
					sub_reg, strlen(sub_reg));
				tmp_offset = tmp_offset + strlen(sub_reg);

				if (memcmp(xx_inst->xx_inst_table.tbl_sib->index_sign, "0", \
					strlen(xx_inst->xx_inst_table.tbl_sib->index_sign)) != 0)
				{
					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_sib->index_sign, \
						strlen(xx_inst->xx_inst_table.tbl_sib->index_sign));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_sib->index_sign);

					xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale = atoi(xx_inst->xx_inst_table.tbl_sib->index_scale_value);

					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_sib->index_scale_value, \
						strlen(xx_inst->xx_inst_table.tbl_sib->index_scale_value));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_sib->index_scale_value);
				}
			}
		}
	}
	else
	{
		if(strcmp(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,"00")==0)
		{
			tmp_size=4;
		}
		else if(strcmp(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,"01")==0)
		{
			memset(tmp,0,sizeof(tmp));
			length=get_sib_reg(xx_inst,INST_ITEM_SIB_BASE,\
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
					tmp,sizeof(tmp));
			if(length==0)
			{
				return -1;
			}
			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,\
					tmp,length);
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len=length;

			memset(sub_reg,0,sizeof(sub_reg));
			xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,sub_reg);
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
					sub_reg,strlen(sub_reg));
			tmp_offset=tmp_offset+strlen(sub_reg);

			tmp_size=1;
		}
		else if(strcmp(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,"10")==0)
		{
			memset(tmp,0,sizeof(tmp));
			length=get_sib_reg(xx_inst,INST_ITEM_SIB_BASE,\
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
					tmp,sizeof(tmp));
			if(length==0)
			{
				return -1;
			}
			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,\
					tmp,length);
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len=length;

			memset(sub_reg,0,sizeof(sub_reg));
			xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,sub_reg);
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
					sub_reg,strlen(sub_reg));
			tmp_offset=tmp_offset+strlen(sub_reg);

			tmp_size=4;
		}
		else
		{
			return -1;
		}

		if (strcmp(xx_inst->xx_inst_table.tbl_sib->index_type, "1") == 0)
		{
			
			memset(tmp,0,sizeof(tmp));
			length=get_sib_reg(xx_inst,INST_ITEM_SIB_INDEX,\
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
					tmp,sizeof(tmp));
			if (length != 0)
			{

				if (tmp_offset != LENGTH_S_BRACKET)
				{
					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_modrm->rm_sign, \
						strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);
				}

				memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def, \
					tmp, length);
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len = length;

				memset(sub_reg, 0, sizeof(sub_reg));
				xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def, sub_reg);
				memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
					sub_reg, strlen(sub_reg));
				tmp_offset = tmp_offset + strlen(sub_reg);

				if (memcmp(xx_inst->xx_inst_table.tbl_sib->index_sign, "0", \
					strlen(xx_inst->xx_inst_table.tbl_sib->index_sign)) != 0)
				{
					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_sib->index_sign, \
						strlen(xx_inst->xx_inst_table.tbl_sib->index_sign));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_sib->index_sign);

					xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale = atoi(xx_inst->xx_inst_table.tbl_sib->index_scale_value);

					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_sib->index_scale_value, \
						strlen(xx_inst->xx_inst_table.tbl_sib->index_scale_value));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_sib->index_scale_value);
				}
			}
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				xx_inst->xx_inst_table.tbl_modrm->rm_sign,\
				strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
		tmp_offset=tmp_offset+strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);

		memset(tmp1,0,sizeof(tmp1));
		byte_opposite(code+xx_inst->xx_inst_code.disasm_length,tmp_size,tmp1);

		memset(tmp,0,sizeof(tmp));
		nhex_str(tmp1,tmp_size,tmp,"");

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
		tmp_offset=tmp_offset+strlen(tmp);
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_EXIST;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				tmp_size);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=tmp_size;

		memcpy(xx_inst->xx_inst_code.displacement,\
				code+xx_inst->xx_inst_code.disasm_length,\
				tmp_size);

		memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
				code+xx_inst->xx_inst_code.disasm_length,tmp_size);

		xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+tmp_size;



	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

	if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MDIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size==0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGI;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size==0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGIDIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGISDIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MISDIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MIDIS;
	}
	else if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len != 0 && \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len == 0 && \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale == 0 && \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size != 0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type = INST_ITEM_TYPE_MREGDIS;
	}

	return 0;
}


int type_sib_displacement(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	uchar tmp[200];
	uchar tmp1[200];
	int tmp_offset=0;
	int length=0;
	int tmp_size=0;
	char sub_reg[100];

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
	tmp_offset=tmp_offset+LENGTH_S_BRACKET;

	if(atoi(xx_inst->xx_inst_table.tbl_sib->base_special)!=1)
	{
		memset(tmp,0,sizeof(tmp));
		length=get_sib_reg(xx_inst,INST_ITEM_SIB_BASE,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
				tmp,sizeof(tmp));
		if(length==0)
		{
			return -1;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,\
				tmp,length);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len=length;

		memset(sub_reg,0,sizeof(sub_reg));
		xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,sub_reg);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				sub_reg,strlen(sub_reg));
		tmp_offset=tmp_offset+strlen(sub_reg);


		if (strcmp(xx_inst->xx_inst_table.tbl_sib->index_type, "1") == 0)
		{
			
			memset(tmp,0,sizeof(tmp));
			length=get_sib_reg(xx_inst,INST_ITEM_SIB_INDEX,\
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
					tmp,sizeof(tmp));
			if (length != 0)
			{

				if (tmp_offset != LENGTH_S_BRACKET)
				{
					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_modrm->rm_sign, \
						strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);
				}

				memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def, \
					tmp, length);
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len = length;

				memset(sub_reg, 0, sizeof(sub_reg));
				xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def, sub_reg);
				memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
					sub_reg, strlen(sub_reg));
				tmp_offset = tmp_offset + strlen(sub_reg);

				if (memcmp(xx_inst->xx_inst_table.tbl_sib->index_sign, "0", \
					strlen(xx_inst->xx_inst_table.tbl_sib->index_sign)) != 0)
				{
					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_sib->index_sign, \
						strlen(xx_inst->xx_inst_table.tbl_sib->index_sign));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_sib->index_sign);

					xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale = atoi(xx_inst->xx_inst_table.tbl_sib->index_scale_value);

					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_sib->index_scale_value, \
						strlen(xx_inst->xx_inst_table.tbl_sib->index_scale_value));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_sib->index_scale_value);
				}
			}

		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				xx_inst->xx_inst_table.tbl_modrm->rm_sign,\
				strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
		tmp_offset=tmp_offset+strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);

		memset(tmp1,0,sizeof(tmp1));
		byte_opposite(code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),\
				tmp1);

		memset(tmp,0,sizeof(tmp));
		nhex_str(tmp1,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type),\
				tmp,"");

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
		tmp_offset=tmp_offset+strlen(tmp);
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_EXIST;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);

		memcpy(xx_inst->xx_inst_code.displacement,\
				code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));

		memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
				code+xx_inst->xx_inst_code.disasm_length,\
				atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type));

		xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
						    atoi(xx_inst->xx_inst_table.tbl_modrm->rm_displacement_type);

	}
	else
	{
		if(strcmp(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,"00")==0)
		{
			tmp_size=4;
		}
		else if(strcmp(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,"01")==0)
		{
			memset(tmp,0,sizeof(tmp));
			length=get_sib_reg(xx_inst,INST_ITEM_SIB_BASE,\
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
					tmp,sizeof(tmp));
			if(length==0)
			{
				return -1;
			}
			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,\
					tmp,length);
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len=length;

			memset(sub_reg,0,sizeof(sub_reg));
			xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,sub_reg);
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
					sub_reg,strlen(sub_reg));
			tmp_offset=tmp_offset+strlen(sub_reg);

			tmp_size=1;
		}
		else if(strcmp(xx_inst->xx_inst_table.tbl_modrm->mod_bitmic,"10")==0)
		{
			memset(tmp,0,sizeof(tmp));
			length=get_sib_reg(xx_inst,INST_ITEM_SIB_BASE,\
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
					tmp,sizeof(tmp));
			if(length==0)
			{
				return -1;
			}
			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,\
					tmp,length);
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len=length;

			memset(sub_reg,0,sizeof(sub_reg));
			xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_base_def,sub_reg);
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
					sub_reg,strlen(sub_reg));
			tmp_offset=tmp_offset+strlen(sub_reg);

			tmp_size=4;
		}
		else
		{
			return -1;
		}
		if (strcmp(xx_inst->xx_inst_table.tbl_sib->index_type, "1") == 0)
		{
			

			memset(tmp,0,sizeof(tmp));
			length=get_sib_reg(xx_inst,INST_ITEM_SIB_INDEX,\
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size,\
					tmp,sizeof(tmp));
			if (length != 0)
			{

				if (tmp_offset != LENGTH_S_BRACKET)
				{
					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_modrm->rm_sign, \
						strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);
				}

				memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def, \
					tmp, length);
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len = length;

				memset(sub_reg, 0, sizeof(sub_reg));
				xx_get_reg_mic(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_index_def, sub_reg);
				memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
					sub_reg, strlen(sub_reg));
				tmp_offset = tmp_offset + strlen(sub_reg);

				if (memcmp(xx_inst->xx_inst_table.tbl_sib->index_sign, "0", \
					strlen(xx_inst->xx_inst_table.tbl_sib->index_sign)) != 0)
				{
					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_sib->index_sign, \
						strlen(xx_inst->xx_inst_table.tbl_sib->index_sign));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_sib->index_sign);

					xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale = atoi(xx_inst->xx_inst_table.tbl_sib->index_scale_value);

					memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, \
						xx_inst->xx_inst_table.tbl_sib->index_scale_value, \
						strlen(xx_inst->xx_inst_table.tbl_sib->index_scale_value));
					tmp_offset = tmp_offset + strlen(xx_inst->xx_inst_table.tbl_sib->index_scale_value);
				}
			}
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,\
				xx_inst->xx_inst_table.tbl_modrm->rm_sign,\
				strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign));
		tmp_offset=tmp_offset+strlen(xx_inst->xx_inst_table.tbl_modrm->rm_sign);

		memset(tmp1,0,sizeof(tmp1));
		byte_opposite(code+xx_inst->xx_inst_code.disasm_length,tmp_size,tmp1);

		memset(tmp,0,sizeof(tmp));
		nhex_str(tmp1,tmp_size,tmp,"");

		memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
				strlen(tmp));
		tmp_offset=tmp_offset+strlen(tmp);

		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_EXIST;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				code+xx_inst->xx_inst_code.disasm_length,\
				tmp_size);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=tmp_size;

		memcpy(xx_inst->xx_inst_code.displacement,\
				code+xx_inst->xx_inst_code.disasm_length,\
				tmp_size);

		memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
				code+xx_inst->xx_inst_code.disasm_length,tmp_size);

		xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+tmp_size;

	}
	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

	if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MDIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size==0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGI;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size==0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGIDIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MREGISDIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MISDIS;
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len!=0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale==0 && \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MIDIS;
	}
	else if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].base_def_len != 0 && \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_def_len == 0 && \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].index_scale == 0 && \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size != 0)
	{
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type = INST_ITEM_TYPE_MREGDIS;
	}


	return 0;
}

int type_sib_mdsp(int nitem,struct XX_INST *xx_inst,uchar *code)
{
	uchar tmp[200];
	uchar tmp1[200];
	int tmp_offset=0;
	int length=0;
	int tmp_size=0;
	char sub_reg[100];
	ulong32 addr32=0;
	ulong64 addr64=0;
	ulong32 dsp32=0;
	ulong64 dsp64=0;


	tmp_size=4;

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].immediate_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].iaddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].maddress_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].displacement_flag=INST_FLAG_EXIST;

	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=INST_ITEM_TYPE_MDIS;
	memset(tmp,0,sizeof(tmp));
	memcpy(tmp,code+xx_inst->xx_inst_code.disasm_length,tmp_size);

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			code+xx_inst->xx_inst_code.disasm_length,tmp_size);

	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+tmp_size;



	if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size==INST_ITEM_SIZE_32)
	{
		dsp32=*(ulong*)tmp;
		addr32=*(ulong*)xx_inst->xx_inst_code.current_addr;
		if(dsp32>0x80000000)
		{
			dsp32=0xffffffff-dsp32+1;
			addr32=addr32-dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		else
		{
			addr32=addr32+dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				&addr32,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	}
	else if(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size==INST_ITEM_SIZE_64)
	{
		dsp32=*(ulong*)tmp;
		addr64=*(ulong64*)xx_inst->xx_inst_code.current_addr;
		if(dsp32>0x80000000)
		{
			dsp32=0xffffffff-dsp32+1;
			addr64=addr64-dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		else
		{
			addr64=addr64+dsp32+xx_inst->xx_inst_code.disasm_length;
		}
		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
				&addr64,\
				xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	}
	else
	{
	}



	xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size=xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;



	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,LEFT_S_BRACKET,LENGTH_S_BRACKET);
	tmp_offset=tmp_offset+LENGTH_S_BRACKET;
	memset(tmp1,0,sizeof(tmp1));
	byte_opposite(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const,\
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size,tmp1);

	memset(tmp,0,sizeof(tmp));
	nhex_str(tmp1,xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size,tmp,"");

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,tmp,\
			strlen(tmp));
	tmp_offset=tmp_offset+strlen(tmp);

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic+tmp_offset,RIGHT_S_BRACKET,LENGTH_S_BRACKET);

	xx_inst->xx_inst_exist.fix_item = INST_FLAG_EXIST;
	xx_inst->xx_inst_code.fix_item = nitem;

	return 0;
}




int  get_modrm_reg(struct XX_INST *xx_inst,int mod_flag,int reg_size,char *buf,int bufsize)
{
	int iret=0;
	int length=0;
	uchar tmp[200];
	char sub_reg[100];


	if(mod_flag==INST_ITEM_MODRM_REG)
	{
		if(reg_size==INST_ITEM_SIZE_8)
		{
			memset(tmp,0,sizeof(tmp));
			if(xx_inst->xx_inst_exist.rexr_flag==INST_FLAG_EXIST)
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_cp1,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			else
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_p1,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			if(length==0)
			{
				return 0;
			}
		}
		else if(reg_size==INST_ITEM_SIZE_16)
		{
			memset(tmp,0,sizeof(tmp));
			if(xx_inst->xx_inst_exist.rexr_flag==INST_FLAG_EXIST)
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_cp2,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			else
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_p2,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			if(length==0)
			{
				return 0;
			}
		}
		else if(reg_size==INST_ITEM_SIZE_32)
		{
			memset(tmp,0,sizeof(tmp));
			if(xx_inst->xx_inst_exist.rexr_flag==INST_FLAG_EXIST)
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_cp3,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			else
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_p3,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			if(length==0)
			{
				return 0;
			}
		}
		else if(reg_size==INST_ITEM_SIZE_64)
		{
			memset(tmp,0,sizeof(tmp));
			if(xx_inst->xx_inst_exist.rexr_flag==INST_FLAG_EXIST)
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_cp4,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			else
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_p4,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			if(length==0)
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	else if(mod_flag==INST_ITEM_MODRM_RM)
	{
		if(reg_size==INST_ITEM_SIZE_8)
		{
			memset(tmp,0,sizeof(tmp));
			if(xx_inst->xx_inst_exist.rexb_flag==INST_FLAG_EXIST)
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_cp1,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			else
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_p1,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			if(length==0)
			{
				return 0;
			}
		}
		else if(reg_size==INST_ITEM_SIZE_16)
		{
			memset(tmp,0,sizeof(tmp));
			if(xx_inst->xx_inst_exist.rexb_flag==INST_FLAG_EXIST)
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_cp2,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			else
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_p2,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			if(length==0)
			{
				return 0;
			}
		}
		else if(reg_size==INST_ITEM_SIZE_32)
		{
			memset(tmp,0,sizeof(tmp));
			if(xx_inst->xx_inst_exist.rexb_flag==INST_FLAG_EXIST)
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_cp3,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			else
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_p3,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			if(length==0)
			{
				return 0;
			}
		}
		else if(reg_size==INST_ITEM_SIZE_64)
		{
			memset(tmp,0,sizeof(tmp));
			if(xx_inst->xx_inst_exist.rexb_flag==INST_FLAG_EXIST)
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_cp4,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			else
			{
				length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_p4,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
			}
			if(length==0)
			{
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	else if(mod_flag==INST_ITEM_MODRM_REG_MMX)
	{
		memset(tmp,0,sizeof(tmp));
		length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_mmx,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
		if(length==0)
		{
			return 0;
		}
	}
	else if(mod_flag==INST_ITEM_MODRM_REG_XMM)
	{
		memset(tmp,0,sizeof(tmp));
		length=reg_def(xx_inst->xx_inst_table.tbl_modrm->reg_def_xmm,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
		if(length==0)
		{
			return 0;
		}
	}
	else if(mod_flag==INST_ITEM_MODRM_RM_REG_MMX)
	{
		memset(tmp,0,sizeof(tmp));
		length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_mmx,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
		if(length==0)
		{
			return 0;
		}
	}
	else if(mod_flag==INST_ITEM_MODRM_RM_REG_XMM)
	{
		memset(tmp,0,sizeof(tmp));
		length=reg_def(xx_inst->xx_inst_table.tbl_modrm->rm_def_xmm,tmp,\
					xx_inst->xx_inst_exist.rex_flag, \
					reg_size);
		if(length==0)
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}



	if(length>=bufsize)
	{
		return 0;
	}
	memcpy(buf,tmp,length);
	return length;

}


int  get_sib_reg(struct XX_INST *xx_inst,int sib_flag,int reg_size,char *buf,int bufsize)
{
	int iret=0;
	int length=0;
	uchar tmp[200];
	char sub_reg[100];


	if(sib_flag==INST_ITEM_SIB_BASE)
	{
		memset(tmp,0,sizeof(tmp));
		if(xx_inst->xx_inst_exist.rexb_flag==INST_FLAG_EXIST)
		{
			length=reg_def(xx_inst->xx_inst_table.tbl_sib->base_cmic,tmp,\
				xx_inst->xx_inst_exist.rex_flag, \
				reg_size);
		}
		else
		{
			length=reg_def(xx_inst->xx_inst_table.tbl_sib->base_mic,tmp,\
				xx_inst->xx_inst_exist.rex_flag, \
				reg_size);
		}
		if(length==0)
		{
			return 0;
		}
	}
	else if(sib_flag==INST_ITEM_SIB_INDEX)
	{
		memset(tmp,0,sizeof(tmp));
		if(xx_inst->xx_inst_exist.rexx_flag==INST_FLAG_EXIST)
		{
			length=reg_def(xx_inst->xx_inst_table.tbl_sib->index_cmic,tmp,\
				xx_inst->xx_inst_exist.rex_flag, \
				reg_size);
		}
		else
		{
			length=reg_def(xx_inst->xx_inst_table.tbl_sib->index_mic,tmp,\
				xx_inst->xx_inst_exist.rex_flag, \
				reg_size);
		}
		if(length==0)
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}



	if(length>=bufsize)
	{
		return 0;
	}
	memcpy(buf,tmp,length);
	return length;

}



int item_const_fix(struct XX_INST *xx_inst, int offset)
{
	uchar tmp[200];
	uchar tmp1[200];
	int tmp_offset = 0;
	int length = 0;
	int tmp_size = 0;
	//char *sub_reg;
	char sub_reg[100];
	ulong32 addr32 = 0;
	ulong64 addr64 = 0;
	ulong32 dsp32 = 0;
	ulong64 dsp64 = 0;
	int nitem = 0;

	nitem = xx_inst->xx_inst_code.fix_item;

	if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size == INST_ITEM_SIZE_32)
	{
		addr32 = *(ulong32*)xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const;

		memset(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const, 0, \
			sizeof(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const));

		addr32 = addr32 + offset;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const, \
			&addr32, \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	}
	else if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size == INST_ITEM_SIZE_64)
	{
		addr64 = *(ulong64*)xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const;

		memset(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const, 0, \
			sizeof(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const));

		addr64 = addr64 + offset;

		memcpy(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const, \
			&addr64, \
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size);
	}
	else
	{
		//zlog_info(zc, "item_data_size error");
	}

	/*给项常数赋值*/
	tmp_size = 0;
	xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size = xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size;

	memset(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic, 0, \
		sizeof(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic));

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, LEFT_S_BRACKET, LENGTH_S_BRACKET);
	tmp_offset = tmp_offset + LENGTH_S_BRACKET;
	memset(tmp1, 0, sizeof(tmp1));
	byte_opposite(xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_const, \
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size, tmp1);

	memset(tmp, 0, sizeof(tmp));
	nhex_str(tmp1, xx_inst->xx_inst_items.xx_inst_items_var[nitem].const_size, tmp, "");

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, tmp, \
		strlen(tmp));
	//zlog_info(zc, "item_self_mic:[%s]", xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic);
	tmp_offset = tmp_offset + strlen(tmp);

	memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_self_mic + tmp_offset, RIGHT_S_BRACKET, LENGTH_S_BRACKET);

	return 0;
}















