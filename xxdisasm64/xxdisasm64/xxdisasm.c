#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm64.h"


/////////////////////////////////////
void xx_inst_print();
void xx_disasm_mod_dispatch(struct XX_INST* xx_inst,uchar *codes);
int disasm_log_init();


extern int xx_begin_check_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_prefix_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_one_opcode_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_two_opcode_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_three_opcode_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_ext_one_opcode_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_ext_two_opcode_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_ext_three_opcode_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_modrm_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_sib_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_items_mod(struct XX_INST* xx_inst,uchar* codes);
extern int xx_finish_mod(struct XX_INST* xx_inst,uchar* codes);

extern int xx_rex_mod(struct XX_INST* xx_inst,uchar* codes);

extern void  print_inst_code(struct XX_INST* xx_inst);
extern void  print_inst_exist(struct XX_INST* xx_inst);
extern void  print_inst_item(struct XX_INST* xx_inst);
extern void  print_inst_mic(struct XX_INST* xx_inst);
/////////////////////////////////////


 DLL_PUBLIC void xx_disasm(struct XX_INST* xx_inst, uchar *codes)
{
	int iret;

	iret = init_sqlfile();
	if (iret)
	{
		return;
	}

	xx_disasm_mod_dispatch(xx_inst, codes);

	return;

};

_export void xx_disasm_free()
{
	free_sqlfile();
};


void xx_disasm_mod_dispatch(struct XX_INST* xx_inst,uchar *codes)
{
	int iret=0;

	iret=xx_begin_check_mod(xx_inst,codes);
	if(iret)
	{
		return;
	}
	

	while(1)
	{
		switch(xx_inst->xx_inst_exist.next_op_flag)
		{
			case INST_NEXTOP_PREFIX:
				iret=xx_prefix_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_REX:
				iret=xx_rex_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_ONE_OPCODE:
				iret=xx_one_opcode_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_TWO_OPCODE:
				iret=xx_two_opcode_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_THREE_OPCODE:
				iret=xx_three_opcode_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_EXT_ONE_OPCODE:
				iret=xx_ext_one_opcode_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_EXT_TWO_OPCODE:
				iret=xx_ext_two_opcode_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_EXT_THREE_OPCODE:
				iret=xx_ext_three_opcode_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_MODRM:
				iret=xx_modrm_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_SIB:
				iret=xx_sib_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			case INST_NEXTOP_ITEMS:
				iret=xx_items_mod(xx_inst,codes);
				if(iret)
				{
					xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
					return;
				}
				break;
			default:
				xx_inst->xx_inst_exist.finish_flag=INST_FLAG_UNEXIST;
				return;
		}
		if(xx_inst->xx_inst_exist.next_op_flag==INST_NEXTOP_FINISH)
		{
			xx_inst->xx_inst_exist.finish_flag=INST_FLAG_EXIST;
			xx_finish_mod(xx_inst,codes);
			break;
		}
	}
}







