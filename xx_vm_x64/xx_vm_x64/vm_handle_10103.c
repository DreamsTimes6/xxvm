
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


extern char opmz_dfile[];  //优化数据文件名


int vm_handle_10103(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int outs_size, char *out_var)
{
	int n = 0;
	int iret = 0;
	int f_cnd = 0;
	int s_cnd = 0;
	int t_cnd = 0;

	char t0[20];
	int index = 0;
	char tmp[100];

	struct ST_VM_ITEM_DATA item_data;



	memset(&item_data, 0, sizeof(item_data));
#if 0
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "0103\r\n");
	xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
	//_Addtolist(0,1,"vm_handle_0103");
	memset(outs, 0, outs_size);
	/*mov t0,[regbase+index]*/
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_mov, sop_len) == 0)
		{
			/*检查第一个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_MREGI &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size == INST_ITEM_SIZE_64)
				{
					/*检查第二个操作项寄存器*/
					if (xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def, vm_regbase) == 0)
					{
						if (xx_inst[n].xx_inst_items.xx_inst_items_var[1].index_def_len != 0)
						{
							memset(t0, 0, sizeof(t0));
							memcpy(t0, xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, \
								xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);

							//memset(tmp,0,sizeof(tmp));
							//xx_get_reg_mic(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_index_def,tmp);
							//sprintf(outs+strlen(outs),"index_%s=",tmp);
							index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_index_def);
							sprintf(outs + strlen(outs), "index=%llx  ", xx_contect[n].r[index]);

							/*out_var，常数或者寄存器*/
							memcpy(out_var, &(xx_contect[n].r[index]), sizeof(xx_contect[n].r[index]));

							//sprintf(outs+strlen(outs),"  ");

						

							//_Addtolist(0,1,"first condition inst_num:[%d]",n);
							f_cnd = 1;
							break;
						}
					}
				}
			}
		}
	}
	if (f_cnd == 0)
	{
		outs[0] = 0;
#if 0
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "1 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}


	/*sub sp,4;lea sp,[sp+fc]*/
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_sub, sop_len) == 0)
		{
			/*检查第一个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, vm_sp) == 0)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_IMM)
				{
					/*检查第二个操作项寄存器*/
					if (xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0] == 0x08)
					{
						//_Addtolist(0,1,"second condition inst_num:[%d]",n);
						s_cnd = 1;
					}
				}
			}
		}
	}
	for (n = 0; n < num; n++)
	{
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_lea, sop_len) == 0)
		{
			/*检查第一个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def, vm_sp) == 0)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_MREGDIS)
				{
					/*检查第二个操作项寄存器*/
					if (xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def, vm_sp) == 0)
					{
						if (xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0] == 0xf8)
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
		outs[0] = 0;
#if 0
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "2 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}

	/*mov [sp+00],t0*/
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_mov, sop_len) == 0)
		{
			/*检查第一个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_MREGDIS &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def, vm_sp) == 0 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_const[0] == 0x00 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size == INST_ITEM_SIZE_64)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_REG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def, t0) == 0)
				{
					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg_mic(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,tmp);
					//sprintf(outs+strlen(outs),"%s=",tmp);
					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					sprintf(outs + strlen(outs), "%llx  ", xx_contect[n].r[index]);

					//sprintf(outs+strlen(outs),"  ");

					//_Addtolist(0,1,"third condition inst_num:[%d]",n);
					t_cnd = 1;

					item_data.item_num = 1;
					memcpy(item_data.flag_start, &xx_contect[n].eflag, sizeof(xx_contect[n].eflag));
					memcpy(item_data.flag_end, &xx_contect[n + 1].eflag, sizeof(xx_contect[n + 1].eflag));

					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					memcpy(item_data.var[0], &xx_contect[n].r[index], sizeof(xx_contect[n].r[index]));

					memcpy(item_data.result, &xx_contect[n].r[REG_RSP], sizeof(xx_contect[n].r[REG_RSP]));

					break;
				}
			}
			else if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_MREG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def, vm_sp) == 0 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size == INST_ITEM_SIZE_64)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_REG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def, t0) == 0)
				{
					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg_mic(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,tmp);
					//sprintf(outs+strlen(outs),"%s=",tmp);
					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					sprintf(outs + strlen(outs), "%llx  ", xx_contect[n].r[index]);

					//sprintf(outs+strlen(outs),"  ");

					//_Addtolist(0,1,"third condition inst_num:[%d]",n);
					t_cnd = 1;

					item_data.item_num = 1;
					memcpy(item_data.flag_start, &xx_contect[n].eflag, sizeof(xx_contect[n].eflag));
					memcpy(item_data.flag_end, &xx_contect[n + 1].eflag, sizeof(xx_contect[n + 1].eflag));

					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					memcpy(item_data.var[0], &xx_contect[n].r[index], sizeof(xx_contect[n].r[index]));

					memcpy(item_data.result, &xx_contect[n].r[REG_RSP], sizeof(xx_contect[n].r[REG_RSP]));

					break;
				}
			}
		}
	}
	if (t_cnd == 0)
	{
		outs[0] = 0;
#if 0
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "3 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}


#if 0
	//_Addtolist(0,1,"regbase_index:[%08x]",push_index/4);
	//_Addtolist(0,1,"reg_status:[%08x]",reg_status_regbase.reg_status[push_index/4]);
	if (reg_status_regbase.reg_status[push_index / 4] == 1)
	{
		if (xreg_cmp(reg_status_regbase.reg_def[push_index / 4], xreg_unknow) != 0)
		{
			memset(tmp, 0, sizeof(tmp));
			xx_get_reg_mic(reg_status_regbase.reg_def[push_index / 4], tmp);
			sprintf(outs + strlen(outs), "%s", tmp);
			//_Addtolist(0,1,"regbase_index:[%s]",tmp);
		}
	}
	/*处理寄存器索引*/
	/*根据索引从堆栈中找到寄存器定义*/
	for (n = 0; n < reg_status_stack.push_num; n++)
	{
		memcpy(reg_status_stack.reg_def[reg_status_stack.push_num - n], \
			reg_status_stack.reg_def[reg_status_stack.push_num - n - 1], xreg_len);
		reg_status_stack.reg_index[reg_status_stack.push_num - n] = reg_status_stack.reg_index[reg_status_stack.push_num - n - 1];
	}
	memcpy(reg_status_stack.reg_def[0], \
		reg_status_regbase.reg_def[push_index / 4], xreg_len);
	reg_status_stack.reg_index[0] = push_index;
	reg_status_stack.reg_status[0] = 1;
	reg_status_stack.push_num++;
#endif

	iret = xx_opmz_item_write(opmz_dfile, &item_data);
	if (iret == 0)
	{
		return -1;
	}
	return 0x10103;
}



















