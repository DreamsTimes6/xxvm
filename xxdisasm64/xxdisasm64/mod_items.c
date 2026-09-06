#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm64.h"



/////////////////////////////////////
int xxdisasm(char *opcodes);
int query_prefix(char *opcodes);
void xx_inst_print();

//////////////////////////////////////

extern int subprefix(int nprefix,char *prefix,int sub_prefix);
extern int item_const_fix(struct XX_INST *xx_inst, int offset);

int op_items(int nitems,int tbl,struct XX_INST *xx_inst,uchar *codes);
int item_type(char *item_type,int nitem,struct XX_INST *xx_inst,uchar *codes);



typedef int(*PITEM_TYPE_FUNC)(int,struct XX_INST*,uchar*);

typedef struct 
{
	char* 		   item_type;
	int                (*type_func)(int,struct XX_INST*,uchar*);

}ST_TYPE_FUNC;



extern int  item_type_A(int nitem,struct XX_INST *xx_inst,uchar *code);       
extern int  item_type_B(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_C(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_D(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_E(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_F(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_G(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_H(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_I(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_J(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_L(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_M(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_N(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_O(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_P(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_Q(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_R(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_S(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_U(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_V(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_W(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_X(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_Y(int nitem,struct XX_INST *xx_inst,uchar *code);      
                
extern int  item_type_CS(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_DS(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_SS(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_ES(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_FS(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_GS(int nitem,struct XX_INST *xx_inst,uchar *code);      

extern int  item_type_rAX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_rBX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_rCX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_rDX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_rSI(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_rDI(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_rBP(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_rSP(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_eAX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_eBX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_eCX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_eDX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_eSI(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_eDI(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_eBP(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_eSP(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_AX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_BX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_CX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_DX(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_SI(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_DI(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_BP(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_SP(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_AL(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_BL(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_CL(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_DL(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_AH(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_BH(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_CH(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_DH(int nitem,struct XX_INST *xx_inst,uchar *code);      
extern int  item_type_CONST(int nitem,struct XX_INST *xx_inst,uchar *code);      

extern int  item_type_reg(int nitem,struct XX_INST *xx_inst,uchar *code);      


static ST_TYPE_FUNC st_type_func[]=
{
	{"A",item_type_A},
	{"B",item_type_B},
	{"C",item_type_C},
	{"D",item_type_D},
	{"E",item_type_E},
	{"F",item_type_reg},
	{"G",item_type_G},
	{"H",item_type_H},
	{"I",item_type_I},
	{"J",item_type_J},
	{"L",item_type_L},
	{"M",item_type_M},
	{"N",item_type_N},
	{"O",item_type_O},
	{"P",item_type_P},
	{"Q",item_type_Q},
	{"R",item_type_R},
	{"S",item_type_S},
	{"U",item_type_U},
	{"V",item_type_V},
	{"W",item_type_W},
	{"X",item_type_X},
	{"Y",item_type_Y},
	{"CS",item_type_reg},
	{"DS",item_type_reg},
	{"SS",item_type_reg},
	{"ES",item_type_reg},
	{"FS",item_type_reg},
	{"GS",item_type_reg},
	{"rAX",item_type_reg},
	{"rBX",item_type_reg},
	{"rCX",item_type_reg},
	{"rDX",item_type_reg},
	{"rSI",item_type_reg},
	{"rDI",item_type_reg},
	{"rBP",item_type_reg},
	{"rSP",item_type_reg},
	{"eAX",item_type_reg},
	{"eBX",item_type_reg},
	{"eCX",item_type_reg},
	{"eDX",item_type_reg},
	{"eSI",item_type_reg},
	{"eDI",item_type_reg},
	{"eBP",item_type_reg},
	{"eSP",item_type_reg},
	{"AX",item_type_reg},
	{"BX",item_type_reg},
	{"CX",item_type_reg},
	{"DX",item_type_reg},
	{"SI",item_type_reg},
	{"DI",item_type_reg},
	{"BP",item_type_reg},
	{"SP",item_type_reg},
	{"AL",item_type_reg},
	{"BL",item_type_reg},
	{"CL",item_type_reg},
	{"DL",item_type_reg},
	{"AH",item_type_reg},
	{"BH",item_type_reg},
	{"CH",item_type_reg},
	{"DH",item_type_reg},
	{"1",item_type_CONST},
	{"r8",item_type_reg},
	{"r9",item_type_reg},
	{"r10",item_type_reg},
	{"r11",item_type_reg},
	{"r12",item_type_reg},
	{"r13",item_type_reg},
	{"r14",item_type_reg},
	{"r15",item_type_reg},
	{"r8l",item_type_reg},
	{"r9l",item_type_reg},
	{"r10l",item_type_reg},
	{"r11l",item_type_reg},
	{"r12l",item_type_reg},
	{"r13l",item_type_reg},
	{"r14l",item_type_reg},
	{"r15l",item_type_reg},

};

#define  TYPE_FUNC_NUM    sizeof(st_type_func)/sizeof(ST_TYPE_FUNC)

int xx_items_mod(struct XX_INST* xx_inst,uchar* codes)
{
	int nitems=0;
	int n=0;
	int iret=0;




	if(xx_inst->xx_inst_exist.sib_flag==INST_FLAG_NOTOP &&
			xx_inst->xx_inst_exist.items_flag!=INST_FLAG_NOTOP)
	{
		return -2;
	}

	xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_FINISH;

	if(xx_inst->xx_inst_exist.one_opcode_flag==INST_FLAG_EXIST)
	{
		nitems=atoi(xx_inst->xx_inst_table.tbl_one_opcode->op_items);
		if(nitems==0)
		{
			xx_inst->xx_inst_exist.items_flag=INST_FLAG_UNEXIST;
			return 0;
		}
		xx_inst->xx_inst_exist.items_flag=INST_FLAG_EXIST;
		op_items(nitems,TBL_ONE_OPCODE,xx_inst,codes);
	}
	else if(xx_inst->xx_inst_exist.two_opcode_flag==INST_FLAG_EXIST)
	{
		nitems=atoi(xx_inst->xx_inst_table.tbl_two_opcode->op_items);
		if(nitems==0)
		{
			xx_inst->xx_inst_exist.items_flag=INST_FLAG_UNEXIST;
			return 0;
		}
		xx_inst->xx_inst_exist.items_flag=INST_FLAG_EXIST;
		op_items(nitems,TBL_TWO_OPCODE,xx_inst,codes);
	}
	else if(xx_inst->xx_inst_exist.three_opcode_flag==INST_FLAG_EXIST)
	{
		nitems=atoi(xx_inst->xx_inst_table.tbl_three_opcode->op_items);
		if(nitems==0)
		{
			xx_inst->xx_inst_exist.items_flag=INST_FLAG_UNEXIST;
			return 0;
		}
		xx_inst->xx_inst_exist.items_flag=INST_FLAG_EXIST;
		op_items(nitems,TBL_THREE_OPCODE,xx_inst,codes);
	}
	else if(xx_inst->xx_inst_exist.ext_one_opcode_flag==INST_FLAG_EXIST)
	{
		nitems=atoi(xx_inst->xx_inst_table.tbl_ext_one_opcode->op_items);
		if(nitems==0)
		{
			xx_inst->xx_inst_exist.items_flag=INST_FLAG_UNEXIST;
			return 0;
		}
		xx_inst->xx_inst_exist.items_flag=INST_FLAG_EXIST;
		op_items(nitems,TBL_EXT_ONE_OPCODE,xx_inst,codes);
	}
	else if(xx_inst->xx_inst_exist.ext_two_opcode_flag==INST_FLAG_EXIST)
	{
		nitems=atoi(xx_inst->xx_inst_table.tbl_ext_two_opcode->op_items);
		if(nitems==0)
		{
			xx_inst->xx_inst_exist.items_flag=INST_FLAG_UNEXIST;
			return 0;
		}
		xx_inst->xx_inst_exist.items_flag=INST_FLAG_EXIST;
		op_items(nitems,TBL_EXT_TWO_OPCODE,xx_inst,codes);
	}
	else if(xx_inst->xx_inst_exist.ext_three_opcode_flag==INST_FLAG_EXIST)
	{
		nitems=atoi(xx_inst->xx_inst_table.tbl_ext_three_opcode->op_items);
		if(nitems==0)
		{
			xx_inst->xx_inst_exist.items_flag=INST_FLAG_UNEXIST;
			return 0;
		}
		xx_inst->xx_inst_exist.items_flag=INST_FLAG_EXIST;
		op_items(nitems,TBL_EXT_THREE_OPCODE,xx_inst,codes);
	}
	else if(xx_inst->xx_inst_exist.fpu_opcode_flag==INST_FLAG_EXIST)
	{
		nitems=atoi(xx_inst->xx_inst_table.tbl_fpu_opcode->op_items);
		if(nitems==0)
		{
			xx_inst->xx_inst_exist.items_flag=INST_FLAG_UNEXIST;
			return 0;
		}
		xx_inst->xx_inst_exist.items_flag=INST_FLAG_EXIST;
		op_items(nitems,TBL_FPU_OPCODE,xx_inst,codes);
	}
	else
	{
		return -2;
	}
	

	return 0;
}

int op_items(int nitems,int tbl,struct XX_INST *xx_inst,uchar *codes)
{
	int n=0;
	int m=0;
	struct XX_INST_TBL_ITEMS xx_inst_tbl_items[0x10];
	int type_rows;
	int ltype_rows;
	uchar tmp[200];
	struct XX_TBL_ITEM_TYPE *tbl_item_type;
	struct XX_TBL_ITEM_LTYPE *tbl_item_ltype;
	unsigned int tmp_length = 0;


	xx_inst->xx_inst_items.nitem=nitems;

	memset(&xx_inst_tbl_items,0,sizeof(xx_inst_tbl_items));
	switch(tbl)
	{
		case TBL_ONE_OPCODE:
			if(xx_inst->xx_inst_exist.rexb_flag==INST_FLAG_EXIST)
			{
				if(strlen(xx_inst->xx_inst_table.tbl_one_opcode->citem1_type))
				{
					memcpy(xx_inst_tbl_items[0].item_type,
							xx_inst->xx_inst_table.tbl_one_opcode->citem1_type,\
							strlen(xx_inst->xx_inst_table.tbl_one_opcode->citem1_type));
				}
				else
				{
					memcpy(xx_inst_tbl_items[0].item_type,
							xx_inst->xx_inst_table.tbl_one_opcode->item1_type,\
							strlen(xx_inst->xx_inst_table.tbl_one_opcode->item1_type));
				}
				if(strlen(xx_inst->xx_inst_table.tbl_one_opcode->citem2_type))
				{
					memcpy(xx_inst_tbl_items[1].item_type,
							xx_inst->xx_inst_table.tbl_one_opcode->citem2_type,\
							strlen(xx_inst->xx_inst_table.tbl_one_opcode->citem2_type));
				}
				else
				{
					memcpy(xx_inst_tbl_items[1].item_type,
							xx_inst->xx_inst_table.tbl_one_opcode->item2_type,\
							strlen(xx_inst->xx_inst_table.tbl_one_opcode->item2_type));
				}
				if(strlen(xx_inst->xx_inst_table.tbl_one_opcode->citem3_type))
				{
					memcpy(xx_inst_tbl_items[2].item_type,
							xx_inst->xx_inst_table.tbl_one_opcode->citem3_type,\
							strlen(xx_inst->xx_inst_table.tbl_one_opcode->citem3_type));
				}
				else
				{
					memcpy(xx_inst_tbl_items[2].item_type,
							xx_inst->xx_inst_table.tbl_one_opcode->item3_type,\
							strlen(xx_inst->xx_inst_table.tbl_one_opcode->item3_type));
				}
				if(strlen(xx_inst->xx_inst_table.tbl_one_opcode->citem4_type))
				{
					memcpy(xx_inst_tbl_items[3].item_type,
							xx_inst->xx_inst_table.tbl_one_opcode->citem4_type,\
							strlen(xx_inst->xx_inst_table.tbl_one_opcode->citem4_type));
				}
				else
				{
					memcpy(xx_inst_tbl_items[3].item_type,
							xx_inst->xx_inst_table.tbl_one_opcode->item4_type,\
							strlen(xx_inst->xx_inst_table.tbl_one_opcode->item4_type));
				}
				memcpy(xx_inst_tbl_items[0].item_ltype,
						xx_inst->xx_inst_table.tbl_one_opcode->item1_ltype,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item1_ltype));
				memcpy(xx_inst_tbl_items[1].item_ltype,
						xx_inst->xx_inst_table.tbl_one_opcode->item2_ltype,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item2_ltype));
				memcpy(xx_inst_tbl_items[2].item_ltype,
						xx_inst->xx_inst_table.tbl_one_opcode->item3_ltype,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item3_ltype));
				memcpy(xx_inst_tbl_items[3].item_ltype,
						xx_inst->xx_inst_table.tbl_one_opcode->item4_ltype,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item4_ltype));
			}
			else
			{
				memcpy(xx_inst_tbl_items[0].item_type,
						xx_inst->xx_inst_table.tbl_one_opcode->item1_type,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item1_type));
				memcpy(xx_inst_tbl_items[1].item_type,
						xx_inst->xx_inst_table.tbl_one_opcode->item2_type,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item2_type));
				memcpy(xx_inst_tbl_items[2].item_type,
						xx_inst->xx_inst_table.tbl_one_opcode->item3_type,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item3_type));
				memcpy(xx_inst_tbl_items[3].item_type,
						xx_inst->xx_inst_table.tbl_one_opcode->item4_type,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item4_type));
				memcpy(xx_inst_tbl_items[0].item_ltype,
						xx_inst->xx_inst_table.tbl_one_opcode->item1_ltype,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item1_ltype));
				memcpy(xx_inst_tbl_items[1].item_ltype,
						xx_inst->xx_inst_table.tbl_one_opcode->item2_ltype,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item2_ltype));
				memcpy(xx_inst_tbl_items[2].item_ltype,
						xx_inst->xx_inst_table.tbl_one_opcode->item3_ltype,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item3_ltype));
				memcpy(xx_inst_tbl_items[3].item_ltype,
						xx_inst->xx_inst_table.tbl_one_opcode->item4_ltype,\
						strlen(xx_inst->xx_inst_table.tbl_one_opcode->item4_ltype));
			}
			break;
		case TBL_TWO_OPCODE:
			if (xx_inst->xx_inst_exist.rexb_flag == INST_FLAG_EXIST)
			{
				if (strlen(xx_inst->xx_inst_table.tbl_two_opcode->citem1_type))
				{
					memcpy(xx_inst_tbl_items[0].item_type,
						xx_inst->xx_inst_table.tbl_two_opcode->citem1_type, \
						strlen(xx_inst->xx_inst_table.tbl_two_opcode->citem1_type));
				}
				else
				{
					memcpy(xx_inst_tbl_items[0].item_type,
						xx_inst->xx_inst_table.tbl_two_opcode->item1_type, \
						strlen(xx_inst->xx_inst_table.tbl_two_opcode->item1_type));
				}
				if (strlen(xx_inst->xx_inst_table.tbl_two_opcode->citem2_type))
				{
					memcpy(xx_inst_tbl_items[1].item_type,
						xx_inst->xx_inst_table.tbl_two_opcode->citem2_type, \
						strlen(xx_inst->xx_inst_table.tbl_two_opcode->citem2_type));
				}
				else
				{
					memcpy(xx_inst_tbl_items[1].item_type,
						xx_inst->xx_inst_table.tbl_two_opcode->item2_type, \
						strlen(xx_inst->xx_inst_table.tbl_two_opcode->item2_type));
				}
				if (strlen(xx_inst->xx_inst_table.tbl_two_opcode->citem3_type))
				{
					memcpy(xx_inst_tbl_items[2].item_type,
						xx_inst->xx_inst_table.tbl_two_opcode->citem3_type, \
						strlen(xx_inst->xx_inst_table.tbl_two_opcode->citem3_type));
				}
				else
				{
					memcpy(xx_inst_tbl_items[2].item_type,
						xx_inst->xx_inst_table.tbl_two_opcode->item3_type, \
						strlen(xx_inst->xx_inst_table.tbl_two_opcode->item3_type));
				}
				if (strlen(xx_inst->xx_inst_table.tbl_two_opcode->citem4_type))
				{
					memcpy(xx_inst_tbl_items[3].item_type,
						xx_inst->xx_inst_table.tbl_two_opcode->citem4_type, \
						strlen(xx_inst->xx_inst_table.tbl_two_opcode->citem4_type));
				}
				else
				{
					memcpy(xx_inst_tbl_items[3].item_type,
						xx_inst->xx_inst_table.tbl_two_opcode->item4_type, \
						strlen(xx_inst->xx_inst_table.tbl_two_opcode->item4_type));
				}
				memcpy(xx_inst_tbl_items[0].item_ltype,
					xx_inst->xx_inst_table.tbl_two_opcode->item1_ltype, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item1_ltype));
				memcpy(xx_inst_tbl_items[1].item_ltype,
					xx_inst->xx_inst_table.tbl_two_opcode->item2_ltype, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item2_ltype));
				memcpy(xx_inst_tbl_items[2].item_ltype,
					xx_inst->xx_inst_table.tbl_two_opcode->item3_ltype, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item3_ltype));
				memcpy(xx_inst_tbl_items[3].item_ltype,
					xx_inst->xx_inst_table.tbl_two_opcode->item4_ltype, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item4_ltype));
			}
			else
			{
				memcpy(xx_inst_tbl_items[0].item_type,
					xx_inst->xx_inst_table.tbl_two_opcode->item1_type, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item1_type));
				memcpy(xx_inst_tbl_items[1].item_type,
					xx_inst->xx_inst_table.tbl_two_opcode->item2_type, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item2_type));
				memcpy(xx_inst_tbl_items[2].item_type,
					xx_inst->xx_inst_table.tbl_two_opcode->item3_type, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item3_type));
				memcpy(xx_inst_tbl_items[3].item_type,
					xx_inst->xx_inst_table.tbl_two_opcode->item4_type, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item4_type));
				memcpy(xx_inst_tbl_items[0].item_ltype,
					xx_inst->xx_inst_table.tbl_two_opcode->item1_ltype, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item1_ltype));
				memcpy(xx_inst_tbl_items[1].item_ltype,
					xx_inst->xx_inst_table.tbl_two_opcode->item2_ltype, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item2_ltype));
				memcpy(xx_inst_tbl_items[2].item_ltype,
					xx_inst->xx_inst_table.tbl_two_opcode->item3_ltype, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item3_ltype));
				memcpy(xx_inst_tbl_items[3].item_ltype,
					xx_inst->xx_inst_table.tbl_two_opcode->item4_ltype, \
					strlen(xx_inst->xx_inst_table.tbl_two_opcode->item4_ltype));
			}
			break;
		case TBL_THREE_OPCODE:
			memcpy(xx_inst_tbl_items[0].item_type,
					xx_inst->xx_inst_table.tbl_three_opcode->item1_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_three_opcode->item1_type));
			memcpy(xx_inst_tbl_items[1].item_type,
					xx_inst->xx_inst_table.tbl_three_opcode->item2_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_three_opcode->item2_type));
			memcpy(xx_inst_tbl_items[2].item_type,
					xx_inst->xx_inst_table.tbl_three_opcode->item3_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_three_opcode->item3_type));
			memcpy(xx_inst_tbl_items[3].item_type,
					xx_inst->xx_inst_table.tbl_three_opcode->item4_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_three_opcode->item4_type));
			memcpy(xx_inst_tbl_items[0].item_ltype,
					xx_inst->xx_inst_table.tbl_three_opcode->item1_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_three_opcode->item1_ltype));
			memcpy(xx_inst_tbl_items[1].item_ltype,
					xx_inst->xx_inst_table.tbl_three_opcode->item2_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_three_opcode->item2_ltype));
			memcpy(xx_inst_tbl_items[2].item_ltype,
					xx_inst->xx_inst_table.tbl_three_opcode->item3_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_three_opcode->item3_ltype));
			memcpy(xx_inst_tbl_items[3].item_ltype,
					xx_inst->xx_inst_table.tbl_three_opcode->item4_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_three_opcode->item4_ltype));
			break;
		case TBL_EXT_ONE_OPCODE:
			memcpy(xx_inst_tbl_items[0].item_type,
					xx_inst->xx_inst_table.tbl_ext_one_opcode->item1_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->item1_type));
			memcpy(xx_inst_tbl_items[1].item_type,
					xx_inst->xx_inst_table.tbl_ext_one_opcode->item2_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->item2_type));
			memcpy(xx_inst_tbl_items[2].item_type,
					xx_inst->xx_inst_table.tbl_ext_one_opcode->item3_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->item3_type));
			memcpy(xx_inst_tbl_items[3].item_type,
					xx_inst->xx_inst_table.tbl_ext_one_opcode->item4_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->item4_type));
			memcpy(xx_inst_tbl_items[0].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_one_opcode->item1_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->item1_ltype));
			memcpy(xx_inst_tbl_items[1].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_one_opcode->item2_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->item2_ltype));
			memcpy(xx_inst_tbl_items[2].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_one_opcode->item3_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->item3_ltype));
			memcpy(xx_inst_tbl_items[3].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_one_opcode->item4_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_one_opcode->item4_ltype));
			break;
		case TBL_EXT_TWO_OPCODE:
			memcpy(xx_inst_tbl_items[0].item_type,
					xx_inst->xx_inst_table.tbl_ext_two_opcode->item1_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->item1_type));
			memcpy(xx_inst_tbl_items[1].item_type,
					xx_inst->xx_inst_table.tbl_ext_two_opcode->item2_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->item2_type));
			memcpy(xx_inst_tbl_items[2].item_type,
					xx_inst->xx_inst_table.tbl_ext_two_opcode->item3_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->item3_type));
			memcpy(xx_inst_tbl_items[3].item_type,
					xx_inst->xx_inst_table.tbl_ext_two_opcode->item4_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->item4_type));
			memcpy(xx_inst_tbl_items[0].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_two_opcode->item1_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->item1_ltype));
			memcpy(xx_inst_tbl_items[1].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_two_opcode->item2_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->item2_ltype));
			memcpy(xx_inst_tbl_items[2].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_two_opcode->item3_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->item3_ltype));
			memcpy(xx_inst_tbl_items[3].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_two_opcode->item4_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_two_opcode->item4_ltype));
			break;
		case TBL_EXT_THREE_OPCODE:
			memcpy(xx_inst_tbl_items[0].item_type,
					xx_inst->xx_inst_table.tbl_ext_three_opcode->item1_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->item1_type));
			memcpy(xx_inst_tbl_items[1].item_type,
					xx_inst->xx_inst_table.tbl_ext_three_opcode->item2_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->item2_type));
			memcpy(xx_inst_tbl_items[2].item_type,
					xx_inst->xx_inst_table.tbl_ext_three_opcode->item3_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->item3_type));
			memcpy(xx_inst_tbl_items[3].item_type,
					xx_inst->xx_inst_table.tbl_ext_three_opcode->item4_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->item4_type));
			memcpy(xx_inst_tbl_items[0].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_three_opcode->item1_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->item1_ltype));
			memcpy(xx_inst_tbl_items[1].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_three_opcode->item2_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->item2_ltype));
			memcpy(xx_inst_tbl_items[2].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_three_opcode->item3_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->item3_ltype));
			memcpy(xx_inst_tbl_items[3].item_ltype,
					xx_inst->xx_inst_table.tbl_ext_three_opcode->item4_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_ext_three_opcode->item4_ltype));
			break;
		case TBL_FPU_OPCODE:
			memcpy(xx_inst_tbl_items[0].item_type,
					xx_inst->xx_inst_table.tbl_fpu_opcode->item1_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item1_type));
			memcpy(xx_inst_tbl_items[1].item_type,
					xx_inst->xx_inst_table.tbl_fpu_opcode->item2_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item2_type));
			memcpy(xx_inst_tbl_items[2].item_type,
					xx_inst->xx_inst_table.tbl_fpu_opcode->item3_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item3_type));
			memcpy(xx_inst_tbl_items[3].item_type,
					xx_inst->xx_inst_table.tbl_fpu_opcode->item4_type,\
			      		strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item4_type));
			memcpy(xx_inst_tbl_items[0].item_ltype,
					xx_inst->xx_inst_table.tbl_fpu_opcode->item1_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item1_ltype));
			memcpy(xx_inst_tbl_items[1].item_ltype,
					xx_inst->xx_inst_table.tbl_fpu_opcode->item2_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item2_ltype));
			memcpy(xx_inst_tbl_items[2].item_ltype,
					xx_inst->xx_inst_table.tbl_fpu_opcode->item3_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item3_ltype));
			memcpy(xx_inst_tbl_items[3].item_ltype,
					xx_inst->xx_inst_table.tbl_fpu_opcode->item4_ltype,\
			      		strlen(xx_inst->xx_inst_table.tbl_fpu_opcode->item4_ltype));
			break;
		default:
			return -1;
	}


	type_rows=0;
	tbl_item_type=0;
	memset(tmp,0,sizeof(tmp));

	tbl_item_type=get_sql_struct(TBL_ITEM_TYPE);
	if(!tbl_item_type)
	{
		return -1;
	}
	type_rows=get_sql_rows_num(TBL_ITEM_TYPE);
	if(!type_rows)
	{
		return -1;
	}



	ltype_rows=0;
	tbl_item_ltype=0;
	memset(tmp,0,sizeof(tmp));

	tbl_item_ltype=get_sql_struct(TBL_ITEM_LTYPE);
	if(!tbl_item_ltype)
	{
		return -1;
	}
	ltype_rows=get_sql_rows_num(TBL_ITEM_LTYPE);
	if(!ltype_rows)
	{
		return -1;
	}

	for(n=0;n<nitems;n++)
	{
		m=0;
		m=seek_item_type_tbl(tbl_item_type,type_rows,xx_inst_tbl_items[n].item_type);
		if(m<0)
		{
			return -1;
		}
		xx_inst->xx_inst_table.tbl_item_type=&tbl_item_type[m];
		

		m=0;
		m=seek_item_ltype_tbl(tbl_item_ltype,ltype_rows,xx_inst_tbl_items[n].item_ltype);
		if(m<0)
		{
			return -1;
		}
		xx_inst->xx_inst_table.tbl_item_ltype=&tbl_item_ltype[m];

		tmp_length = xx_inst->xx_inst_code.disasm_length;

		item_type(xx_inst_tbl_items[n].item_type,n,\
				xx_inst,codes);

		if (tmp_length < (xx_inst->xx_inst_code.disasm_length) && \
			xx_inst->xx_inst_exist.fix_item == INST_FLAG_EXIST)
		{
			if (xx_inst->xx_inst_code.fix_item < n)
			{
				item_const_fix(xx_inst, xx_inst->xx_inst_code.disasm_length - tmp_length);
			}
		}


		if(xx_inst->xx_inst_items.xx_inst_items_var[n].reg_def_len!=0)
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[n].item_base_def,\
					xx_inst->xx_inst_items.xx_inst_items_var[n].item_reg_def,\
					xx_inst->xx_inst_items.xx_inst_items_var[n].reg_def_len);
			xx_inst->xx_inst_items.xx_inst_items_var[n].base_def_len=xx_inst->xx_inst_items.xx_inst_items_var[n].reg_def_len;
		}
		if(xx_inst->xx_inst_items.xx_inst_items_var[n].base_def_len!=0)
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_var[n].item_reg_def,\
					xx_inst->xx_inst_items.xx_inst_items_var[n].item_base_def,\
					xx_inst->xx_inst_items.xx_inst_items_var[n].base_def_len);
			xx_inst->xx_inst_items.xx_inst_items_var[n].reg_def_len=xx_inst->xx_inst_items.xx_inst_items_var[n].base_def_len;
		}

	}
	return 0;
}


