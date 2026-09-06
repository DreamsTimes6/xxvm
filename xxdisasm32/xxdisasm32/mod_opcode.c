#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm32.h"

  
extern int xx_fpu_opcode_mod(struct XX_INST* xx_inst,uchar* codes);



/////////////////////////////////////


int xx_one_opcode_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int n;
	int iret=0;
	int rows;
	uchar fbyte[100];
	uchar sfirst_opcode[20];
	struct XX_TBL_ONE_OPCODE *tbl_one_opcode;

	//printf("===xx_one_opcode_mod start===\n");

	memset(fbyte,0,sizeof(fbyte));	
 	
	if(xx_inst->xx_inst_exist.prefix_flag==INST_FLAG_NOTOP )
	{
		return -2;
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

	if(memcmp(xx_inst->xx_inst_table.tbl_one_opcode->fpu_opcode_flag,"1",1)==0)
	{
		iret=xx_fpu_opcode_mod(xx_inst,codes);
		if(iret==-1)
		{
			xx_inst->xx_inst_exist.one_opcode_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_exist.opcode_flag=INST_FLAG_NOTOP;
			xx_inst->xx_inst_exist.finish_flag=INST_ERROR_UNDEFINE;
			return -1;
		}
		return 0;
	}


	memcpy(xx_inst->xx_inst_code.first_opcode,fbyte,\
			sizeof(xx_inst->xx_inst_code.first_opcode));

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,fbyte,\
			sizeof(xx_inst->xx_inst_code.first_opcode));
	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+sizeof(xx_inst->xx_inst_code.first_opcode);


	//printf("===xx_one_opcode_mod end===\n");

	if(memcmp(xx_inst->xx_inst_table.tbl_one_opcode->two_opcode_flag,"1",1)==0)
	{
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_TWO_OPCODE;
		return 0;
	}
	else if(memcmp(xx_inst->xx_inst_table.tbl_one_opcode->ext_opcode_flag,"1",1)==0)
	{
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_EXT_ONE_OPCODE;
		return 0;
	}
	else
	{	

		xx_inst->xx_inst_exist.one_opcode_flag=INST_FLAG_EXIST;
		xx_inst->xx_inst_exist.two_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.three_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.ext_one_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.ext_two_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.ext_three_opcode_flag=INST_FLAG_UNEXIST;


		switch(xx_inst->xx_inst_code.inst_system)
		{
			case INST_SYSTEM_16:
				memcpy(xx_inst->xx_inst_mic.opcode_mic,\
						xx_inst->xx_inst_table.tbl_one_opcode->opcode_mic_1,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->opcode_mic_1));
				break;	
			case INST_SYSTEM_32:
				memcpy(xx_inst->xx_inst_mic.opcode_mic,\
						xx_inst->xx_inst_table.tbl_one_opcode->opcode_mic_2,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->opcode_mic_2));
				break;	
			case INST_SYSTEM_64:
				memcpy(xx_inst->xx_inst_mic.opcode_mic,\
						xx_inst->xx_inst_table.tbl_one_opcode->opcode_mic_3,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->opcode_mic_3));
				break;	
			default:
				break;
		}


		memcpy(xx_inst->xx_inst_code.opcode_type,\
				xx_inst->xx_inst_table.tbl_one_opcode->opcode_type,\
				strlen(xx_inst->xx_inst_table.tbl_one_opcode->opcode_type));
		
		memcpy(xx_inst->xx_inst_code.var_seq,\
				xx_inst->xx_inst_table.tbl_one_opcode->var_seq,\
				strlen(xx_inst->xx_inst_table.tbl_one_opcode->var_seq));
		memcpy(xx_inst->xx_inst_code.eflag,\
				xx_inst->xx_inst_table.tbl_one_opcode->eflag,\
				strlen(xx_inst->xx_inst_table.tbl_one_opcode->eflag));

		memcpy(xx_inst->xx_inst_code.sups, \
			xx_inst->xx_inst_table.tbl_one_opcode->superscripts, \
			strlen(xx_inst->xx_inst_table.tbl_one_opcode->superscripts));

		if(memcmp(xx_inst->xx_inst_table.tbl_one_opcode->modrm_exist_flag,"1",1)==0)
		{
			xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_EXIST;
			xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_MODRM;
			return 0;
		}
		else
		{
			xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ITEMS;
			return 0;
		}
	}	
	return -1;        	
}


