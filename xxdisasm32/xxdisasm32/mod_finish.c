#include <stdio.h>
#include <string.h>

#include "g_sql.h"
#include "xx_inst.h"
#include "xx_comm32.h"




int xx_finish_mod(struct XX_INST* xx_inst,uchar* codes)
{
	char *space=" ";
	char *point=",";
	int n=0;
	int offset=0;


	memcpy(xx_inst->xx_inst_mic.disasm+offset,xx_inst->xx_inst_mic.opcode_mic,strlen(xx_inst->xx_inst_mic.opcode_mic));	
	offset=offset+strlen(xx_inst->xx_inst_mic.opcode_mic);
	memcpy(xx_inst->xx_inst_mic.disasm+offset,space,strlen(space));	
	offset=offset+strlen(space);

	while(1)
	{
		memcpy(xx_inst->xx_inst_mic.disasm+offset,xx_inst->xx_inst_items.xx_inst_items_mic[n].item_self_mic,\
				strlen(xx_inst->xx_inst_items.xx_inst_items_mic[n].item_self_mic));	
		offset=offset+strlen(xx_inst->xx_inst_items.xx_inst_items_mic[n].item_self_mic);
		if(n>=(xx_inst->xx_inst_items.nitem-1))
		{
			break;
		}
		memcpy(xx_inst->xx_inst_mic.disasm+offset,point,strlen(point));	
		offset=offset+strlen(point);
		n++;
	}
	return 0;
}

