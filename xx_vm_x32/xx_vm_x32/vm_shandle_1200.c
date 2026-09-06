#include "stdio.h"
//#include "Plugin.h"
#include "xx_opdef.h"
#include "xx_inst.h"
#include "xx_vm.h"





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

extern char xreg_ax[];
extern char xreg_bx[];
extern char xreg_cx[];
extern char xreg_dx[];
extern char xreg_bp[];
extern char xreg_sp[];
extern char xreg_si[];
extern char xreg_di[];

extern char xreg_al[];
extern char xreg_bl[];
extern char xreg_cl[];
extern char xreg_dl[];

extern char xreg_ah[];
extern char xreg_bh[];
extern char xreg_ch[];
extern char xreg_dh[];


extern int xreg_cmp(char* buf1,char* buf2);
extern int xreg_incmp(char* buf1,char* buf2);

extern void    cdecl _Addtolist(long addr,int highlight,char *format,...);

extern char vm_sp[];
extern char vm_regbase[];
extern char vm_opcode[];
extern char vm_ip[];
extern char vm_key[];
extern char vm_oip[];
/*
给定指令块，逐个特征识别
指令块用xx_inst数组表示
*/
//////////////////////////////////
/*
识别特征
mov sp,esp
sub esp,000000c0 ;lea esp,[esp+ffffff40]
mov vm_key,vm_opcode
lea ebp,[dconst] //ip=dconst
*/

int vm_shandle_1200(struct XX_INST *xx_inst,int num,char *outs,int outs_size)
{
	int n=0;
	int iret=0;
	int f_cnd=0;
	int s_cnd=0;
	int t_cnd=0;
	int o_cnd=0;

	char t0[20];
	char t1[20];
	char t2[20];
	char t3[20];
	char tmp[100];


	//_Addtolist(0,1,"vm_shandle_1200");
	memset(outs,0,outs_size);

	/*mov regbase,sp*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_mov,op_len)==0)
		{
			
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_regbase)==0)
			{
				/*检查第一个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,vm_sp)==0)
				{
					memset(t0,0,sizeof(t0));
					memcpy(t0,xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[0].reg_def_len);

					//_Addtolist(0,1,"first condition inst_num:[%d]",n);
					f_cnd=1;
				}
			}
		}
	}
	if(f_cnd==0)
	{
		outs[0]=0;
		return -1;
	}

	/*pop vm_sp*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_pop,op_len)==0)
		{
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_sp)==0)
			{
				//_Addtolist(0,1,"other condition inst_num:[%d]",n);
				o_cnd=1;
			}
		}
	}
	if(o_cnd==0)
	{
		outs[0]=0;
		return -1;
	}

	/*pop vm_opcode*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_pop,op_len)==0)
		{
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_opcode)==0)
			{
				//_Addtolist(0,1,"other condition inst_num:[%d]",n);
				o_cnd=1;
			}
		}
	}
	if(o_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	/*pop vm_ip*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_pop,op_len)==0)
		{
			/*检查第二个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_ip)==0)
			{
				//_Addtolist(0,1,"other condition inst_num:[%d]",n);
				o_cnd=1;
			}
		}
	}
	if(o_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	return 0x1200;
}