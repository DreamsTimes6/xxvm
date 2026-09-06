#include <stdio.h>
#include <memory.h>
//#include <mysql/mysql.h>
#include "g_sql.h"
#include "xx_def.h"

extern int init_flag;

//////////////////////

struct XX_INIT_TBL_LIST init_tbl_list[]=
{
	{"XX_TBL_PREFIX",           0,0,sizeof(struct XX_TBL_PREFIX),           sizeof(struct XX_TBL_PREFIX_LEN),\
		0,0,select_xx_prefix,          select_xx_prefix_length,"data_prefix","length_prefix"},\
	{"XX_TBL_ONE_OPCODE",       0,0,sizeof(struct XX_TBL_ONE_OPCODE),       sizeof(struct XX_TBL_ONE_OPCODE_LEN),\
		0,0,select_xx_one_opcode,      select_xx_one_opcode_length,"data_1opcode","length_1opcode"},\
	{"XX_TBL_TWO_OPCODE",       0,0,sizeof(struct XX_TBL_TWO_OPCODE),       sizeof(struct XX_TBL_TWO_OPCODE_LEN),\
		0,0,select_xx_two_opcode,      select_xx_two_opcode_length,"data_2opcode","length_2opcode"},\
	{"XX_TBL_THREE_OPCODE",     0,0,sizeof(struct XX_TBL_THREE_OPCODE),     sizeof(struct XX_TBL_THREE_OPCODE_LEN),\
		0,0,select_xx_three_opcode,    select_xx_three_opcode_length,"data_3opcode","length_3opcode"},\
	{"XX_TBL_EXT_ONE_OPCODE",   0,0,sizeof(struct XX_TBL_EXT_ONE_OPCODE),   sizeof(struct XX_TBL_EXT_ONE_OPCODE_LEN),\
		0,0,select_xx_ext_one_opcode,  select_xx_ext_one_opcode_length,"data_e1opcode","length_e1opcode"},\
	{"XX_TBL_EXT_TWO_OPCODE",   0,0,sizeof(struct XX_TBL_EXT_TWO_OPCODE),   sizeof(struct XX_TBL_EXT_TWO_OPCODE_LEN),\
		0,0,select_xx_ext_two_opcode,  select_xx_ext_two_opcode_length,"data_e2opcode","length_e2opcode"},\
	{"XX_TBL_EXT_THREE_OPCODE", 0,0,sizeof(struct XX_TBL_EXT_THREE_OPCODE), sizeof(struct XX_TBL_EXT_THREE_OPCODE_LEN),\
		0,0,select_xx_ext_three_opcode,select_xx_ext_three_opcode_length,"data_e3opcode","length_e3opcode"},\
	{"XX_TBL_MODRM",            0,0,sizeof(struct XX_TBL_MODRM),            sizeof(struct XX_TBL_MODRM_LEN),\
		0,0,select_xx_modrm,           select_xx_modrm_length,"data_modrm","length_modrm"},\
	{"XX_TBL_SIB",              0,0,sizeof(struct XX_TBL_SIB),              sizeof(struct XX_TBL_SIB_LEN),\
		0,0,select_xx_sib,             select_xx_sib_length,"data_sib","length_sib"},\
	{"XX_TBL_ITEM_TYPE",        0,0,sizeof(struct XX_TBL_ITEM_TYPE),        sizeof(struct XX_TBL_ITEM_TYPE_LEN),\
		0,0,select_xx_item_type,       select_xx_item_type_length,"data_type","length_type"},\
	{"XX_TBL_ITEM_LTYPE", 0, 0, sizeof(struct XX_TBL_ITEM_LTYPE), sizeof(struct XX_TBL_ITEM_LTYPE_LEN), \
	0, 0, select_xx_item_ltype, select_xx_item_ltype_length, "data_ltype", "length_ltype"}, \
	{"XX_TBL_FPU_OPCODE", 0, 0, sizeof(struct XX_TBL_FPU_OPCODE), sizeof(struct XX_TBL_FPU_OPCODE_LEN), \
	0, 0, select_xx_fpu_opcode, select_xx_fpu_opcode_length, "data_fpu", "length_fpu"},
};


_export    void* get_sql_struct(int tbl)
{
	if(init_flag==0)
	{
		return 0;
	}

	if(tbl<=0 || tbl>(sizeof(init_tbl_list)/sizeof(struct XX_INIT_TBL_LIST)))
	{
		return 0;
	}
	else
	{
		return init_tbl_list[tbl-1].ptbl;
	}
}

_export    void* get_sql_lstruct(int tbl)
{
	if(init_flag==0)
	{
		return 0;
	}

	if(tbl<=0 || tbl>(sizeof(init_tbl_list)/sizeof(struct XX_INIT_TBL_LIST)))
	{
		return 0;
	}
	else
	{
		return init_tbl_list[tbl-1].pltbl;
	}
}

 _export   int get_sql_rows_num(int tbl)
{
	if(init_flag==0)
	{
		return 0;
	}

	if(tbl<=0 || tbl>(sizeof(init_tbl_list)/sizeof(struct XX_INIT_TBL_LIST)))
	{
		return 0;
	}
	else
	{
		return init_tbl_list[tbl-1].rows_num;
	}
}

 _export   int get_sql_lrows_num(int tbl)
{
	if(init_flag==0)
	{
		return 0;
	}

	if(tbl<=0 || tbl>(sizeof(init_tbl_list)/sizeof(struct XX_INIT_TBL_LIST)))
	{
		return 0;
	}
	else
	{
		return init_tbl_list[tbl-1].lrows_num;
	}
}

_export    int get_sql_item_size(int tbl)
{
	if(init_flag==0)
	{
		return 0;
	}

	if(tbl<=0 || tbl>(sizeof(init_tbl_list)/sizeof(struct XX_INIT_TBL_LIST)))
	{
		return 0;
	}
	else
	{
		return init_tbl_list[tbl-1].item_size;
	}
}

_export    int get_sql_litem_size(int tbl)
{
	if(init_flag==0)
	{
		return 0;
	}

	if(tbl<=0 || tbl>(sizeof(init_tbl_list)/sizeof(struct XX_INIT_TBL_LIST)))
	{
		return 0;
	}
	else
	{
		return init_tbl_list[tbl-1].litem_size;
	}
}

_export    int get_sql_list_num()
{
	return sizeof(init_tbl_list)/sizeof(struct XX_INIT_TBL_LIST);
}

_export  struct XX_INIT_TBL_LIST* get_sql_list_st()
{
	return init_tbl_list;
}
