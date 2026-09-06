
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


int vm_handle_0801(struct XX_INST *xx_inst, int num, struct XX_CONTEXT64 *xx_contect, char *outs, int outs_size, char *out_var)
{
	int n = 0;
	int iret = 0;
	int f_cnd = 0;
	int s_cnd = 0;
	int t_cnd = 0;
	int o_cnd = 0;

	char t0[20];
	char t1[20];
	char t2[20];
	char t3[20];

	int index = 0;
	char tmp[100];

	struct ST_VM_ITEM_DATA item_data;



	memset(&item_data, 0, sizeof(item_data));
#ifdef DEBUG2_LOG
	memset(debug_buf, 0, sizeof(debug_buf));
	sprintf(debug_buf, "0801\r\n");
	xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
	//_Addtolist(0,1,"vm_handle_0800");
	memset(outs, 0, outs_size);

	/*inc edx*/
	/*inc t0*/
	o_cnd = 0;
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_inc, sop_len) == 0)
		{
			/*检查第二个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size == INST_ITEM_SIZE_32)
			{
				//_Addtolist(0,1,"other condition inst_num:[%d]",n);
				o_cnd = 1;
			}
		}
	}
	if (o_cnd == 0)
	{
		outs[0] = 0;
#ifdef DEBUG2_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "1 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}

	/*dec ecx*/
	/*dec t1*/
	o_cnd = 0;
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_dec, sop_len) == 0)
		{
			/*检查第二个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_data_size == INST_ITEM_SIZE_32)
			{
				//_Addtolist(0,1,"other condition inst_num:[%d]",n);
				o_cnd = 1;
			}
		}
	}
	if (o_cnd == 0)
	{
		outs[0] = 0;
#ifdef DEBUG2_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "2 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}

	/*mov esi,dword ptr ds:[esi*4+0x7275F8]*/
	o_cnd = 0;
	for (n = 0; n < num; n++)
	{
		/*检查操作码*/
		if (memcmp(xx_inst[n].xx_inst_code.opcode_type, sop_mov, sop_len) == 0)
		{

			/*检查第二个操作项类型*/
			if (xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type == INST_ITEM_TYPE_REG)
			{
				/*检查第一个操作项类型*/
				if (xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type == INST_ITEM_TYPE_MREGISDIS &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def, \
						xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_index_def) == 0 &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].index_scale == 0x04)
				{
					//_Addtolist(0,1,"second condition inst_num:[%d]",n);
					o_cnd = 1;
				}
			}
		}
	}
	if (o_cnd == 0)
	{
		outs[0] = 0;
#ifdef DEBUG2_LOG
		memset(debug_buf, 0, sizeof(debug_buf));
		sprintf(debug_buf, "3 %llx\r\n", *(ulong64*)xx_inst[n].xx_inst_code.current_addr);
		xx_append_file(log_file, debug_buf, strlen(debug_buf));
#endif
		return -1;
	}


	iret = xx_opmz_item_write(opmz_dfile, &item_data);
	if (iret == 0)
	{
		return -1;
	}
	return 0x0801;
}
