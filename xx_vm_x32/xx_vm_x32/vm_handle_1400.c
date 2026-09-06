
#include "stdio.h"
#include "xx_vm.h"
#include "xx_comm32.h"
#include "xx_opdef3.h"
#include "xxdisasm32.h"
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int vm_handle_1400(struct XX_INST *xx_inst, int num, struct XX_CONTEXT *xx_contect, char *outs, int outs_size, char *out_var)
{
	int n = 0;
	int iret = 0;
	int f_cnd = 0;
	int s_cnd = 0;
	int t_cnd = 0;

	char t0[20];
	int index = 0;
	char tmp[100];
	int call_index = 0;
	unsigned int var = 0;
	int argc = 0;
	struct ST_VM_ITEM_DATA item_data;


	memset(&item_data, 0, sizeof(item_data));

#ifdef DEBUG2_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "1400\r\n");
	xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
	//_Addtolist(0,1,"vm_handle_1400");
	memset(outs, 0, outs_size);

	f_cnd = 0;
	for (n = 0; n < num; n++)
	{
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_call, sop_len) == 0)
		{
			/*检查第一个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
			{
				//memset(tmp,0,sizeof(tmp));
				//xx_get_reg_mic(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,tmp);
				//sprintf(outs+strlen(outs),"%s=",tmp);
				memset(tmp, 0, sizeof(tmp));
				//_Findsymbolicname(xx_contect[n].r[index],tmp);
				index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def);
				sprintf(outs + strlen(outs), "call:%lx  %s  ", xx_contect[n].r[index], tmp);
				call_index = n;
				f_cnd = 1;
				break;
			}
		}
	}
	if (f_cnd == 0)
	{
		//_Addtolist(0,1,"f");
		outs[0] = 0;
#ifdef DEBUG2_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "1 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}

	t_cnd = 0;
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_mov, sop_len) == 0)
		{

			/*检查第二个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
			{
				/*检查第一个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_var[1].index_scale == 0x04)
				{
					//_Addtolist(0,1,"second condition inst_num:[%d]",n);
					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_index_def);
					sprintf(outs + strlen(outs), "arg_count=%lx  ", xx_contect[n + 1].r[index]);
					//argc=xx_contect[n+1].r[index];
					t_cnd = 1;
					break;
				}

			}
		}
	}
	if (t_cnd == 0)
	{
		//_Addtolist(0,1,"t");
		outs[0] = 0;
#ifdef DEBUG2_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "2 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}

	index = get_reg_index(xreg_eax);
	sprintf(outs + strlen(outs), "retn=%lx  ", xx_contect[call_index + 1].r[index]);


#if 0
	/*mov [sp+00],t0*/
	s_cnd = 0;
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, op_mov, op_len) == 0)
		{
			/*检查第一个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_MREGDIS &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def, vm_sp) == 0 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_const[0] == 0x00 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size == INST_ITEM_SIZE_32)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_REG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def, xreg_eax) == 0)
				{
					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg_mic(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,tmp);
					//sprintf(outs+strlen(outs),"%s=",tmp);
					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					sprintf(outs + strlen(outs), "retn=%08x  ", xx_contect[n].r[index]);

					//sprintf(outs+strlen(outs),"  ");

					//_Addtolist(0,1,"third condition inst_num:[%d]",n);
					s_cnd = 1;
				}
			}
			else if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_MREG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def, vm_sp) == 0 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size == INST_ITEM_SIZE_32)
			{
				/*检查第二个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_REG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def, xreg_eax) == 0)
				{
					//memset(tmp,0,sizeof(tmp));
					//xx_get_reg_mic(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,tmp);
					//sprintf(outs+strlen(outs),"%s=",tmp);
					index = get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					sprintf(outs + strlen(outs), "retn=%08x  ", xx_contect[n].r[index]);

					//sprintf(outs+strlen(outs),"  ");

					//_Addtolist(0,1,"third condition inst_num:[%d]",n);
					s_cnd = 1;
				}
			}
		}
	}
	if (s_cnd == 0)
	{
		_Addtolist(0, 1, "s");
		outs[0] = 0;
		return -1;
	}
#endif

	iret = xx_opmz_item_write(opmz_dfile, &item_data);
	if (iret == 0)
	{
		return -1;
	}

	return 0x1400;
}



