int xx_ext_one_opcode_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int n;
	int rows;
	uchar sfirst_opcode[20];
	uchar sprefix[20];
	uchar smod[20];
	uchar sreg_opcode[20];
	uchar srm[20];
	uchar fbyte[100];
	struct XX_TBL_MODRM *tbls_modrm;
	struct XX_TBL_MODRM *tbl_modrm;
	struct XX_TBL_EXT_ONE_OPCODE *tbl_ext_one_opcode;

	//printf("===xx_ext_one_opcode_mod start===\n");

	memset(fbyte,0,sizeof(fbyte));	
	memcpy(fbyte,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));


	tbl_ext_one_opcode=get_sql_struct(TBL_EXT_ONE_OPCODE);
	if(!tbl_ext_one_opcode)
	{
		return -1;
	}

	rows=get_sql_rows_num(TBL_EXT_ONE_OPCODE);
	if(!rows)
	{
		return -1;
	}

	tbls_modrm=get_sql_struct(TBL_MODRM);
	if(!tbls_modrm)
	{
		return -1;
	}
	tbl_modrm=(void*)&tbls_modrm[*fbyte];

	memset(sfirst_opcode,0,sizeof(sfirst_opcode));	
	memcpy(sfirst_opcode,xx_inst->xx_inst_table.tbl_one_opcode->opcode,\
			strlen(xx_inst->xx_inst_table.tbl_one_opcode->opcode));

	memset(sprefix,0,sizeof(sprefix));	
	if(xx_inst->xx_inst_exist.prefix_flag==INST_FLAG_EXIST)
	{
		memcpy(sprefix,xx_inst->xx_inst_table.tbl_prefix->prefix,\
				strlen(xx_inst->xx_inst_table.tbl_prefix->prefix));
	}

	memset(smod,0,sizeof(smod));	
	memcpy(smod,tbl_modrm->mod,strlen(tbl_modrm->mod));

	memset(sreg_opcode,0,sizeof(sreg_opcode));	
	memcpy(sreg_opcode,tbl_modrm->reg_opcode,strlen(tbl_modrm->reg_opcode));

	memset(srm,0,sizeof(srm));	
	memcpy(srm,tbl_modrm->rm,strlen(tbl_modrm->rm));

	n=seek_ext_one_opcode_tbl(tbl_ext_one_opcode,rows,sfirst_opcode,\
			smod,sreg_opcode,srm);
	if(n==-1)
	{
		return -1;
	}
	xx_inst->xx_inst_table.tbl_ext_one_opcode=(void*)&tbl_ext_one_opcode[n];

	memcpy(xx_inst->xx_inst_code.ext_first_opcode,\
			codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));

	xx_inst->xx_inst_exist.one_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.two_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.three_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.ext_one_opcode_flag=INST_FLAG_EXIST;
	xx_inst->xx_inst_exist.ext_two_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.ext_three_opcode_flag=INST_FLAG_UNEXIST;

	switch(xx_inst->xx_inst_code.inst_system)
	{
		case INST_SYSTEM_16:
			memcpy(xx_inst->xx_inst_mic.opcode_mic,\
					xx_inst->xx_inst_table.tbl_ext_one_opcode->opcode_mic_1,\
					strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->opcode_mic_1));
			break;	
		case INST_SYSTEM_32:
			memcpy(xx_inst->xx_inst_mic.opcode_mic,\
					xx_inst->xx_inst_table.tbl_ext_one_opcode->opcode_mic_2,\
					strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->opcode_mic_2));
			break;	
		case INST_SYSTEM_64:
			memcpy(xx_inst->xx_inst_mic.opcode_mic,\
					xx_inst->xx_inst_table.tbl_ext_one_opcode->opcode_mic_3,\
					strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->opcode_mic_3));
			break;	
		default:
			break;
	}

	memcpy(xx_inst->xx_inst_code.opcode_type,\
			xx_inst->xx_inst_table.tbl_ext_one_opcode->opcode_type,\
			strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->opcode_type));

	memcpy(xx_inst->xx_inst_code.var_seq,\
			xx_inst->xx_inst_table.tbl_ext_one_opcode->var_seq,\
			strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->var_seq));
	memcpy(xx_inst->xx_inst_code.eflag,\
			xx_inst->xx_inst_table.tbl_ext_one_opcode->eflag,\
			strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->eflag));

	memcpy(xx_inst->xx_inst_code.sups, \
		xx_inst->xx_inst_table.tbl_ext_one_opcode->superscripts, \
		strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->superscripts));

	//printf("===xx_ext_one_opcode_mod end===\n");

	if(memcmp(xx_inst->xx_inst_table.tbl_ext_one_opcode->modrm_exist_flag,"1",1)==0)
	{
		xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_EXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_MODRM;
		return 0;
	}
	else
	{
		xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ITEMS;
		return 0;
	}
	return -1;
}

