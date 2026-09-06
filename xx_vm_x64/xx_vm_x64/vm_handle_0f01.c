
#include "stdio.h"
#include "xx_vm.h"
#include "xx_comm64.h"
#include "xx_opdef3.h"
#include "xxdisasm64.h"
#include "xx_opmz.h"

extern char xreg_unknow[];
extern char xreg_rax[];
extern char xreg_rbx[];
extern char xreg_rcx[];
extern char xreg_rdx[];
extern char xreg_rbp[];
extern char xreg_rsp[];
extern char xreg_rsi[];
extern char xreg_rdi[];

extern char xreg_eax[];
extern char xreg_ebx[];
extern char xreg_ecx[];
extern char xreg_edx[];
extern char xreg_ebp[];
extern char xreg_esp[];
extern char xreg_esi[];
extern char xreg_edi[];


extern char xreg_r8[];
extern char xreg_r9[];
extern char xreg_r10[];
extern char xreg_r11[];
extern char xreg_r12[];
extern char xreg_r13[];
extern char xreg_r14[];
extern char xreg_r15[];


extern char xreg_r8d[];
extern char xreg_r9d[];
extern char xreg_r10d[];
extern char xreg_r11d[];
extern char xreg_r12d[];
extern char xreg_r13d[];
extern char xreg_r14d[];
extern char xreg_r15d[];


extern char log_file[];

extern char vm_sp[];
extern char vm_regbase[];
extern char vm_opcode[];
extern char vm_ip[];
extern char vm_key[];

extern char vm_osp[];
extern char vm_oregbase[];
extern char vm_oopcode[];
extern char vm_oip[];
extern char vm_okey[];


extern int xreg_cmp(char* buf1, char* buf2);
extern int xreg_incmp(char* buf1, char* buf2);

//#define DEBUG2_LOG


extern char opmz_dfile[];  //优化数据文件名

int vm_handle_0f01(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int outs_size, char *out_var)
{
	int n = 0;
	int iret = 0;
	int f_cnd = 0;
	int s_cnd = 0;
	int t_cnd = 0;
	int o_cnd = 0;
	int sp_cnd = 0;
	int ip_cnd = 0;
	int opcode_cnd = 0;




	int index = 0;

	struct ST_VM_ITEM_DATA item_data;



	memset(&item_data, 0, sizeof(item_data));
#ifdef DEBUG2_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "0f01\r\n");
	xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
	//_Addtolist(0,1,"vm_shandle_0f01");
	memset(outs, 0, outs_size);



	/*mov dt0,[sp+00]*/
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_mov, sop_len) == 0)
		{

			/*检查第二个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
			{
				/*检查第一个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_MREGDIS &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def, vm_osp) == 0 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0] == 0x00)
				{
					//_Addtolist(0,1,"first condition inst_num:[%d]",n);
					f_cnd = 1;
				}
				else if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_MREG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def, vm_osp) == 0)
				{

					//_Addtolist(0,1,"first condition inst_num:[%d]",n);
					f_cnd = 1;
				}
			}
		}
	}
	if (f_cnd == 0)
	{
		outs[0] = 0;
#ifdef DEBUG2_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "1 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}

	/*add sp,4 */
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_add, sop_len) == 0)
		{
			/*检查第一个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, vm_osp) == 0)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_IMM)
				{
					/*检查第二个操作项寄存器*/
					if (xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0] == (uchar)0x08)
					{
						//_Addtolist(0,1,"second condition inst_num:[%d]",n);
						s_cnd = 1;
					}
					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg_mic(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,tmp);
					//sprintf(outs+strlen(outs),"%s=",tmp);

				}
			}
		}
	}

	/*lea sp,[sp+4]*/
	for (n = 0; n < num; n++)
	{
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_lea, sop_len) == 0)
		{
			/*检查第一个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, vm_osp) == 0)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_MREGDIS)
				{
					/*检查第二个操作项寄存器*/
					if (xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def, vm_osp) == 0)
					{
						if (xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0] == 0x08)
						{
							//_Addtolist(0,1,"second condition inst_num:[%d]",n);
							s_cnd = 1;
						}

					}
				}
			}
		}
	}
	if (s_cnd == 0)
	{
		//outs[0]=0;
#ifdef DEBUG2_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "1 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}

#if 0
	/*mov vm_key,vm_opcode*/
	opcode_cnd = 0;
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_mov, sop_len) == 0)
		{
			/*检查第二个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def, vm_okey) == 0)
			{
				/*检查第一个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_REG)
				{

					//_Addtolist(0,1,"other condition inst_num:[%d]",n);
					opcode_cnd = 1;
				}
			}
		}
	}
	if (opcode_cnd == 0)
	{
		outs[0] = 0;
#ifdef DEBUG2_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "3 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}
#endif


	/*mov dt0,0 暂时不判断*/
	/*lea vm_ip,d[dconst]*/
	ip_cnd = 0;
	for (n = 0; n < num; n++)
	{
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_lea, sop_len) == 0)
		{
			/*检查第一个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_MDIS)
				{
					/*检查第二个操作项寄存器*/
					if (memcmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const, \
						xx_inst[n].xx_inst_code.current_addr, xx_inst[n].xx_inst_code.inst_system) == 0)
					{

						//_Addtolist(0,1,"other condition inst_num:[%d]",n);
						ip_cnd = 1;
					}
				}
			}
		}
	}
	if (ip_cnd == 0)
	{
		outs[0] = 0;
#ifdef DEBUG2_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "4 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}

	iret = xx_opmz_item_write(opmz_dfile, &item_data);
	if (iret == 0)
	{
		return -1;
	}

	return 0x0f01;
}



