int item_type(char *item_type,int nitem,struct XX_INST *xx_inst,uchar *codes)
{

	int n=0;
	int iret=0;
	int tmp_size = 0;

	
	if(xx_inst->xx_inst_code.inst_system==INST_SYSTEM_16)
	{    
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size=atoi((char*)xx_inst->xx_inst_table.tbl_item_ltype->item16_addr_size);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size=atoi((char*)xx_inst->xx_inst_table.tbl_item_ltype->item16_data_size);
	}
	else if(xx_inst->xx_inst_code.inst_system==INST_SYSTEM_32)
	{
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size=atoi((char*)xx_inst->xx_inst_table.tbl_item_ltype->item32_addr_size);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size=atoi((char*)xx_inst->xx_inst_table.tbl_item_ltype->item32_data_size);
	}
	else if(xx_inst->xx_inst_code.inst_system==INST_SYSTEM_64)
	{
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size=atoi((char*)xx_inst->xx_inst_table.tbl_item_ltype->item64_addr_size);
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size=atoi((char*)xx_inst->xx_inst_table.tbl_item_ltype->item64_data_size);
	}
	else
	{
		return -1;
	}

	if(xx_inst->xx_inst_exist.rexw_flag==INST_FLAG_EXIST && \
			memcmp(xx_inst->xx_inst_table.tbl_item_type->type_mic,"I",1)!=0)
	{
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size=INST_SYSTEM_64;
	}

#if 0
	if(xx_inst->xx_inst_code.inst_system==INST_SYSTEM_64 && xx_inst->xx_inst_exist.one_opcode_flag==INST_FLAG_EXIST)
	{
		if(memcmp(xx_inst->xx_inst_table.tbl_one_opcode->superscripts,SUPS_D64,strlen(SUPS_D64))==0)
		{
			xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size=INST_SYSTEM_64;
		}
	}
#endif

	/*普通指令集以外的指令不使用前缀定义操作项大小*/
	if (xx_inst->xx_inst_code.opcode_type[2] == 0x30)
	{
		if (xx_inst->xx_inst_exist.prefix_flag == INST_FLAG_EXIST)
		{
			if (subprefix(xx_inst->xx_inst_code.nprefix, xx_inst->xx_inst_code.prefix, 0x66) == 1)
			{
				if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size == INST_SYSTEM_32)
				{
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = INST_SYSTEM_16;
				}
				else if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size == INST_SYSTEM_16)
				{
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = INST_SYSTEM_32;
				}
				else
				{
				}
			}
			else if (subprefix(xx_inst->xx_inst_code.nprefix, xx_inst->xx_inst_code.prefix, 0x67) == 1)
			{
				if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size == INST_SYSTEM_32)
				{
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size = INST_SYSTEM_16;
				}
				else if (xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size == INST_SYSTEM_16)
				{
					xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_addr_size = INST_SYSTEM_32;
				}
				else
				{
				}
			}
		}
	}

	tmp_size = atoi(xx_inst->xx_inst_table.tbl_item_type->type_size);
	if (tmp_size)
	{
		xx_inst->xx_inst_items.xx_inst_items_var[nitem].item_data_size = tmp_size;
	}
	
	if(xx_inst->xx_inst_exist.modrm_flag!=atoi(xx_inst->xx_inst_table.tbl_item_type->modrm_exist))
	{
	}
	xx_inst->xx_inst_exist.modrm_flag=atoi(xx_inst->xx_inst_table.tbl_item_type->modrm_exist);
	if(xx_inst->xx_inst_exist.sib_flag!=atoi(xx_inst->xx_inst_table.tbl_item_type->sib_exist))
	{
	}
	xx_inst->xx_inst_exist.sib_flag=atoi(xx_inst->xx_inst_table.tbl_item_type->sib_exist);


	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type=atoi(xx_inst->xx_inst_table.tbl_item_type->item_type);
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_modrm_select=atoi(xx_inst->xx_inst_table.tbl_item_type->modrm_select);
	xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_reg_flag=atoi(xx_inst->xx_inst_table.tbl_item_type->reg_type);


	iret=(st_type_func[atoi(xx_inst->xx_inst_table.tbl_item_type->index_func)].type_func)(nitem,xx_inst,codes);
	if(iret)
	{
		return -1;
	}


	if(xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type==INST_ITEM_TYPE_MREG || \
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type==INST_ITEM_TYPE_MIMM || \
		xx_inst->xx_inst_items.xx_inst_items_flag[nitem].item_type==INST_ITEM_TYPE_MREGDIS )
	{
		if(subprefix(xx_inst->xx_inst_code.nprefix,xx_inst->xx_inst_code.prefix,0x2e)==1)
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_prefix,PREFIX_CS,strlen(PREFIX_CS));
		}
		else if(subprefix(xx_inst->xx_inst_code.nprefix,xx_inst->xx_inst_code.prefix,0x36)==1)
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_prefix,PREFIX_SS,strlen(PREFIX_SS));
		}
		else if(subprefix(xx_inst->xx_inst_code.nprefix,xx_inst->xx_inst_code.prefix,0x3e)==1)
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_prefix,PREFIX_DS,strlen(PREFIX_DS));
		}
		else if(subprefix(xx_inst->xx_inst_code.nprefix,xx_inst->xx_inst_code.prefix,0x26)==1)
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_prefix,PREFIX_ES,strlen(PREFIX_ES));
		}
		else if(subprefix(xx_inst->xx_inst_code.nprefix,xx_inst->xx_inst_code.prefix,0x64)==1)
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_prefix,PREFIX_FS,strlen(PREFIX_FS));
		}
		else if(subprefix(xx_inst->xx_inst_code.nprefix,xx_inst->xx_inst_code.prefix,0x65)==1)
		{
			memcpy(xx_inst->xx_inst_items.xx_inst_items_mic[nitem].item_prefix,PREFIX_GS,strlen(PREFIX_GS));
		}
		else
		{
		}
	}

	return 0;

}