int xx_two_opcode_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int n;
	int rows;
	uchar ssecond_opcode[20];
	uchar sprefix[20];
	uchar fbyte[100];
	struct XX_TBL_TWO_OPCODE *tbl_two_opcode;

	//printf("===xx_two_opcode_mod start===\n");

	memset(fbyte,0,sizeof(fbyte));	

	memcpy(fbyte,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.second_opcode));

	memcpy(xx_inst->xx_inst_code.second_opcode,\
			codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.second_opcode));

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			fbyte,\
			sizeof(xx_inst->xx_inst_code.second_opcode));
	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
					    sizeof(xx_inst->xx_inst_code.second_opcode);

	tbl_two_opcode=get_sql_struct(TBL_TWO_OPCODE);
	if(!tbl_two_opcode)
	{
		//printf("error:get_sql_struct\n");
		return -1;
	}

	rows=get_sql_rows_num(TBL_TWO_OPCODE);
	if(!rows)
	{
		//printf("error:get_sql_rows_num\n");
		return -1;
	}

	memset(ssecond_opcode,0,sizeof(ssecond_opcode));	
	nhex_str(fbyte,1,ssecond_opcode,"");

	memset(sprefix,0,sizeof(sprefix));	
	if(xx_inst->xx_inst_exist.prefix_flag==INST_FLAG_EXIST)
	{
		memcpy(sprefix,xx_inst->xx_inst_table.tbl_prefix->prefix,\
				strlen(xx_inst->xx_inst_table.tbl_prefix->prefix));
	}

	n=seek_two_opcode_tbl(tbl_two_opcode,rows,ssecond_opcode,sprefix);
	if(n==-1)
	{
		//printf("error:seek_two_opcode_tbl\n");
		return -1;
	}
	xx_inst->xx_inst_table.tbl_two_opcode=(void*)&tbl_two_opcode[n];

	//printf("===xx_two_opcode_mod end===\n");

	if(memcmp(xx_inst->xx_inst_table.tbl_two_opcode->three_opcode_flag,"1",1)==0)
	{
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_THREE_OPCODE;
		return 0;
	}
	else if(memcmp(xx_inst->xx_inst_table.tbl_two_opcode->ext_opcode_flag,"1",1)==0)
	{
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_EXT_TWO_OPCODE;
		return 0;
	}
	else
	{	

		xx_inst->xx_inst_exist.one_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.two_opcode_flag=INST_FLAG_EXIST;    
		xx_inst->xx_inst_exist.three_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.ext_one_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.ext_two_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.ext_three_opcode_flag=INST_FLAG_UNEXIST;

		switch(xx_inst->xx_inst_code.inst_system)
		{
			case INST_SYSTEM_16:
				memcpy(xx_inst->xx_inst_mic.opcode_mic,\
						xx_inst->xx_inst_table.tbl_two_opcode->opcode_mic_1,\
						sizeof(xx_inst->xx_inst_table.tbl_two_opcode->opcode_mic_1));
				break;	
			case INST_SYSTEM_32:
				memcpy(xx_inst->xx_inst_mic.opcode_mic,\
						xx_inst->xx_inst_table.tbl_two_opcode->opcode_mic_2,\
						sizeof(xx_inst->xx_inst_table.tbl_two_opcode->opcode_mic_2));
				break;	
			case INST_SYSTEM_64:
				memcpy(xx_inst->xx_inst_mic.opcode_mic,\
						xx_inst->xx_inst_table.tbl_two_opcode->opcode_mic_3,\
						sizeof(xx_inst->xx_inst_table.tbl_two_opcode->opcode_mic_3));
				break;	
			default:
				break;
		}

		memcpy(xx_inst->xx_inst_code.opcode_type,\
				xx_inst->xx_inst_table.tbl_two_opcode->opcode_type,\
				strlen(xx_inst->xx_inst_table.tbl_two_opcode->opcode_type));

		memcpy(xx_inst->xx_inst_code.var_seq,\
				xx_inst->xx_inst_table.tbl_two_opcode->var_seq,\
				strlen(xx_inst->xx_inst_table.tbl_two_opcode->var_seq));
		memcpy(xx_inst->xx_inst_code.eflag,\
				xx_inst->xx_inst_table.tbl_two_opcode->eflag,\
				strlen(xx_inst->xx_inst_table.tbl_two_opcode->eflag));

		memcpy(xx_inst->xx_inst_code.sups, \
			xx_inst->xx_inst_table.tbl_two_opcode->superscripts, \
			strlen(xx_inst->xx_inst_table.tbl_two_opcode->superscripts));


		if(memcmp(xx_inst->xx_inst_table.tbl_two_opcode->modrm_exist_flag,"1",1)==0)
		{
			xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_EXIST;
			xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_MODRM;
			return 0;
		}
		else
		{
			xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ITEMS;
			return 0;
		}
	}
	return -1;
}

