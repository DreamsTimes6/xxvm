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

/*

sub sp,2 ; lea sp,[sp+fe]
mov w[sp+00],wt0

=============
*/
int vm_handle_11(struct XX_INST *xx_inst,int num,struct XX_CONTECT *xx_contect,char *outs)
{
	int n=0;
	int iret=0;
	int f_cnd=0;
	int s_cnd=0;
	int t_cnd=0;
	int index=0;

	char t0[20];
	char tmp[100];
	
	_Addtolist(0,1,"vm_handle_11");
	/*sub sp,4 ; lea sp,[sp+fc]*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_sub,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_sp)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_IMM )
				{
					/*检查第二个操作项寄存器*/
					if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]=0x02)
					{
						_Addtolist(0,1,"second condition inst_num:[%d]",n);
						s_cnd=1;
					}
				}
			}
		}
	}

	/*lea sp,[sp+fc]*/
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
						if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0xfe)
						{
							_Addtolist(0,1,"second condition inst_num:[%d]",n);
							s_cnd=1;
						}
					}
				}
			}
		}
	}
	if(s_cnd==0)
	{
		outs[0]=0;
		return -1;
	}

	/*mov [sp+00],t0*/
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_mov,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_MREGDIS &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,vm_sp)==0 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_const[0]==0x00 &&
				xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_16)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG &&
					xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_data_size==INST_ITEM_SIZE_16)
				{
					memset(tmp,0,sizeof(tmp));
					sprintf(outs+strlen(outs),"dconst=");
					index=get_reg_index(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def);
					sprintf(outs+strlen(outs),"%08x",xx_contect[n].r[index]);

					sprintf(outs+strlen(outs),"  ");

					_Addtolist(0,1,"third condition inst_num:[%d]",n);
					t_cnd=1;
				}
			}
		}
	}
	if(t_cnd==0)
	{
		outs[0]=0;
		return -1;
	}


	/*没有  mov t0,[sp+index]*/
	f_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_mov,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MREGIDIS)
				{
					/*检查第二个操作项寄存器*/
					if(xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_base_def,vm_regbase)==0 )
					{
						if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].index_def_len!=0)
						{	
							_Addtolist(0,1,"first condition inst_num:[%d]",n);
							f_cnd=1;
						}
					}
				}
			}
		}
	}
	if(f_cnd==1)
	{
		outs[0]=0;
		return -1;
	}

	/*没有  mov t0,sp*/
	f_cnd=0;
	for(n=0;n<num;n++)
	{
		/*检查操作码*/
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,op_mov,op_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG &&
					xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,vm_sp)==0)
				{	
					_Addtolist(0,1,"first condition inst_num:[%d]",n);
					f_cnd=1;
				}
			}
		}
	}
	if(f_cnd==1)
	{
		outs[0]=0;
		return -1;
	}

	return 0;
}


