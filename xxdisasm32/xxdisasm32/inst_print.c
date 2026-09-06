#include <string.h>
#include <stdlib.h>
#include "xx_inst.h"
#include "xx_err.h"
#include "xx_comm32.h"
//////////////////////////////////

extern ulong g_count ;
extern struct XX_INST *g_inst ;
extern ulong err_code ;


extern void xx_disasm_free();
extern void xx_disasm(struct XX_INST* xx_inst, uchar *codes);


DLL_PUBLIC int  print_inst_code(ulong seq)
{

	uchar tmp[200];
	struct XX_INST *xx_inst = 0;

	if (g_count == 0 || g_inst == 0)
	{
		err_code = INST_ERR_UNINIT;
		return 0;
	}
	if (seq >= g_count)
	{
		err_code = INST_ERR_SEQ;
		return 0;
	}
	xx_inst = &g_inst[seq];
	if (xx_inst == 0)
	{
		err_code = INST_ERR_SEQ;
		return 0;
	}

	printf("*******************************************\n");
	printf("*********      [xx_inst_code]      ********\n");

	printf("xx_inst->xx_inst_code.nprefix:[%0x]\n",\
			xx_inst->xx_inst_code.nprefix);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.prefix,\
			sizeof(xx_inst->xx_inst_code.prefix),\
			tmp," ");
	printf("xx_inst->xx_inst_code.prefix:[%s]\n",tmp);
	printf("xx_inst->xx_inst_code.prefix_len:[%0x]\n",xx_inst->xx_inst_code.prefix_len);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.opcode_type,\
			sizeof(xx_inst->xx_inst_code.opcode_type),\
			tmp," ");
	printf("xx_inst->xx_inst_code.opcode_type:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.first_opcode,\
			sizeof(xx_inst->xx_inst_code.first_opcode),\
			tmp," ");
	printf("xx_inst->xx_inst_code.first_opcode:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.second_opcode,\
			sizeof(xx_inst->xx_inst_code.second_opcode),\
			tmp," ");
	printf("xx_inst->xx_inst_code.second_opcode:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.third_opcode,\
			sizeof(xx_inst->xx_inst_code.third_opcode),\
			tmp," ");
	printf("xx_inst->xx_inst_code.third_opcode:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.ext_first_opcode,\
			sizeof(xx_inst->xx_inst_code.ext_first_opcode),\
			tmp," ");
	printf("xx_inst->xx_inst_code.ext_first_opcode:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.ext_second_opcode,\
			sizeof(xx_inst->xx_inst_code.ext_second_opcode),\
			tmp," ");
	printf("xx_inst->xx_inst_code.ext_second_opcode:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.ext_third_opcode,\
			sizeof(xx_inst->xx_inst_code.ext_third_opcode),\
			tmp," ");
	printf("xx_inst->xx_inst_code.ext_third_opcode:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.modrm,\
			sizeof(xx_inst->xx_inst_code.modrm),\
			tmp," ");
	printf("xx_inst->xx_inst_code.modrm:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.sib,\
			sizeof(xx_inst->xx_inst_code.sib),\
			tmp," ");
	printf("xx_inst->xx_inst_code.sib:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.modify_address,\
			sizeof(xx_inst->xx_inst_code.modify_address),\
			tmp," ");
	printf("xx_inst->xx_inst_code.modify_address:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.direct_address,\
			sizeof(xx_inst->xx_inst_code.direct_address),\
			tmp," ");
	printf("xx_inst->xx_inst_code.direct_address:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.displacement,\
			sizeof(xx_inst->xx_inst_code.displacement),\
			tmp," ");
	printf("xx_inst->xx_inst_code.displacement:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.immediate,\
			sizeof(xx_inst->xx_inst_code.immediate),\
			tmp," ");
	printf("xx_inst->xx_inst_code.immediate:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.base,\
			sizeof(xx_inst->xx_inst_code.base),\
			tmp," ");
	printf("xx_inst->xx_inst_code.base:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.current_addr,\
			sizeof(xx_inst->xx_inst_code.current_addr),\
			tmp," ");
	printf("xx_inst->xx_inst_code.current_addr:[%s]\n",tmp);
	memset(tmp,0,sizeof(tmp));
	nhex_str(xx_inst->xx_inst_code.disasm,\
			sizeof(xx_inst->xx_inst_code.disasm),\
			tmp," ");
	printf("xx_inst->xx_inst_code.disasm:[%s]\n",tmp);

	printf("xx_inst->xx_inst_code.disasm_length:[%0x]\n",\
			xx_inst->xx_inst_code.disasm_length);
	printf("xx_inst->xx_inst_code.inst_system:[%0x]\n",\
			xx_inst->xx_inst_code.inst_system);
	printf("*******************************************\n");
	printf("                                           \n");
	err_code = INST_ERR_SUCCESS;
	return 1;
}

DLL_PUBLIC int  print_inst_exist(ulong seq)
{
	uchar tmp[200];
	struct XX_INST *xx_inst = 0;

	if (g_count == 0 || g_inst == 0)
	{
		err_code = INST_ERR_UNINIT;
		return 0;
	}
	if (seq >= g_count)
	{
		err_code = INST_ERR_SEQ;
		return 0;
	}
	xx_inst = &g_inst[seq];
	if (xx_inst == 0)
	{
		err_code = INST_ERR_SEQ;
		return 0;
	}


	printf("*******************************************\n");
	printf("*********      [xx_inst_exist]      *******\n");
	printf("xx_inst->xx_inst_exist.prefix_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.prefix_flag);
	printf("xx_inst->xx_inst_exist.next_op_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.next_op_flag);
	printf("xx_inst->xx_inst_exist.opcode_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.opcode_flag);
	printf("xx_inst->xx_inst_exist.one_opcode_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.one_opcode_flag);
	printf("xx_inst->xx_inst_exist.two_opcode_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.two_opcode_flag);
	printf("xx_inst->xx_inst_exist.three_opcode_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.three_opcode_flag);
	printf("xx_inst->xx_inst_exist.ext_one_opcode_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.ext_one_opcode_flag);
	printf("xx_inst->xx_inst_exist.ext_two_opcode_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.ext_two_opcode_flag);
	printf("xx_inst->xx_inst_exist.ext_three_opcode_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.ext_three_opcode_flag);
	printf("xx_inst->xx_inst_exist.modrm_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.modrm_flag);
	printf("xx_inst->xx_inst_exist.sib_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.sib_flag);
	printf("xx_inst->xx_inst_exist.modify_address_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.modify_address_flag);
	printf("xx_inst->xx_inst_exist.items_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.items_flag);
	printf("xx_inst->xx_inst_exist.finish_flag:[%0x]\n",\
			xx_inst->xx_inst_exist.finish_flag);
	printf("*******************************************\n");
	printf("                                           \n");
	err_code = INST_ERR_SUCCESS;
	return 1;
}

DLL_PUBLIC int print_inst_mic(ulong seq)
{
	int n;
	uchar tmp[200];
	struct XX_INST *xx_inst = 0;

	if (g_count == 0 || g_inst == 0)
	{
		err_code = INST_ERR_UNINIT;
		return 0;
	}
	if (seq >= g_count)
	{
		err_code = INST_ERR_SEQ;
		return 0;
	}
	xx_inst = &g_inst[seq];
	if (xx_inst == 0)
	{
		err_code = INST_ERR_SEQ;
		return 0;
	}

	printf("*******************************************\n");
	printf("*********       [xx_inst_mic]       *******\n");
	printf("xx_inst->xx_inst_mic.prefix:[%s]\n",\
			xx_inst->xx_inst_mic.prefix);
	printf("xx_inst->xx_inst_mic.opcode_mic:[%s]\n",\
			xx_inst->xx_inst_mic.opcode_mic);
	printf("xx_inst->xx_inst_mic.disasm:[%s]\n",\
			xx_inst->xx_inst_mic.disasm);
	printf("*******************************************\n");
	printf("                                           \n");
	err_code = INST_ERR_SUCCESS;
	return 1;
}


DLL_PUBLIC int print_inst_item(ulong seq)
{
	int n;
	uchar tmp[200];
	struct XX_INST *xx_inst = 0;

	if (g_count == 0 || g_inst == 0)
	{
		err_code = INST_ERR_UNINIT;
		return 0;
	}
	if (seq >= g_count)
	{
		err_code = INST_ERR_SEQ;
		return 0;
	}
	xx_inst = &g_inst[seq];
	if (xx_inst == 0)
	{
		err_code = INST_ERR_SEQ;
		return 0;
	}


	printf("*******************************************\n");
	printf("**********[xx_inst_item] total:[%0x]*******\n",\
			xx_inst->xx_inst_items.nitem);
	for(n=0;n<xx_inst->xx_inst_items.nitem;n++)
	{
		printf("*******************************************\n");
		printf("*********   [xx_inst_flag] [%0x]     *******\n",n);
		printf("xx_inst->xx_inst_items.xx_inst_items_flag[%0x].item_type:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_flag[n].item_type);
		printf("xx_inst->xx_inst_items.xx_inst_items_flag[%0x].item_modrm_select:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_flag[n].item_modrm_select);
		printf("xx_inst->xx_inst_items.xx_inst_items_flag[%0x].item_op_flag:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_flag[n].item_op_flag);
		printf("xx_inst->xx_inst_items.xx_inst_items_flag[%0x].item_reg_flag:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_flag[n].item_reg_flag);
		printf("xx_inst->xx_inst_items.xx_inst_items_flag[%0x].item_seg_flag:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_flag[n].item_seg_flag);
		printf("xx_inst->xx_inst_items.xx_inst_items_flag[%0x].iaddress_flag:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_flag[n].iaddress_flag);
		printf("xx_inst->xx_inst_items.xx_inst_items_flag[%0x].maddress_flag:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_flag[n].maddress_flag);
		printf("xx_inst->xx_inst_items.xx_inst_items_flag[%0x].immediate_flag:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_flag[n].immediate_flag);
		printf("xx_inst->xx_inst_items.xx_inst_items_flag[%0x].displacement_flag:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_flag[n].displacement_flag);
		printf("*******************************************\n");
		printf("                                           \n");



		printf("*******************************************\n");
		printf("*********   [xx_inst_var] [%0x]      *******\n",n);
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].item_addr_size:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_var[n].item_addr_size);
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].item_data_size:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_var[n].item_data_size);
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].item_seg_size:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_var[n].item_seg_size);
		memset(tmp,0,sizeof(tmp));
		nhex_str(xx_inst->xx_inst_items.xx_inst_items_var[n].item_reg_def,\
				sizeof(xx_inst->xx_inst_items.xx_inst_items_var[n].item_reg_def),\
				tmp," ");
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].item_reg_def:[%s]\n",n,tmp);
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].reg_def_len:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_var[n].reg_def_len);
		memset(tmp,0,sizeof(tmp));
		nhex_str(xx_inst->xx_inst_items.xx_inst_items_var[n].item_base_def,\
				sizeof(xx_inst->xx_inst_items.xx_inst_items_var[n].item_base_def),\
				tmp," ");
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].item_base_def:[%s]\n",n,tmp);
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].base_def_len:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_var[n].base_def_len);
		memset(tmp,0,sizeof(tmp));
		nhex_str(xx_inst->xx_inst_items.xx_inst_items_var[n].item_index_def,\
				sizeof(xx_inst->xx_inst_items.xx_inst_items_var[n].item_index_def),\
				tmp," ");
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].item_index_def:[%s]\n",n,tmp);
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].index_def_len:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_var[n].index_def_len);
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].index_scale:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_var[n].index_scale);
		memset(tmp,0,sizeof(tmp));
		nhex_str(xx_inst->xx_inst_items.xx_inst_items_var[n].item_segment,\
				sizeof(xx_inst->xx_inst_items.xx_inst_items_var[n].item_segment),\
				tmp," ");
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].item_segment:[%s]\n",n,tmp);
		memset(tmp,0,sizeof(tmp));
		nhex_str(xx_inst->xx_inst_items.xx_inst_items_var[n].item_const,\
				sizeof(xx_inst->xx_inst_items.xx_inst_items_var[n].item_const),\
				tmp," ");
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].item_const:[%s]\n",n,tmp);
		printf("xx_inst->xx_inst_items.xx_inst_items_var[%0x].const_size:[%0x]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_var[n].const_size);
		printf("*******************************************\n");
		printf("                                           \n");



		printf("*******************************************\n");
		printf("*********   [xx_inst_mic] [%0x]      *******\n",n);
		printf("xx_inst->xx_inst_items.xx_inst_items_mic[%0x].item_prefix:[%s]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_mic[n].item_prefix);
		printf("xx_inst->xx_inst_items.xx_inst_items_mic[%0x].item_segment:[%s]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_mic[n].item_segment);
		printf("xx_inst->xx_inst_items.xx_inst_items_mic[%0x].item_self_mic:[%s]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_mic[n].item_self_mic);
		printf("xx_inst->xx_inst_items.xx_inst_items_mic[%0x].item_mic:[%s]\n",\
				n,xx_inst->xx_inst_items.xx_inst_items_mic[n].item_mic);
		printf("*******************************************\n");
		printf("                                           \n");

	}
	printf("*******************************************\n");
	printf("                                           \n");
	err_code = INST_ERR_SUCCESS;
	return 1;
}




