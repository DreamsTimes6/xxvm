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
/*
给定指令块，逐个特征识别
指令块用xx_inst数组表示
*/
//////////////////////////////////

extern struct VM_REG_STATUS reg_status_regbase;
extern struct VM_REG_STATUS reg_status_stack;

/*
定义sp寄存器
*/

extern char vm_sp[];
extern char vm_regbase[];
extern char vm_opcode[];
extern char vm_ip[];
extern char vm_key[];
extern char vm_oip[];

/*
mov edx,dword ptr [ebp]
add ebp,4
mov edi,edx
mov ebx,ebp
mov edx,0
lea esi,[const]

mov dt0,d[sp+00]
add sp,4 ;lea sp,[sp+4]
xchg dt0,sp                 
mov vm_opcode,dt0             
mov vm_key,sp               
mov dt0,0
sub vm_key,dt0       
lea vm_ip,d[dconst];mov vm_ip,dconst

vm_sp= xreg_dt0
vm_regbase= vm_regbase
vm_opcode= xreg_esp
vm_ip= vm_ip
vm_key= vm_key
*/

int vm_shandle_0f00(struct XX_INST *xx_inst,int num,char *outs,int outs_size)
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

	int index=0;
	char tmp[100];


	//_Addtolist(0,1,"vm_shandle_0f00");
	memset(outs,0,outs_size);

	/*lea vm_ip,d[dconst]*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_lea,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,vm_ip)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MIMM  )
				{
					if(memcmp(xx_inst[n].xx_inst_code.current_addr,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const,\
						xx_inst[n].xx_inst_code.inst_system)==0)
					{
						//_Addtolist(0,1,"other condition inst_num:[%d]",n);
						o_cnd=1;
					}
				}
			}
		}
	}
	if(o_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	/*没有mov sp,regbase,*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_mov,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG )
			{
				/*检查第一个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,vm_regbase)==0)
				{
					//_Addtolist(0,1,"first condition inst_num:[%d]",n);
					o_cnd=1;
				}
			}
		}
	}
	if(o_cnd==1)
	{
		outs[0]=0;
		return -1;
	}


	/*没有sub esp,000000c0 ;lea esp,[esp+ffffff40]*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_sub,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_regbase)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_IMM )
				{
					/*检查第二个操作项寄存器*/
					if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]=0xc0)
					{
						//_Addtolist(0,1,"second condition inst_num:[%d]",n);
						o_cnd=1;
					}
				}
			}
		}
	}
	for(n=0;n<num;n++)
	{
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_lea,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_regbase)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGDIS )
				{
					/*检查第二个操作项寄存器*/
					if(xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_regbase)==0)
					{
						if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0x40)
						{
							//_Addtolist(0,1,"second condition inst_num:[%d]",n);
							o_cnd=1;
						}
					}
				}
			}
		}
	}
	if(o_cnd==1)
	{
		outs[0]=0;
		return -1;
	}


	/*没有add sp,4 */
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_add,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_sp)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_IMM )
				{
					/*检查第二个操作项寄存器*/
					if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]=0x04)
					{
						//_Addtolist(0,1,"second condition inst_num:[%d]",n);
						o_cnd=1;
					}
				}
			}
		}
	}

	/*没有lea sp,[sp+4]*/
	for(n=0;n<num;n++)
	{
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_lea,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_sp)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGDIS )
				{
					/*检查第二个操作项寄存器*/
					if(xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,vm_sp)==0)
					{
						if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0x04)
						{
							//_Addtolist(0,1,"second condition inst_num:[%d]",n);
							o_cnd=1;
						}
					}
				}
			}
		}
	}
	if(o_cnd==1)
	{
		outs[0]=0;
		return -1;
	}



	return 0x0f00;
}








