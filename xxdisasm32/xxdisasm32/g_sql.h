#include <stdio.h>
#include "xx_mysql.h"
#include "xx_def.h"

#pragma once


#define select_xx_prefix "select * from xxasm.xx_tbl_prefix"
#define select_xx_one_opcode "select * from xxasm.xx_tbl_one_opcode"
#define select_xx_two_opcode "select * from xxasm.xx_tbl_two_opcode"
#define select_xx_three_opcode "select * from xxasm.xx_tbl_three_opcode"
#define select_xx_ext_one_opcode "select * from xxasm.xx_tbl_ext_one_opcode"
#define select_xx_ext_two_opcode "select * from xxasm.xx_tbl_ext_two_opcode"
#define select_xx_ext_three_opcode "select * from xxasm.xx_tbl_ext_three_opcode"
#define select_xx_modrm "select * from xxasm.xx_tbl_modrm"
#define select_xx_sib "select * from xxasm.xx_tbl_sib"
#define select_xx_item_type "select * from xxasm.xx_tbl_item_type"
#define select_xx_item_ltype "select * from xxasm.xx_tbl_item_ltype"
#define select_xx_fpu_opcode "select * from xxasm.xx_tbl_fpu_opcode"


#define select_xx_prefix_length "select * from xxasm.xx_tbl_prefix_length"
#define select_xx_one_opcode_length "select * from xxasm.xx_tbl_one_opcode_length"
#define select_xx_two_opcode_length "select * from xxasm.xx_tbl_two_opcode_length"
#define select_xx_three_opcode_length "select * from xxasm.xx_tbl_three_opcode_length"
#define select_xx_ext_one_opcode_length "select * from xxasm.xx_tbl_ext_one_opcode_length"
#define select_xx_ext_two_opcode_length "select * from xxasm.xx_tbl_ext_two_opcode_length"
#define select_xx_ext_three_opcode_length "select * from xxasm.xx_tbl_ext_three_opcode_length"
#define select_xx_modrm_length "select * from xxasm.xx_tbl_modrm_length"
#define select_xx_sib_length "select * from xxasm.xx_tbl_sib_length"
#define select_xx_item_type_length "select * from xxasm.xx_tbl_item_type_length"
#define select_xx_item_ltype_length "select * from xxasm.xx_tbl_item_ltype_length"
#define select_xx_fpu_opcode_length  "select * from xxasm.xx_tbl_fpu_opcode_length "



#define select_xx_opmz_inst "select * from xxasm.xx_tbl_opmz_inst "
#define select_xx_opmz_inst_length "select * from xxasm.xx_tbl_opmz_inst_length "

#define select_xx_opmz_item "select * from xxasm.xx_tbl_opmz_item "
#define select_xx_opmz_item_length "select * from xxasm.xx_tbl_opmz_item_length "

#define select_xx_opmz_inst_rule "select * from xxasm.xx_tbl_opmz_inst_rule "
#define select_xx_opmz_inst_rule_length "select * from xxasm.xx_tbl_opmz_inst_rule_length "





#define TBL_PREFIX            1
#define TBL_ONE_OPCODE        2
#define TBL_TWO_OPCODE        3
#define TBL_THREE_OPCODE      4
#define TBL_EXT_ONE_OPCODE    5
#define TBL_EXT_TWO_OPCODE    6
#define TBL_EXT_THREE_OPCODE  7
#define TBL_MODRM             8
#define TBL_SIB               9
#define TBL_ITEM_TYPE         10
#define TBL_ITEM_LTYPE        11
#define TBL_FPU_OPCODE        12



#define ZLOG_CONF      "zlog.conf"


#define INIT_TBL_LIST  1
#define OPMZ_INST_TBL_LIST  2
#define OPMZ_ITEM_TBL_LIST  3
#define OPMZ_INST_RULE_TBL_LIST  4

#define COMMA   ","
#define LPTHS   "("
#define RPTHS   ")"
#define SMCL    ";"
#define DQTM   "\"" 
#define SLEN    1

#if 0
#ifndef DLL_PUBLIC
#define DLL_PUBLIC __attribute__((visibility("default")))
#endif



void* get_sql_struct(int tbl);
void* get_sql_lstruct(int tbl);
int get_sql_rows_num(int tbl);
int get_sql_lrows_num(int tbl);
int get_sql_item_size(int tbl);
int get_sql_litem_size(int tbl);
int get_sql_list_num();
struct XX_INIT_TBL_LIST* get_sql_list_st();

int init_sql_struct(MYSQL *, struct XX_INIT_TBL_LIST* tbl_list, int);
int init_sql(int list_index);
int fillin_struct(MYSQL *mysql, struct XX_INIT_TBL_LIST* tbl_list);


int init_sqlfile();
int free_sqlfile();


int exc_sql(char*);


struct XX_INIT_TBL_LIST* get_opmz_inst_list_st();
struct XX_INIT_TBL_LIST* get_opmz_item_list_st();
struct XX_INIT_TBL_LIST* get_opmz_inst_rule_list_st();


#endif




	int seek_one_opcode_tbl(struct XX_TBL_ONE_OPCODE *tables, int nrows, uchar *first_opcode,int);
	int seek_two_opcode_tbl(struct XX_TBL_TWO_OPCODE *tables, int nrows, uchar *second_opcode, uchar *prefix);
	int seek_three_opcode_tbl(struct XX_TBL_THREE_OPCODE *tables, int nrows, uchar *second_opcode, uchar *third_opcode, \
		int nprefix, uchar *prefix);
	int seek_ext_one_opcode_tbl(struct XX_TBL_EXT_ONE_OPCODE *tables, int nrows, uchar *opcode, \
		uchar *mod, uchar *reg_opcode, uchar *rm);
	int seek_ext_two_opcode_tbl(struct XX_TBL_EXT_TWO_OPCODE *tables, int nrows, uchar *second_opcode, \
		uchar *prefix, uchar *mod, uchar *reg_opcode, uchar *rm);
	int seek_ext_three_opcode_tbl(struct XX_TBL_EXT_THREE_OPCODE *tables, int nrows, uchar *second_opcode, \
		uchar *third_opcode, uchar *prefix, uchar *mod, uchar *reg_opcode, uchar *rm);
	int seek_item_type_tbl(struct XX_TBL_ITEM_TYPE* tbl_item_type, int rows, uchar* type);
	int seek_item_ltype_tbl(struct XX_TBL_ITEM_LTYPE* tbl_item_ltype, int rows, uchar* ltype);
	int seek_fpu_opcode_tbl(struct XX_TBL_FPU_OPCODE* tbl_fpu_opcode, int rows, uchar* sbyte, uchar* smodrm);

	int init_sqlfile();
	int free_sqlfile();

	void* get_sql_struct(int tbl);
	void* get_sql_lstruct(int tbl);
	int get_sql_rows_num(int tbl);
	int get_sql_lrows_num(int tbl);
	int get_sql_item_size(int tbl);
	int get_sql_litem_size(int tbl);
	int get_sql_list_num();

	struct XX_INIT_TBL_LIST* get_sql_list_st();






