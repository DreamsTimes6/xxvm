#include <stdio.h>
#include <string.h>

#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm64.h"




/////////////////////////////////////
int xxdisasm(char *opcodes);
void xx_inst_print();

/////////////////////////////////////


int xx_begin_check_mod(struct XX_INST* xx_inst,uchar* codes)
{
	if(xx_inst->xx_inst_code.inst_system==INST_SYSTEM_UNEXIST)
	{
		return -1;
	}
	
	xx_inst->xx_inst_exist.next_op_flag=INST_NEXTOP_PREFIX;
	return 0;
}













