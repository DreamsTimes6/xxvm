#include "stdio.h"
//#include "Plugin.h"
#include "xx_opdef3.h"
#include "xx_vm.h"
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
//extern void    cdecl _Addtolist(long addr,int highlight,char *format,...);
/*
给定指令块，逐个特征识别
指令块用xx_inst数组表示
*/
//////////////////////////////////

/*
定义sp寄存器
*/

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





extern char opmz_dfile[];  //优化数据文件名

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int vm_handle_0f00(struct XX_INST *xx_inst,int num,struct XX_CONTEXT *xx_contect,char *outs,int outs_size,char *out_var)
{
	int n=0;
	int iret=0;
	int f_cnd=0;
	int s_cnd=0;
	int t_cnd=0;
	int o_cnd=0;



	int index=0;
	struct ST_VM_ITEM_DATA item_data;


	memset(&item_data, 0, sizeof(item_data));


	//_Addtolist(0,1,"vm_shandle_0f00");
	memset(outs,0,outs_size);

	/*lea vm_ip,d[dconst]*/
	o_cnd=0;
	for(n=0;n<num;n++)
	{
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_lea,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_base_def,vm_ip)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_MDIS  )
				{
					if(memcmp(xx_inst[n].xx_inst_code.current_addr,\
						xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const,\
						xx_inst[n].xx_inst_code.inst_system)==0)
					{

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
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_mov,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG && \
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,vm_sp)==0)
			{
				/*检查第一个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_REG && \
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_reg_def,vm_regbase)==0)
				{

					
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
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_sub,sop_len)==0)
		{
			/*检查第一个操作项类型*/
			if(xx_inst[n].xx_inst_items.xx_inst_items_flag[0].item_type==INST_ITEM_TYPE_REG &&
				xreg_cmp(xx_inst[n].xx_inst_items.xx_inst_items_var[0].item_reg_def,vm_regbase)==0)
			{
				/*检查第二个操作项类型*/
				if(xx_inst[n].xx_inst_items.xx_inst_items_flag[1].item_type==INST_ITEM_TYPE_IMM )
				{
					/*检查第二个操作项寄存器*/
					if(xx_inst[n].xx_inst_items.xx_inst_items_var[1].item_const[0]==0xc0)
					{

						o_cnd=1;
					}
				}
			}
		}
	}
	for(n=0;n<num;n++)
	{
		if(memcmp(xx_inst[n].xx_inst_code.opcode_type,sop_lea,sop_len)==0)
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


	iret = xx_opmz_item_write(opmz_dfile, &item_data);
	if (iret == 0)
	{
		return -1;
	}

	return 0x0f00;
}