int xx_ext_two_opcode_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int n;
	int rows;
	uchar sprefix[20];
	uchar sfirst_opcode[20];
	uchar ssecond_opcode[20];
	uchar smod[20];
	uchar sreg_opcode[20];
	uchar srm[20];
	uchar fbyte[100];
	struct XX_TBL_MODRM *tbls_modrm;
	struct XX_TBL_MODRM *tbl_modrm;
	struct XX_TBL_EXT_TWO_OPCODE *tbl_ext_two_opcode;

	//printf("===xx_ext_two_opcode_mod start===\n");

	memset(fbyte,0,sizeof(fbyte));	

	memcpy(fbyte,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));


	tbl_ext_two_opcode=get_sql_struct(TBL_EXT_TWO_OPCODE);
	if(!tbl_ext_two_opcode)
	{
		return -1;
	}
	rows=get_sql_rows_num(TBL_EXT_TWO_OPCODE);
	if(!rows)
	{
		return -1;
	}

	tbls_modrm=get_sql_struct(TBL_MODRM);
	if(!tbls_modrm)
	{
		return -1;
	}
	tbl_modrm=(void*)&tbls_modrm[*fbyte];

	memset(ssecond_opcode,0,sizeof(ssecond_opcode));	
	nhex_str(xx_inst->xx_inst_code.second_opcode,sizeof(xx_inst->xx_inst_code.second_opcode),ssecond_opcode,"");

	memset(sprefix,0,sizeof(sprefix));	
	if(xx_inst->xx_inst_exist.prefix_flag==INST_FLAG_EXIST)
	{
		memcpy(sprefix,xx_inst->xx_inst_table.tbl_prefix->prefix,\
				strlen(xx_inst->xx_inst_table.tbl_prefix->prefix));
	}

	memset(smod,0,sizeof(smod));	
	memcpy(smod,tbl_modrm->mod,strlen(tbl_modrm->mod));

	memset(sreg_opcode,0,sizeof(sreg_opcode));	
	memcpy(sreg_opcode,tbl_modrm->reg_opcode,strlen(tbl_modrm->reg_opcode));

	memset(srm,0,sizeof(srm));	
	memcpy(srm,tbl_modrm->rm,strlen(tbl_modrm->rm));

	n=seek_ext_two_opcode_tbl(tbl_ext_two_opcode,rows,ssecond_opcode,sprefix,\
			smod,sreg_opcode,srm);
	if(n==-1)
	{
		return -1;
	}
	xx_inst->xx_inst_table.tbl_ext_two_opcode=(void*)&tbl_ext_two_opcode[n];

	memcpy(xx_inst->xx_inst_code.ext_second_opcode,\
			codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));

	xx_inst->xx_inst_exist.one_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.two_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.three_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.ext_one_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.ext_two_opcode_flag=INST_FLAG_EXIST;   
	xx_inst->xx_inst_exist.ext_three_opcode_flag=INST_FLAG_UNEXIST;

	switch(xx_inst->xx_inst_code.inst_system)
	{
		case INST_SYSTEM_16:
			memcpy(xx_inst->xx_inst_mic.opcode_mic,\
					xx_inst->xx_inst_table.tbl_ext_two_opcode->opcode_mic_1,\
					sizeof(xx_inst->xx_inst_table.tbl_ext_two_opcode->opcode_mic_1));
			break;	
		case INST_SYSTEM_32:
			memcpy(xx_inst->xx_inst_mic.opcode_mic,\
					xx_inst->xx_inst_table.tbl_ext_two_opcode->opcode_mic_2,\
					sizeof(xx_inst->xx_inst_table.tbl_ext_two_opcode->opcode_mic_2));
			break;	
		case INST_SYSTEM_64:
			memcpy(xx_inst->xx_inst_mic.opcode_mic,\
					xx_inst->xx_inst_table.tbl_ext_two_opcode->opcode_mic_3,\
					sizeof(xx_inst->xx_inst_table.tbl_ext_two_opcode->opcode_mic_3));
			break;	
		default:
			break;
	}

	memcpy(xx_inst->xx_inst_code.opcode_type,\
			xx_inst->xx_inst_table.tbl_ext_two_opcode->opcode_type,\
			strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->opcode_type));

	memcpy(xx_inst->xx_inst_code.var_seq,\
			xx_inst->xx_inst_table.tbl_ext_two_opcode->var_seq,\
			strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->var_seq));
	memcpy(xx_inst->xx_inst_code.eflag,\
			xx_inst->xx_inst_table.tbl_ext_two_opcode->eflag,\
			strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->eflag));

	memcpy(xx_inst->xx_inst_code.sups, \
		xx_inst->xx_inst_table.tbl_ext_two_opcode->superscripts, \
		strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->superscripts));

	//printf("===xx_ext_two_opcode_mod end===\n");

	if(memcmp(xx_inst->xx_inst_table.tbl_ext_two_opcode->modrm_exist_flag,"1",1)==0)
	{
		xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_EXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_MODRM;
		return 0;
	}
	else
	{
		xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ITEMS;
		return 0;

	}
	return -1;
}

int xx_three_opcode_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int n;
	int rows;
	uchar sprefix[20];
	uchar sfirst_opcode[20];
	uchar ssecond_opcode[20];
	uchar sthird_opcode[20];
	uchar smod[20];
	uchar sreg_opcode[20];
	uchar srm[20];
	uchar fbyte[100];
	struct XX_TBL_MODRM *tbls_modrm;
	struct XX_TBL_MODRM *tbl_modrm;
	struct XX_TBL_THREE_OPCODE *tbl_three_opcode;

	//printf("===xx_three_opcode_mod start===\n");

	memset(fbyte,0,sizeof(fbyte));	

	memcpy(fbyte,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.third_opcode));

	memcpy(xx_inst->xx_inst_code.third_opcode,\
			codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.third_opcode));

	memcpy(xx_inst->xx_inst_code.disasm+xx_inst->xx_inst_code.disasm_length,\
			fbyte,\
			sizeof(xx_inst->xx_inst_code.third_opcode));
	xx_inst->xx_inst_code.disasm_length=xx_inst->xx_inst_code.disasm_length+ \
					    sizeof(xx_inst->xx_inst_code.third_opcode);

	tbl_three_opcode=get_sql_struct(TBL_THREE_OPCODE);
	if(!tbl_three_opcode)
	{
		//printf("error:get_sql_struct\n");
		return -1;
	}

	rows=get_sql_rows_num(TBL_THREE_OPCODE);
	if(!rows)
	{
		//printf("error:get_sql_rows_num\n");
		return -1;
	}
	
	memset(ssecond_opcode,0,sizeof(ssecond_opcode));	
	nhex_str(xx_inst->xx_inst_code.second_opcode,1,ssecond_opcode,"");
	
	memset(sthird_opcode,0,sizeof(sthird_opcode));	
	nhex_str(xx_inst->xx_inst_code.third_opcode,1,sthird_opcode,"");
	

	if(xx_inst->xx_inst_code.nprefix!=0)
	{
		memset(sprefix,0,sizeof(sprefix));	
		nhex_str(xx_inst->xx_inst_code.prefix,xx_inst->xx_inst_code.prefix_len,sprefix,"");
	}

	n=seek_three_opcode_tbl(tbl_three_opcode,rows,ssecond_opcode,\
			sthird_opcode,\
			xx_inst->xx_inst_code.nprefix,\
			xx_inst->xx_inst_code.prefix);
	if(n==-1)
	{
		//printf("error:seek_three_opcode_tbl rows:[%x] ssecond_opcode:[%s] sthird_opcode:[%s]\n",rows,ssecond_opcode,sthird_opcode);
		//printf("error:xx_inst->xx_inst_code.nprefix:[%x] \n",xx_inst->xx_inst_code.nprefix);
		return -1;
	}
	xx_inst->xx_inst_table.tbl_three_opcode=(void*)&tbl_three_opcode[n];

	//printf("===xx_three_opcode_mod end===\n");

	if(memcmp(xx_inst->xx_inst_table.tbl_three_opcode->ext_opcode_flag,"1",1)==0)
	{
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_EXT_THREE_OPCODE;
		return 0;
	}
	else
	{	
		xx_inst->xx_inst_exist.one_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.two_opcode_flag=INST_FLAG_UNEXIST;    
		xx_inst->xx_inst_exist.three_opcode_flag=INST_FLAG_EXIST;   
		xx_inst->xx_inst_exist.ext_one_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.ext_two_opcode_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.ext_three_opcode_flag=INST_FLAG_UNEXIST;

		switch(xx_inst->xx_inst_code.inst_system)
		{
			case INST_SYSTEM_16:
				memcpy(xx_inst->xx_inst_mic.opcode_mic,\
						xx_inst->xx_inst_table.tbl_three_opcode->opcode_mic_1,\
						sizeof(xx_inst->xx_inst_table.tbl_three_opcode->opcode_mic_1));
				break;	
			case INST_SYSTEM_32:
				memcpy(xx_inst->xx_inst_mic.opcode_mic,\
						xx_inst->xx_inst_table.tbl_three_opcode->opcode_mic_2,\
						sizeof(xx_inst->xx_inst_table.tbl_three_opcode->opcode_mic_2));
				break;	
			case INST_SYSTEM_64:
				memcpy(xx_inst->xx_inst_mic.opcode_mic,\
						xx_inst->xx_inst_table.tbl_three_opcode->opcode_mic_3,\
						sizeof(xx_inst->xx_inst_table.tbl_three_opcode->opcode_mic_3));
				break;	
			default:
				break;
		}

		memcpy(xx_inst->xx_inst_code.opcode_type,\
				xx_inst->xx_inst_table.tbl_three_opcode->opcode_type,\
				strlen(xx_inst->xx_inst_table.tbl_three_opcode->opcode_type));

		memcpy(xx_inst->xx_inst_code.var_seq,\
				xx_inst->xx_inst_table.tbl_three_opcode->var_seq,\
				strlen(xx_inst->xx_inst_table.tbl_three_opcode->var_seq));
		memcpy(xx_inst->xx_inst_code.eflag,\
				xx_inst->xx_inst_table.tbl_three_opcode->eflag,\
				strlen(xx_inst->xx_inst_table.tbl_three_opcode->eflag));

		memcpy(xx_inst->xx_inst_code.sups, \
			xx_inst->xx_inst_table.tbl_three_opcode->superscripts, \
			strlen(xx_inst->xx_inst_table.tbl_three_opcode->superscripts));

		if(memcmp(xx_inst->xx_inst_table.tbl_three_opcode->modrm_exist_flag,"1",1)==0)
		{
			xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_EXIST;
			xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_MODRM;
			return 0;
		}
		else
		{
			xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_UNEXIST;
			xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ITEMS;
			return 0;
		}
	}
	return -1;
}


int xx_ext_three_opcode_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int n;
	int rows;
	uchar sprefix[20];
	uchar sfirst_opcode[20];
	uchar ssecond_opcode[20];
	uchar sthird_opcode[20];
	uchar smod[20];
	uchar sreg_opcode[20];
	uchar srm[20];
	uchar fbyte[100];
	struct XX_TBL_MODRM *tbls_modrm;
	struct XX_TBL_MODRM *tbl_modrm;
	struct XX_TBL_EXT_THREE_OPCODE *tbl_ext_three_opcode;

	//printf("===xx_ext_three_opcode_mod start===\n");

	memset(fbyte,0,sizeof(fbyte));	

	memcpy(fbyte,codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));

	tbl_ext_three_opcode=get_sql_struct(TBL_EXT_THREE_OPCODE);
	if(!tbl_ext_three_opcode)
	{
		return -1;
	}

	rows=get_sql_rows_num(TBL_EXT_THREE_OPCODE);
	if(!rows)
	{
		return -1;
	}

	tbls_modrm=get_sql_struct(TBL_MODRM);
	if(!tbls_modrm)
	{
		return -1;
	}
	tbl_modrm=(void*)&tbls_modrm[*fbyte];

	memset(sprefix,0,sizeof(sprefix));	
	if(xx_inst->xx_inst_exist.prefix_flag==INST_FLAG_EXIST)
	{
		memcpy(sprefix,xx_inst->xx_inst_table.tbl_prefix->prefix,\
				strlen(xx_inst->xx_inst_table.tbl_prefix->prefix));
	}

	memset(ssecond_opcode,0,sizeof(ssecond_opcode));	
	nhex_str(xx_inst->xx_inst_code.second_opcode,sizeof(xx_inst->xx_inst_code.second_opcode),ssecond_opcode,"");

	memset(sthird_opcode,0,sizeof(sthird_opcode));	
	nhex_str(xx_inst->xx_inst_code.third_opcode,sizeof(xx_inst->xx_inst_code.third_opcode),sthird_opcode,"");

	memset(smod,0,sizeof(smod));	
	memcpy(smod,tbl_modrm->mod,strlen(tbl_modrm->mod));

	memset(sreg_opcode,0,sizeof(sreg_opcode));	
	memcpy(sreg_opcode,tbl_modrm->reg_opcode,strlen(tbl_modrm->reg_opcode));

	memset(srm,0,sizeof(srm));	
	memcpy(srm,tbl_modrm->rm,strlen(tbl_modrm->rm));

	n=seek_ext_three_opcode_tbl(tbl_ext_three_opcode,rows,ssecond_opcode,sthird_opcode,sprefix,\
			smod,sreg_opcode,srm);
	if(n==-1)
	{
		return -1;
	}
	xx_inst->xx_inst_table.tbl_ext_three_opcode=(void*)&tbl_ext_three_opcode[n];

	memcpy(xx_inst->xx_inst_code.ext_third_opcode,\
			codes+xx_inst->xx_inst_code.disasm_length,\
			sizeof(xx_inst->xx_inst_code.modrm));

	xx_inst->xx_inst_exist.one_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.three_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.three_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.ext_one_opcode_flag=INST_FLAG_UNEXIST;
	xx_inst->xx_inst_exist.ext_three_opcode_flag=INST_FLAG_EXIST;   
	xx_inst->xx_inst_exist.ext_three_opcode_flag=INST_FLAG_UNEXIST;

	switch(xx_inst->xx_inst_code.inst_system)
	{
		case INST_SYSTEM_16:
			memcpy(xx_inst->xx_inst_mic.opcode_mic,\
					xx_inst->xx_inst_table.tbl_ext_three_opcode->opcode_mic_1,\
					sizeof(xx_inst->xx_inst_table.tbl_ext_three_opcode->opcode_mic_1));
			break;	
		case INST_SYSTEM_32:
			memcpy(xx_inst->xx_inst_mic.opcode_mic,\
					xx_inst->xx_inst_table.tbl_ext_three_opcode->opcode_mic_2,\
					sizeof(xx_inst->xx_inst_table.tbl_ext_three_opcode->opcode_mic_2));
			break;	
		case INST_SYSTEM_64:
			memcpy(xx_inst->xx_inst_mic.opcode_mic,\
					xx_inst->xx_inst_table.tbl_ext_three_opcode->opcode_mic_3,\
					sizeof(xx_inst->xx_inst_table.tbl_ext_three_opcode->opcode_mic_3));
			break;	
		default:
			break;
	}

	memcpy(xx_inst->xx_inst_code.opcode_type,\
			xx_inst->xx_inst_table.tbl_ext_three_opcode->opcode_type,\
			strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->opcode_type));

	memcpy(xx_inst->xx_inst_code.var_seq,\
			xx_inst->xx_inst_table.tbl_ext_three_opcode->var_seq,\
			strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->var_seq));
	memcpy(xx_inst->xx_inst_code.eflag,\
			xx_inst->xx_inst_table.tbl_ext_three_opcode->eflag,\
			strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->eflag));

	memcpy(xx_inst->xx_inst_code.sups, \
		xx_inst->xx_inst_table.tbl_ext_three_opcode->superscripts, \
		strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->superscripts));

	//printf("===xx_ext_three_opcode_mod end===\n");

	if(memcmp(xx_inst->xx_inst_table.tbl_ext_three_opcode->modrm_exist_flag,"1",1)==0)
	{
		xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_EXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_MODRM;
		return 0;
	}
	else
	{
		xx_inst->xx_inst_exist.modrm_flag=INST_FLAG_UNEXIST;
		xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_ITEMS;
		return 0;

	}
	return -1;
}




